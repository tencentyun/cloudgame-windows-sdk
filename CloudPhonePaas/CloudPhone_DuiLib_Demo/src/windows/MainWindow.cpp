#include "MainWindow.h"

#include <thread>

#include <curl/curl.h>
#include <windows.h>

#include "core/BatchTaskOperator.h"
#include "services/ApiService.h"
#include "tcr_c_api.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include "utils/UiThreadHelper.h"
#include "viewmodels/AndroidInstanceModel.h"
#include "viewmodels/BatchTaskOperatorModel.h"
#include "windows/StreamingWindow.h"
#include "windows/dialogs/ConfirmDialog.h"
#include "windows/dialogs/InputDialogs.h"
#include "windows/dialogs/ResultDialog.h"

// Custom message for card click (wParam = heap-allocated std::string* instanceId)
#define WM_INSTANCE_CARD_CLICKED (WM_USER + 200)
#define WM_INSTANCE_CHECK_CHANGED (WM_USER + 201)

// ---- curl write callback (for image download) ----
static size_t curlWriteCallback(char* ptr, size_t size, size_t nmemb, void* userdata) {
    auto* buf = reinterpret_cast<std::vector<char>*>(userdata);
    buf->insert(buf->end(), ptr, ptr + size * nmemb);
    return size * nmemb;
}

MainWindow::MainWindow(ApiService* apiService, AndroidInstanceModel* instanceModel,
                       BatchTaskOperator* batchOperator)
    : m_apiService(apiService), m_instanceModel(instanceModel), m_batchOperator(batchOperator) {
    m_batchModel = new BatchTaskOperatorModel(batchOperator);
    m_batchModel->onShowDialog = [this](const std::string& title, const std::string& msg) {
        showResultDialog(title, msg);
    };
}

MainWindow::~MainWindow() {
    ::KillTimer(GetHWND(), TIMER_SCREENSHOT);
    delete m_batchModel;
}

LPCTSTR MainWindow::GetWindowClassName() const { return _T("MainWindowClass"); }

CDuiString MainWindow::GetSkinFile() { return _T("main.xml"); }

CDuiString MainWindow::GetSkinFolder() { return _T("skin"); }

void MainWindow::InitWindow() {
    m_tileInstances = static_cast<CTileLayoutUI*>(m_PaintManager.FindControl(_T("tile_instances")));
    m_lblStatus = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_status")));

    // Wire up instance model callbacks
    HWND hwnd = GetHWND();
    m_instanceModel->onInstancesChanged = [hwnd]() {
        UiThreadHelper::postToUiThread(hwnd, [hwnd]() {
            MainWindow* self =
                reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self)
                self->rebuildInstanceList();
        });
    };

    m_instanceModel->onError = [hwnd](const std::string& error) {
        UiThreadHelper::postToUiThread(hwnd, [hwnd, error]() {
            MainWindow* self =
                reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self && self->m_lblStatus) {
                std::wstring msg = L"错误: " + StringUtils::Utf8ToWide(error);
                self->m_lblStatus->SetText(msg.c_str());
            }
        });
    };

    // Start screenshot polling once TcrSdk is ready
    m_instanceModel->onTcrSdkReady = [hwnd]() {
        UiThreadHelper::postToUiThread(hwnd, [hwnd]() {
            MainWindow* self =
                reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self)
                self->startScreenshotTimer();
        });
    };

    // Handle images downloaded by InstanceImageDownloader (from AndroidInstanceModel)
    m_instanceModel->onImageDownloaded =
        [hwnd](const std::string& instanceId, const std::vector<uint8_t>& imageData) {
            // Save to temp file and update UI on main thread
            if (imageData.empty())
                return;

            // Write to temp file
            char tempDir[MAX_PATH] = {};
            ::GetTempPathA(MAX_PATH, tempDir);
            std::string path = std::string(tempDir) + "cai_dl_" + instanceId + ".jpg";

            FILE* f = fopen(path.c_str(), "wb");
            if (!f)
                return;
            fwrite(imageData.data(), 1, imageData.size(), f);
            fclose(f);

            std::string capturedId = instanceId;
            std::string capturedPath = path;
            UiThreadHelper::postToUiThread(hwnd, [hwnd, capturedId, capturedPath]() {
                MainWindow* self =
                    reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
                if (!self || !self->m_tileInstances)
                    return;

                std::wstring ctrlName = L"img_" + StringUtils::Utf8ToWide(capturedId);
                CControlUI* imgCtrl = self->m_PaintManager.FindControl(ctrlName.c_str());
                if (!imgCtrl)
                    return;

                imgCtrl->SetBkImage(L"");
                std::wstring wPath = StringUtils::Utf8ToWide(capturedPath);
                imgCtrl->SetBkImage(wPath.c_str());
                imgCtrl->Invalidate();
            });
        };

    // Trigger initial instance fetch (login already succeeded)
    m_instanceModel->onLoginSuccess("");
}

void MainWindow::startScreenshotTimer() {
    m_tcrSdkReady.store(true);
    Logger::info("TcrSdk ready — starting screenshot polling (2s interval)");
    ::SetTimer(GetHWND(), TIMER_SCREENSHOT, 2000, nullptr);
    // Fetch immediately without waiting for first tick
    fetchAndUpdateScreenshots();
}

void MainWindow::fetchAndUpdateScreenshots() {
    if (!m_tcrSdkReady.load())
        return;

    const auto& instances = m_instanceModel->instances();
    if (instances.empty())
        return;

    // Collect instance IDs for the worker thread
    std::vector<std::string> ids;
    for (const auto& inst : instances)
        ids.push_back(inst.AndroidInstanceId);

    HWND hwnd = GetHWND();
    int slot = m_screenshotSlot.fetch_xor(1);  // toggle 0->1->0 each poll round

    std::thread([ids, hwnd, slot]() {
        TcrClientHandle client = tcr_client_get_instance();
        TcrAndroidInstance androidInst = tcr_client_get_android_instance(client);
        if (!androidInst) {
            Logger::warning("tcr_client_get_android_instance returned null — TcrSdk not ready?");
            return;
        }

        for (const auto& instanceId : ids) {
            char urlBuf[1024] = {};
            bool ok = tcr_instance_get_image(androidInst, urlBuf, sizeof(urlBuf),
                                             instanceId.c_str(), 144, 256, 60);
            if (!ok || urlBuf[0] == '\0') {
                Logger::debug("tcr_instance_get_image failed for " + instanceId);
                continue;
            }

            std::string url(urlBuf);

            std::string tempPath = downloadToTempFile(url, instanceId, slot);
            if (tempPath.empty()) {
                Logger::warning("Failed to download screenshot for " + instanceId);
                continue;
            }

            // Post to UI thread to update the image control
            std::string capturedId = instanceId;
            std::string capturedPath = tempPath;
            UiThreadHelper::postToUiThread(hwnd, [hwnd, capturedId, capturedPath]() {
                MainWindow* self =
                    reinterpret_cast<MainWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
                if (!self || !self->m_tileInstances)
                    return;

                std::wstring ctrlName = L"img_" + StringUtils::Utf8ToWide(capturedId);
                CControlUI* imgCtrl = self->m_PaintManager.FindControl(ctrlName.c_str());
                if (!imgCtrl) {
                    Logger::debug("Image control not found: " +
                                  StringUtils::WideToUtf8(ctrlName));
                    return;
                }

                // Log control rect for diagnosis
                RECT rc = imgCtrl->GetPos();
                Logger::info("imgCtrl rect: (" + std::to_string(rc.left) + "," +
                             std::to_string(rc.top) + ") -> (" +
                             std::to_string(rc.right) + "," + std::to_string(rc.bottom) +
                             ")  size=" + std::to_string(rc.right - rc.left) + "x" +
                             std::to_string(rc.bottom - rc.top));
                Logger::info("SetBkImage path: " + capturedPath);

                // Clear previous image first to force DuiLib reload
                imgCtrl->SetBkImage(L"");
                std::wstring wPath = StringUtils::Utf8ToWide(capturedPath);
                imgCtrl->SetBkImage(wPath.c_str());
                imgCtrl->Invalidate();
                Logger::debug("Screenshot updated for " + capturedId);
            });
        }
    }).detach();
}

// static
std::string MainWindow::downloadToTempFile(const std::string& url,
                                            const std::string& instanceId, int slot) {
    CURL* curl = curl_easy_init();
    if (!curl)
        return {};

    std::vector<char> buf;
    curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
    curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
    curl_easy_setopt(curl, CURLOPT_WRITEDATA, &buf);
    curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
    curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
    curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);

    CURLcode res = curl_easy_perform(curl);
    curl_easy_cleanup(curl);

    if (res != CURLE_OK || buf.empty())
        return {};

    // Write to %TEMP%\cai_<instanceId>_<slot>.jpg  (slot 0/1 alternates to bypass DuiLib cache)
    char tempDir[MAX_PATH] = {};
    ::GetTempPathA(MAX_PATH, tempDir);
    std::string path = std::string(tempDir) + "cai_" + instanceId + "_" + std::to_string(slot) + ".jpg";

    FILE* f = fopen(path.c_str(), "wb");
    if (!f)
        return {};
    fwrite(buf.data(), 1, buf.size(), f);
    fclose(f);

    return path;
}

void MainWindow::Notify(TNotifyUI& msg) {
    if (msg.sType == DUI_MSGTYPE_CLICK) {
        CDuiString name = msg.pSender->GetName();

        if (name == _T("btn_close")) {
            PostQuitMessage(0);
            return;
        }
        if (name == _T("btn_minimize")) {
            SendMessage(WM_SYSCOMMAND, SC_MINIMIZE, 0);
            return;
        }
        if (name == _T("btn_refresh")) {
            if (m_lblStatus)
                m_lblStatus->SetText(_T("刷新中..."));
            m_instanceModel->refreshInstances();
            return;
        }
        if (name == _T("btn_sync")) {
            if (m_checkedInstanceIds.size() < 2) {
                ::MessageBox(GetHWND(), _T("请至少选择2个实例"), _T("提示"), MB_OK);
            } else {
                // Open group streaming window with all checked instances
                auto* streamWnd = new StreamingWindow(m_checkedInstanceIds, /*isGroupControl=*/true);
                streamWnd->Create(GetHWND(), _T("群控串流"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
                streamWnd->CenterWindow();
                streamWnd->ShowWindow(true);
                Logger::info("Opened group streaming window with " +
                             std::to_string(m_checkedInstanceIds.size()) + " instances");
            }
            return;
        }
        if (name == _T("btn_batch")) {
            if (m_checkedInstanceIds.empty()) {
                ::MessageBox(GetHWND(), _T("请先选择实例"), _T("提示"), MB_OK);
            } else {
                showBatchMenu();
            }
            return;
        }

        // Handle instance card clicks: outer card "card_<id>", image area "img_<id>",
        // and info bar labels also named "card_<id>" — all route to the same handler
        std::wstring nameStr(name.GetData());
        if (nameStr.find(L"card_") == 0) {
            std::string instanceId = StringUtils::WideToUtf8(nameStr.substr(5));
            onInstanceCardClicked(instanceId);
            return;
        }
        if (nameStr.find(L"img_") == 0) {
            std::string instanceId = StringUtils::WideToUtf8(nameStr.substr(4));
            onInstanceCardClicked(instanceId);
            return;
        }
    }

    // Handle checkbox state changes (COptionUI fires SELECTCHANGED after toggling,
    // so IsSelected() returns the correct *new* state here — not in CLICK).
    if (msg.sType == DUI_MSGTYPE_SELECTCHANGED) {
        CDuiString name = msg.pSender->GetName();
        std::wstring nameStr(name.GetData());
        if (nameStr.find(L"chk_") == 0) {
            std::string instanceId = StringUtils::WideToUtf8(nameStr.substr(4));
            COptionUI* chk = static_cast<COptionUI*>(msg.pSender);
            onCheckBoxChanged(instanceId, chk->IsSelected());
            return;
        }
    }

    __super::Notify(msg);
}

LRESULT MainWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_UI_CALLBACK) {
        auto* func = reinterpret_cast<std::function<void()>*>(lParam);
        if (func) {
            (*func)();
            delete func;
        }
        return 0;
    }
    if (uMsg == WM_COMMAND) {
        int menuId = LOWORD(wParam);
        if (menuId >= ID_BATCH_FIRST && menuId < ID_BATCH_FIRST + 100) {
            onBatchMenuCommand(menuId);
            return 0;
        }
    }
    if (uMsg == WM_TIMER && wParam == TIMER_SCREENSHOT) {
        fetchAndUpdateScreenshots();
        return 0;
    }
    if (uMsg == WM_DESTROY) {
        ::KillTimer(GetHWND(), TIMER_SCREENSHOT);
    }
    return __super::HandleMessage(uMsg, wParam, lParam);
}

std::wstring MainWindow::stateToDisplay(const std::string& state) {
    static const std::map<std::string, std::wstring> stateMap = {
        {"NORMAL", L"运行中"},
        {"STARTING", L"开机中"},
        {"STOPPING", L"关机中"},
        {"UPGRADING", L"服务更新中"},
        {"REBOOTING", L"重启中"},
        {"RESETING", L"重置中"},
        {"STOPPED", L"已关机"},
        {"IMAGE_CREATING", L"镜像制作中"},
        {"INITIALIZING", L"初始化中"},
        {"BACKING_UP", L"备份中"},
        {"RESTORING", L"还原中"},
    };
    auto it = stateMap.find(state);
    return it != stateMap.end() ? it->second : StringUtils::Utf8ToWide(state);
}

CContainerUI* MainWindow::createInstanceCard(const std::string& instanceId,
                                             const std::string& state,
                                             const std::string& region) {
    std::wstring wId = StringUtils::Utf8ToWide(instanceId);
    std::wstring wState = stateToDisplay(state);
    std::wstring wRegion = StringUtils::Utf8ToWide(region);

    // Outer container (clickable card) — portrait phone aspect ratio 144:256 (9:16)
    // Layout (top to bottom): checkbox bar (24px) + screenshot (260px) + info bar (54px) = 338px
    auto* card = new CVerticalLayoutUI();
    card->SetFixedWidth(160);
    card->SetFixedHeight(338);
    card->SetBkColor(0xFFFFFFFF);
    card->SetBorderSize(1);
    card->SetBorderColor(0xFFDDDDDD);
    card->SetName((L"card_" + wId).c_str());
    card->SetMouseEnabled(true);

    // ── Checkbox bar (top strip) ──────────────────────────────────────────────
    // Semi-transparent dark bar so the checkbox is visible over any background.
    auto* chkBar = new CHorizontalLayoutUI();
    chkBar->SetFixedHeight(24);
    chkBar->SetBkColor(0xBB000000);  // Semi-transparent black
    chkBar->SetInset({4, 3, 4, 3});

    auto* chk = new COptionUI();
    chk->SetName((L"chk_" + wId).c_str());
    chk->SetText(L"");           // No label — the card itself shows the instance info
    chk->SetFixedWidth(18);
    chk->SetFixedHeight(18);
    chk->SetMouseEnabled(true);
    // Visually distinguish selected state with a highlight background
    chk->SetSelectedBkColor(0xFF2D8CF0);  // Blue when checked
    chkBar->Add(chk);

    // Filler so the checkbox stays on the left
    auto* chkSpacer = new CControlUI();
    chkBar->Add(chkSpacer);

    card->Add(chkBar);

    // ── Screenshot area ───────────────────────────────────────────────────────
    // Reduced from 284px to 260px to make room for the checkbox bar.
    auto* imageArea = new CButtonUI();
    imageArea->SetFixedHeight(260);
    imageArea->SetBkColor(0xFFE8E8E8);
    imageArea->SetHotBkColor(0xFFE8E8E8);   // no hover colour change
    imageArea->SetName((L"img_" + wId).c_str());  // "img_<id>" for screenshot updates
    imageArea->SetMouseEnabled(true);
    card->Add(imageArea);

    // ── Bottom info bar ───────────────────────────────────────────────────────
    auto* infoBar = new CVerticalLayoutUI();
    infoBar->SetFixedHeight(54);
    infoBar->SetBkColor(0xCC000000);  // Semi-transparent black
    infoBar->SetInset({5, 3, 5, 3});

    // Instance ID
    auto* lblId = new CLabelUI();
    lblId->SetText(wId.c_str());
    lblId->SetTextColor(0xFFFFFFFF);
    lblId->SetFont(3);
    lblId->SetFixedHeight(18);
    lblId->SetName((L"card_" + wId).c_str());
    lblId->SetMouseEnabled(true);
    infoBar->Add(lblId);

    // State
    auto* lblState = new CLabelUI();
    lblState->SetText(wState.c_str());
    lblState->SetTextColor(0xFFFFFFFF);
    lblState->SetFont(3);
    lblState->SetFixedHeight(18);
    lblState->SetName((L"card_" + wId).c_str());
    lblState->SetMouseEnabled(true);
    infoBar->Add(lblState);

    // Region
    auto* lblRegion = new CLabelUI();
    lblRegion->SetText(wRegion.c_str());
    lblRegion->SetTextColor(0xFFCCCCCC);
    lblRegion->SetFont(3);
    lblRegion->SetFixedHeight(18);
    lblRegion->SetName((L"card_" + wId).c_str());
    lblRegion->SetMouseEnabled(true);
    infoBar->Add(lblRegion);

    card->Add(infoBar);

    return card;
}

void MainWindow::rebuildInstanceList() {
    if (!m_tileInstances)
        return;

    m_tileInstances->RemoveAll();
    const auto& instances = m_instanceModel->instances();

    for (const auto& inst : instances) {
        auto* card = createInstanceCard(inst.AndroidInstanceId, inst.State,
                                        inst.AndroidInstanceRegion);
        m_tileInstances->Add(card);
    }

    // Restore checkbox state for any instances that were already checked
    for (const auto& checkedId : m_checkedInstanceIds) {
        std::wstring chkName = L"chk_" + StringUtils::Utf8ToWide(checkedId);
        COptionUI* chk = static_cast<COptionUI*>(m_PaintManager.FindControl(chkName.c_str()));
        if (chk)
            chk->Selected(true, /*bTriggerEvent=*/false);
    }

    if (m_lblStatus) {
        std::wstring statusText =
            L"共 " + std::to_wstring(instances.size()) + L" 个实例";
        m_lblStatus->SetText(statusText.c_str());
    }

    Logger::info("Instance list rebuilt, " + std::to_string(instances.size()) + " cards");

    // If TcrSdk is already ready, refresh screenshots immediately for the new cards
    if (m_tcrSdkReady.load())
        fetchAndUpdateScreenshots();
}

void MainWindow::onInstanceCardClicked(const std::string& instanceId) {
    Logger::info("Instance card clicked: " + instanceId);

    // Check if streaming window already open for this instance
    auto it = m_streamingWindowMap.find(instanceId);
    if (it != m_streamingWindowMap.end() && ::IsWindow(it->second)) {
        ::ShowWindow(it->second, SW_SHOW);
        ::SetForegroundWindow(it->second);
        return;
    }

    openStreamingWindow(instanceId);
}

void MainWindow::onCheckBoxChanged(const std::string& instanceId, bool checked) {
    if (checked) {
        if (std::find(m_checkedInstanceIds.begin(), m_checkedInstanceIds.end(), instanceId) ==
            m_checkedInstanceIds.end()) {
            m_checkedInstanceIds.push_back(instanceId);
        }
    } else {
        m_checkedInstanceIds.erase(
            std::remove(m_checkedInstanceIds.begin(), m_checkedInstanceIds.end(), instanceId),
            m_checkedInstanceIds.end());
    }
    Logger::debug("Checked instances: " + std::to_string(m_checkedInstanceIds.size()));
}

void MainWindow::openStreamingWindow(const std::string& instanceId) {
    std::vector<std::string> ids = {instanceId};
    auto* streamWnd = new StreamingWindow(ids, /*isGroupControl=*/false);
    std::wstring title = L"串流 - " + StringUtils::Utf8ToWide(instanceId);
    streamWnd->Create(GetHWND(), title.c_str(), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
    streamWnd->CenterWindow();
    streamWnd->ShowWindow(true);

    m_streamingWindowMap[instanceId] = streamWnd->GetHWND();
    Logger::info("Opened streaming window for: " + instanceId);
}

// ============================================================================
// Batch Operations — popup menu and dispatch
// ============================================================================

// Menu item IDs: ID_BATCH_FIRST + offset
enum BatchMenuId {
    BM_MODIFY_RESOLUTION = 0,
    BM_MODIFY_GPS,
    BM_PASTE,
    BM_SEND_CLIPBOARD,
    BM_MODIFY_SENSOR,
    BM_SEP1,
    BM_SHAKE,
    BM_BLOW,
    BM_REBOOT,
    BM_MOVE_APP_BACKGROUND,
    BM_SEP2,
    BM_SEND_TRANS_MESSAGE,
    BM_MODIFY_INSTANCE_PROPERTIES,
    BM_DESCRIBE_INSTANCE_PROPERTIES,
    BM_SEP3,
    BM_MODIFY_KEEP_FRONT_APP_STATUS,
    BM_DESCRIBE_KEEP_FRONT_APP_STATUS,
    BM_SEP4,
    BM_UNINSTALL_APP,
    BM_START_APP,
    BM_STOP_APP,
    BM_CLEAR_APP_DATA,
    BM_ENABLE_APP,
    BM_DISABLE_APP,
    BM_LIST_USER_APPS,
    BM_LIST_ALL_APPS,
    BM_SEP5,
    BM_START_CAMERA_MEDIA_PLAY,
    BM_DISPLAY_CAMERA_IMAGE,
    BM_STOP_CAMERA_MEDIA_PLAY,
    BM_DESCRIBE_CAMERA_MEDIA_PLAY_STATUS,
    BM_SEP6,
    BM_ADD_KEEP_ALIVE_LIST,
    BM_REMOVE_KEEP_ALIVE_LIST,
    BM_SET_KEEP_ALIVE_LIST,
    BM_DESCRIBE_KEEP_ALIVE_LIST,
    BM_CLEAR_KEEP_ALIVE_LIST,
    BM_SEP7,
    BM_MUTE,
    BM_MEDIA_SEARCH,
    BM_SEP8,
    BM_ADD_APP_INSTALL_BLACK_LIST,
    BM_REMOVE_APP_INSTALL_BLACK_LIST,
    BM_SET_APP_INSTALL_BLACK_LIST,
    BM_DESCRIBE_APP_INSTALL_BLACK_LIST,
    BM_CLEAR_APP_INSTALL_BLACK_LIST,
};

void MainWindow::showBatchMenu() {
    HMENU hMenu = ::CreatePopupMenu();

    auto addItem = [&](int offset, const wchar_t* text) {
        ::AppendMenu(hMenu, MF_STRING, ID_BATCH_FIRST + offset, text);
    };
    auto addSep = [&]() {
        ::AppendMenu(hMenu, MF_SEPARATOR, 0, nullptr);
    };

    // Group 1: Device settings
    addItem(BM_MODIFY_RESOLUTION, L"修改分辨率");
    addItem(BM_MODIFY_GPS, L"修改GPS");
    addItem(BM_PASTE, L"粘贴文本");
    addItem(BM_SEND_CLIPBOARD, L"发送剪贴板");
    addItem(BM_MODIFY_SENSOR, L"修改传感器");
    addSep();

    // Group 2: Device actions
    addItem(BM_SHAKE, L"摇动设备");
    addItem(BM_BLOW, L"吹气");
    addItem(BM_REBOOT, L"重启实例");
    addItem(BM_MOVE_APP_BACKGROUND, L"应用退到后台");
    addSep();

    // Group 3: Instance properties & messages
    addItem(BM_SEND_TRANS_MESSAGE, L"发送透传消息");
    addItem(BM_MODIFY_INSTANCE_PROPERTIES, L"修改实例属性");
    addItem(BM_DESCRIBE_INSTANCE_PROPERTIES, L"查询实例属性");
    addSep();

    // Group 4: Keep front app
    addItem(BM_MODIFY_KEEP_FRONT_APP_STATUS, L"修改保活应用状态");
    addItem(BM_DESCRIBE_KEEP_FRONT_APP_STATUS, L"查询保活应用状态");
    addSep();

    // Group 5: App management
    addItem(BM_UNINSTALL_APP, L"卸载应用");
    addItem(BM_START_APP, L"启动应用");
    addItem(BM_STOP_APP, L"停止应用");
    addItem(BM_CLEAR_APP_DATA, L"清除应用数据");
    addItem(BM_ENABLE_APP, L"启用应用");
    addItem(BM_DISABLE_APP, L"禁用应用");
    addItem(BM_LIST_USER_APPS, L"列出用户应用");
    addItem(BM_LIST_ALL_APPS, L"列出所有应用");
    addSep();

    // Group 6: Camera media
    addItem(BM_START_CAMERA_MEDIA_PLAY, L"开始摄像头媒体播放");
    addItem(BM_DISPLAY_CAMERA_IMAGE, L"显示摄像头图片");
    addItem(BM_STOP_CAMERA_MEDIA_PLAY, L"停止摄像头播放");
    addItem(BM_DESCRIBE_CAMERA_MEDIA_PLAY_STATUS, L"查询摄像头播放状态");
    addSep();

    // Group 7: Keep alive list
    addItem(BM_ADD_KEEP_ALIVE_LIST, L"添加保活列表");
    addItem(BM_REMOVE_KEEP_ALIVE_LIST, L"移除保活列表");
    addItem(BM_SET_KEEP_ALIVE_LIST, L"设置保活列表");
    addItem(BM_DESCRIBE_KEEP_ALIVE_LIST, L"查询保活列表");
    addItem(BM_CLEAR_KEEP_ALIVE_LIST, L"清除保活列表");
    addSep();

    // Group 8: Mute & media
    addItem(BM_MUTE, L"静音设置");
    addItem(BM_MEDIA_SEARCH, L"媒体库搜索");
    addSep();

    // Group 9: App install blacklist
    addItem(BM_ADD_APP_INSTALL_BLACK_LIST, L"添加安装黑名单");
    addItem(BM_REMOVE_APP_INSTALL_BLACK_LIST, L"移除安装黑名单");
    addItem(BM_SET_APP_INSTALL_BLACK_LIST, L"设置安装黑名单");
    addItem(BM_DESCRIBE_APP_INSTALL_BLACK_LIST, L"查询安装黑名单");
    addItem(BM_CLEAR_APP_INSTALL_BLACK_LIST, L"清空安装黑名单");

    POINT pt;
    ::GetCursorPos(&pt);
    ::TrackPopupMenu(hMenu, TPM_LEFTALIGN | TPM_TOPALIGN, pt.x, pt.y, 0, GetHWND(), nullptr);
    ::DestroyMenu(hMenu);
}

void MainWindow::onBatchMenuCommand(int menuId) {
    int offset = menuId - ID_BATCH_FIRST;
    HWND hwnd = GetHWND();

    // Helper: dispatch to model with no extra params (confirm-only operations)
    auto dispatchConfirm = [&](const std::string& dialogType, const std::wstring& msg) {
        ConfirmDialog dlg;
        dlg.setTitle(L"确认操作");
        dlg.setMessage(msg);
        if (dlg.doModal(hwnd)) {
            m_batchModel->handleDialogSignal(dialogType, m_checkedInstanceIds);
        }
    };

    // Helper: dispatch with package name
    auto dispatchPackageName = [&](const std::string& dialogType, const std::wstring& title) {
        PackageNameDialog dlg;
        dlg.setTitle(title);
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["packageName"] = dlg.packageName();
            m_batchModel->handleDialogSignal(dialogType, m_checkedInstanceIds, params);
        }
    };

    // Helper: dispatch with app list
    auto dispatchAppList = [&](const std::string& dialogType, const std::wstring& title) {
        AppListDialog dlg;
        dlg.setTitle(title);
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["appList"] = dlg.appListStr();
            m_batchModel->handleDialogSignal(dialogType, m_checkedInstanceIds, params);
        }
    };

    switch (offset) {
    case BM_MODIFY_RESOLUTION: {
        ResolutionDialog dlg;
        dlg.setTitle(L"修改分辨率");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["width"] = std::to_string(dlg.width());
            params["height"] = std::to_string(dlg.height());
            params["dpi"] = std::to_string(dlg.dpi());
            m_batchModel->handleDialogSignal("resolution", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_MODIFY_GPS: {
        GpsDialog dlg;
        dlg.setTitle(L"修改GPS");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["longitude"] = std::to_string(dlg.longitude());
            params["latitude"] = std::to_string(dlg.latitude());
            m_batchModel->handleDialogSignal("gps", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_PASTE: {
        TextInputDialog dlg;
        dlg.setTitle(L"粘贴文本");
        dlg.setFieldLabel(L"文本内容:");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["text"] = dlg.text();
            m_batchModel->handleDialogSignal("paste", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_SEND_CLIPBOARD: {
        TextInputDialog dlg;
        dlg.setTitle(L"发送剪贴板");
        dlg.setFieldLabel(L"剪贴板内容:");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["text"] = dlg.text();
            m_batchModel->handleDialogSignal("sendClipboard", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_MODIFY_SENSOR: {
        SensorDialog dlg;
        dlg.setTitle(L"修改传感器");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["sensorType"] = dlg.sensorType();
            params["values"] = dlg.valuesStr();
            params["accuracy"] = std::to_string(dlg.accuracy());
            m_batchModel->handleDialogSignal("modifySensor", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_SHAKE:
        dispatchConfirm("shake", L"确定要对选中的实例执行摇动操作吗？");
        break;
    case BM_BLOW:
        dispatchConfirm("blow", L"确定要对选中的实例执行吹气操作吗？");
        break;
    case BM_REBOOT:
        dispatchConfirm("reboot", L"确定要重启选中的实例吗？");
        break;
    case BM_MOVE_APP_BACKGROUND:
        dispatchConfirm("moveAppBackground", L"确定要将选中实例的前台应用退到后台吗？");
        break;
    case BM_SEND_TRANS_MESSAGE: {
        TwoFieldDialog dlg;
        dlg.setTitle(L"发送透传消息");
        dlg.setFieldLabels(L"包名 (PackageName):", L"消息内容 (Msg):");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["packageName"] = dlg.field1();
            params["msg"] = dlg.field2();
            m_batchModel->handleDialogSignal("message", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_MODIFY_INSTANCE_PROPERTIES: {
        TextInputDialog dlg;
        dlg.setTitle(L"修改实例属性");
        dlg.setFieldLabel(L"属性JSON:");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["json"] = dlg.text();
            m_batchModel->handleDialogSignal("modifyInstanceProperties", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_DESCRIBE_INSTANCE_PROPERTIES:
        dispatchConfirm("describeInstanceProperties", L"查询选中实例的属性信息？");
        break;
    case BM_MODIFY_KEEP_FRONT_APP_STATUS: {
        KeepFrontDialog dlg;
        dlg.setTitle(L"修改保活应用状态");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["packageName"] = dlg.packageName();
            params["enable"] = dlg.enable() ? "1" : "0";
            params["restartInterval"] = std::to_string(dlg.restartInterval());
            m_batchModel->handleDialogSignal("modifyKeepFrontAppStatus", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_DESCRIBE_KEEP_FRONT_APP_STATUS:
        dispatchConfirm("describeKeepFrontAppStatus", L"查询选中实例的保活应用状态？");
        break;
    case BM_UNINSTALL_APP:
        dispatchPackageName("unInstallByPackageName", L"卸载应用");
        break;
    case BM_START_APP: {
        StartAppDialog dlg;
        dlg.setTitle(L"启动应用");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["packageName"] = dlg.packageName();
            params["activityName"] = dlg.activityName();
            m_batchModel->handleDialogSignal("startApp", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_STOP_APP:
        dispatchPackageName("stopApp", L"停止应用");
        break;
    case BM_CLEAR_APP_DATA:
        dispatchPackageName("clearAppData", L"清除应用数据");
        break;
    case BM_ENABLE_APP:
        dispatchPackageName("enableApp", L"启用应用");
        break;
    case BM_DISABLE_APP:
        dispatchPackageName("disableApp", L"禁用应用");
        break;
    case BM_LIST_USER_APPS:
        dispatchConfirm("listUserApps", L"列出选中实例的用户应用？");
        break;
    case BM_LIST_ALL_APPS:
        dispatchConfirm("listAllApps", L"列出选中实例的所有应用？");
        break;
    case BM_START_CAMERA_MEDIA_PLAY: {
        CameraPlayDialog dlg;
        dlg.setTitle(L"开始摄像头媒体播放");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["filePath"] = dlg.filePath();
            params["loops"] = std::to_string(dlg.loops());
            m_batchModel->handleDialogSignal("startCameraMediaPlay", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_DISPLAY_CAMERA_IMAGE: {
        TextInputDialog dlg;
        dlg.setTitle(L"显示摄像头图片");
        dlg.setFieldLabel(L"文件路径 (FilePath):");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["filePath"] = dlg.text();
            m_batchModel->handleDialogSignal("displayCameraImage", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_STOP_CAMERA_MEDIA_PLAY:
        dispatchConfirm("stopCameraMediaPlay", L"停止选中实例的摄像头播放？");
        break;
    case BM_DESCRIBE_CAMERA_MEDIA_PLAY_STATUS:
        dispatchConfirm("describeCameraMediaPlayStatus", L"查询选中实例的摄像头播放状态？");
        break;
    case BM_ADD_KEEP_ALIVE_LIST:
        dispatchAppList("addKeepAliveList", L"添加保活列表");
        break;
    case BM_REMOVE_KEEP_ALIVE_LIST:
        dispatchAppList("removeKeepAliveList", L"移除保活列表");
        break;
    case BM_SET_KEEP_ALIVE_LIST:
        dispatchAppList("setKeepAliveList", L"设置保活列表");
        break;
    case BM_DESCRIBE_KEEP_ALIVE_LIST:
        dispatchConfirm("describeKeepAliveList", L"查询选中实例的保活列表？");
        break;
    case BM_CLEAR_KEEP_ALIVE_LIST:
        dispatchConfirm("clearKeepAliveList", L"清除选中实例的保活列表？");
        break;
    case BM_MUTE: {
        MuteDialog dlg;
        dlg.setTitle(L"静音设置");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["mute"] = dlg.isMuted() ? "1" : "0";
            m_batchModel->handleDialogSignal("mute", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_MEDIA_SEARCH: {
        TextInputDialog dlg;
        dlg.setTitle(L"媒体库搜索");
        dlg.setFieldLabel(L"搜索关键字:");
        if (dlg.doModal(hwnd)) {
            std::map<std::string, std::string> params;
            params["keyword"] = dlg.text();
            m_batchModel->handleDialogSignal("mediaSearch", m_checkedInstanceIds, params);
        }
        break;
    }
    case BM_ADD_APP_INSTALL_BLACK_LIST:
        dispatchAppList("addAppInstallBlackList", L"添加安装黑名单");
        break;
    case BM_REMOVE_APP_INSTALL_BLACK_LIST:
        dispatchAppList("removeAppInstallBlackList", L"移除安装黑名单");
        break;
    case BM_SET_APP_INSTALL_BLACK_LIST:
        dispatchAppList("setAppInstallBlackList", L"设置安装黑名单");
        break;
    case BM_DESCRIBE_APP_INSTALL_BLACK_LIST:
        dispatchConfirm("describeAppInstallBlackList", L"查询选中实例的安装黑名单？");
        break;
    case BM_CLEAR_APP_INSTALL_BLACK_LIST:
        dispatchConfirm("clearAppInstallBlackList", L"清空选中实例的安装黑名单？");
        break;
    default:
        Logger::warning("Unknown batch menu ID: " + std::to_string(menuId));
        break;
    }
}

void MainWindow::showResultDialog(const std::string& title, const std::string& message) {
    ResultDialog dlg;
    dlg.setTitle(StringUtils::Utf8ToWide(title));
    dlg.setResultText(StringUtils::Utf8ToWide(message));
    dlg.doModal(GetHWND());
}
