#include "StreamingWindow.h"

#include <commctrl.h>  // SetWindowSubclass
#include <windowsx.h>  // GET_X_LPARAM, GET_Y_LPARAM

#include "core/video/Frame.h"
#include "core/video/VideoRenderer.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include "utils/UiThreadHelper.h"
#include "viewmodels/StreamingViewModel.h"
#include "windows/dialogs/ResultDialog.h"

// Link commctrl for SetWindowSubclass
#pragma comment(lib, "comctl32.lib")

// Subclass ID for video child window
static constexpr UINT_PTR VIDEO_SUBCLASS_ID = 1;

// ==================== Construction / Destruction ====================

StreamingWindow::StreamingWindow(const std::vector<std::string>& instanceIds, bool isGroupControl)
    : m_instanceIds(instanceIds), m_isGroupControl(isGroupControl) {}

StreamingWindow::~StreamingWindow() {
    // Remove subclass before destroying
    if (m_videoHwnd && ::IsWindow(m_videoHwnd)) {
        ::RemoveWindowSubclass(m_videoHwnd, VideoSubclassProc, VIDEO_SUBCLASS_ID);
    }
    // ViewModel destructor will close the session
    m_viewModel.reset();
    m_renderer.reset();
}

LPCTSTR StreamingWindow::GetWindowClassName() const {
    return _T("StreamingWindowClass");
}

CDuiString StreamingWindow::GetSkinFile() {
    return _T("streaming.xml");
}

CDuiString StreamingWindow::GetSkinFolder() {
    return _T("skin");
}

// ==================== Init ====================

void StreamingWindow::InitWindow() {
    // Find DuiLib controls
    m_lblSessionStatus = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_session_status")));
    m_lblStats = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_stats")));
    m_editMinBitrate = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_min_bitrate")));
    m_editMaxBitrate = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_max_bitrate")));
    m_editFps = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_fps")));

    // Render scale buttons
    m_btnScale25  = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_scale_25")));
    m_btnScale50  = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_scale_50")));
    m_btnScale100 = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_scale_100")));
    m_btnScale150 = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_scale_150")));
    m_btnScale200 = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_scale_200")));

    // Create video child window inside the video_placeholder area
    createVideoChildWindow();

    // Create renderer targeting the video child window
    m_renderer = std::make_unique<VideoRenderer>(m_videoHwnd);

    // Create ViewModel (uses our window for PostMessage thread switching)
    m_viewModel = std::make_unique<StreamingViewModel>(GetHWND());

    // Wire up ViewModel callbacks
    m_viewModel->onNewVideoFrame = [this](VideoFrameDataPtr frame) {
        onVideoFrameReceived(std::move(frame));
    };

    m_viewModel->onSessionConnected = [this]() {
        if (m_lblSessionStatus) m_lblSessionStatus->SetText(_T("已连接"));
        Logger::info("[StreamingWindow] session connected");
    };

    m_viewModel->onSessionClosed = [this](const std::string& reason) {
        if (m_lblSessionStatus) {
            std::wstring msg = L"已断开: " + StringUtils::Utf8ToWide(reason);
            m_lblSessionStatus->SetText(msg.c_str());
        }
        Logger::info("[StreamingWindow] session closed: " + reason);
    };

    m_viewModel->onClientStatsChanged = [this](const std::string& stats) {
        if (m_lblStats) {
            // Show a brief summary (just fps + bitrate from JSON)
            m_lblStats->SetText(_T("Stats OK"));
        }
    };

    m_viewModel->onScreenOrientationChanged = [this](bool isLandscape, double rotationAngle, int w, int h) {
        Logger::info("[StreamingWindow] orientation changed: " +
                     std::string(isLandscape ? "landscape" : "portrait") +
                     " rotation=" + std::to_string(static_cast<int>(rotationAngle)) +
                     " " + std::to_string(w) + "x" + std::to_string(h));
        // Apply rotation to renderer
        if (m_renderer) {
            m_renderer->setRotationAngle(rotationAngle);
        }
    };

    // Start streaming session
    if (m_lblSessionStatus) m_lblSessionStatus->SetText(_T("连接中..."));
    m_viewModel->connectSession(m_instanceIds, m_isGroupControl);

    // Force layout now so video_placeholder has a valid rect, then sync video child window.
    // DuiLib doesn't finish layout until after InitWindow() returns, so we trigger it manually.
    RECT rcClient;
    ::GetClientRect(GetHWND(), &rcClient);
    m_PaintManager.SetInitSize(rcClient.right - rcClient.left, rcClient.bottom - rcClient.top);
    m_PaintManager.NeedUpdate();
    syncVideoWindowPos();
}

// ==================== Video Child Window ====================

void StreamingWindow::createVideoChildWindow() {
    // Create child window with a temporary 1x1 size.
    // DuiLib layout hasn't run yet at InitWindow() time, so video_placeholder->GetPos()
    // would return {0,0,0,0}. The real position is applied in syncVideoWindowPos().
    m_videoHwnd = ::CreateWindowExW(
        0, L"STATIC", nullptr,
        WS_CHILD | WS_VISIBLE | WS_CLIPSIBLINGS | SS_NOTIFY,
        0, 0, 1, 1,
        GetHWND(), nullptr,
        CPaintManagerUI::GetInstance(), nullptr);

    if (m_videoHwnd) {
        // Subclass the video window to intercept mouse events
        ::SetWindowSubclass(m_videoHwnd, VideoSubclassProc, VIDEO_SUBCLASS_ID,
                            reinterpret_cast<DWORD_PTR>(this));
        Logger::info("[StreamingWindow] video child window created (initial 1x1, will resize on layout)");
    } else {
        Logger::error("[StreamingWindow] failed to create video child window");
    }
}

void StreamingWindow::syncVideoWindowPos() {
    if (!m_videoHwnd) return;
    CControlUI* placeholder = m_PaintManager.FindControl(_T("video_placeholder"));
    if (!placeholder) return;
    RECT rc = placeholder->GetPos();
    int w = rc.right - rc.left;
    int h = rc.bottom - rc.top;
    if (w > 0 && h > 0) {
        ::MoveWindow(m_videoHwnd, rc.left, rc.top, w, h, TRUE);
        Logger::info("[StreamingWindow] video child window synced: ("
                     + std::to_string(rc.left) + "," + std::to_string(rc.top)
                     + ") " + std::to_string(w) + "x" + std::to_string(h));
    }
}

void StreamingWindow::onVideoFrameReceived(VideoFrameDataPtr frame) {
    if (!frame) {
        Logger::error("[onVideoFrameReceived] frame is null");
        return;
    }
    if (!m_renderer) {
        Logger::error("[onVideoFrameReceived] m_renderer is null");
        return;
    }

    static std::atomic<int> s_renderCount{0};
    int cnt = s_renderCount.fetch_add(1);
    if (cnt == 0) {
        Logger::info("[onVideoFrameReceived] first frame to renderer: "
                     + std::to_string(frame->width) + "x" + std::to_string(frame->height)
                     + " videoHwnd=" + std::to_string(reinterpret_cast<uintptr_t>(m_videoHwnd))
                     + " hwndValid=" + std::to_string(::IsWindow(m_videoHwnd) ? 1 : 0));
        if (m_videoHwnd) {
            RECT rc;
            ::GetClientRect(m_videoHwnd, &rc);
            Logger::info("[onVideoFrameReceived] videoHwnd clientRect: "
                         + std::to_string(rc.right - rc.left) + "x" + std::to_string(rc.bottom - rc.top));
        }
    }

    // If the video child window hasn't been sized yet (still at initial 1x1),
    // sync it now before rendering — layout should be complete by the time frames arrive.
    if (m_videoHwnd) {
        RECT rcWnd;
        ::GetClientRect(m_videoHwnd, &rcWnd);
        if (rcWnd.right <= 1 || rcWnd.bottom <= 1) {
            syncVideoWindowPos();
        }
    }

    // Track video dimensions for mouse coordinate mapping
    m_videoWidth = frame->width;
    m_videoHeight = frame->height;

    m_renderer->renderFrame(frame->frameHandle, frame->width, frame->height);
}

// ==================== Mouse Event Handling ====================

void StreamingWindow::onVideoMouseEvent(UINT msg, WPARAM wParam, LPARAM lParam) {
    if (!m_viewModel || m_videoWidth <= 0 || m_videoHeight <= 0) return;

    // Get mouse position in video child window client coords
    int clientX = GET_X_LPARAM(lParam);
    int clientY = GET_Y_LPARAM(lParam);

    // Get video child window client rect
    RECT rc;
    ::GetClientRect(m_videoHwnd, &rc);
    int clientW = rc.right - rc.left;
    int clientH = rc.bottom - rc.top;
    if (clientW <= 0 || clientH <= 0) return;

    // Map client coords to video coords
    int videoX = static_cast<int>(static_cast<float>(clientX) / clientW * m_videoWidth);
    int videoY = static_cast<int>(static_cast<float>(clientY) / clientH * m_videoHeight);

    int64_t timestamp = static_cast<int64_t>(::GetTickCount64());

    int eventType = -1;
    switch (msg) {
    case WM_LBUTTONDOWN:
        eventType = 0;  // press
        ::SetCapture(m_videoHwnd);
        break;
    case WM_MOUSEMOVE:
        if (wParam & MK_LBUTTON)
            eventType = 1;  // move (only when button held)
        break;
    case WM_LBUTTONUP:
        eventType = 2;  // release
        ::ReleaseCapture();
        break;
    default:
        break;
    }

    if (eventType >= 0) {
        m_viewModel->sendTouchEvent(videoX, videoY, m_videoWidth, m_videoHeight, eventType, timestamp);
    }

    // Mouse wheel → scroll event
    if (msg == WM_MOUSEWHEEL) {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wParam);
        float delta = (zDelta > 0) ? 1.0f : -1.0f;
        m_viewModel->sendMouseScrollEvent(delta);
    }
}

LRESULT CALLBACK StreamingWindow::VideoSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                                     UINT_PTR /*uIdSubclass*/, DWORD_PTR dwRefData) {
    auto* self = reinterpret_cast<StreamingWindow*>(dwRefData);
    if (self) {
        switch (msg) {
        case WM_LBUTTONDOWN:
        case WM_LBUTTONUP:
        case WM_MOUSEMOVE:
        case WM_MOUSEWHEEL:
            self->onVideoMouseEvent(msg, wParam, lParam);
            return 0;
        }
    }
    return ::DefSubclassProc(hwnd, msg, wParam, lParam);
}

// ==================== Button Click Handling ====================

void StreamingWindow::Notify(TNotifyUI& msg) {
    if (msg.sType == DUI_MSGTYPE_CLICK) {
        CDuiString name = msg.pSender->GetName();

        if (name == _T("btn_close")) {
            Close(IDCANCEL);
            return;
        }

        if (!m_viewModel) {
            __super::Notify(msg);
            return;
        }

        // System keys
        if (name == _T("btn_home"))        { m_viewModel->onHomeClicked(); return; }
        if (name == _T("btn_menu"))        { m_viewModel->onMenuClicked(); return; }
        if (name == _T("btn_back"))        { m_viewModel->onBackClicked(); return; }
        if (name == _T("btn_vol_up"))      { m_viewModel->onVolumUp(); return; }
        if (name == _T("btn_vol_down"))    { m_viewModel->onVolumDown(); return; }

        // Stream control
        if (name == _T("btn_pause_video"))  { m_viewModel->onPauseVideoStreamClicked(); return; }
        if (name == _T("btn_resume_video")) { m_viewModel->onResumeVideoStreamClicked(); return; }
        if (name == _T("btn_pause_audio"))  { m_viewModel->onPauseAudioStreamClicked(); return; }
        if (name == _T("btn_resume_audio")) { m_viewModel->onResumeAudioStreamClicked(); return; }

        // Device control
        if (name == _T("btn_enable_camera"))  { m_viewModel->onEnableCameraClicked(); return; }
        if (name == _T("btn_disable_camera")) { m_viewModel->onDisableCameraClicked(); return; }
        if (name == _T("btn_enable_mic"))     { m_viewModel->onEnableMicrophoneClicked(); return; }
        if (name == _T("btn_disable_mic"))    { m_viewModel->onDisableMicrophoneClicked(); return; }

        // Video settings
        if (name == _T("btn_video_settings")) { m_viewModel->onVideoStreamSettingsClicked(); return; }

        // Apply custom video settings
        if (name == _T("btn_apply_settings")) {
            int fps = 30, minBitrate = 1000, maxBitrate = 3000;
            if (m_editFps) fps = _ttoi(m_editFps->GetText());
            if (m_editMinBitrate) minBitrate = _ttoi(m_editMinBitrate->GetText());
            if (m_editMaxBitrate) maxBitrate = _ttoi(m_editMaxBitrate->GetText());
            if (fps <= 0) fps = 30;
            if (minBitrate <= 0) minBitrate = 1000;
            if (maxBitrate <= 0) maxBitrate = 3000;
            m_viewModel->setRemoteVideoProfile(fps, minBitrate, maxBitrate, 720, 1280);
            Logger::info("[StreamingWindow] applied video settings: fps=" + std::to_string(fps) +
                         " bitrate=" + std::to_string(minBitrate) + "-" + std::to_string(maxBitrate));
            return;
        }

        // Render scale
        if (name == _T("btn_scale_25"))  { m_renderer->setRenderScale(0.25); return; }
        if (name == _T("btn_scale_50"))  { m_renderer->setRenderScale(0.5);  return; }
        if (name == _T("btn_scale_100")) { m_renderer->setRenderScale(1.0);  return; }
        if (name == _T("btn_scale_150")) { m_renderer->setRenderScale(1.5);  return; }
        if (name == _T("btn_scale_200")) { m_renderer->setRenderScale(2.0);  return; }

        // Show stats
        if (name == _T("btn_show_stats")) {
            std::string stats = m_viewModel->getInstanceStats();
            if (stats.empty()) stats = "(暂无统计数据)";
            ResultDialog dlg;
            dlg.setTitle(L"统计信息");
            dlg.setResultText(StringUtils::Utf8ToWide(stats));
            dlg.doModal(GetHWND());
            return;
        }
    }
    __super::Notify(msg);
}

// ==================== Message Handling ====================

LRESULT StreamingWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    // Handle TcrSdk custom messages
    if (uMsg == WM_TCR_VIDEO_FRAME && m_viewModel) {
        m_viewModel->handleVideoFrameMessage(wParam, lParam);
        return 0;
    }

    if (uMsg == WM_TCR_SESSION_EVENT && m_viewModel) {
        m_viewModel->handleSessionEventMessage(wParam, lParam);
        return 0;
    }

    if (uMsg == WM_UI_CALLBACK) {
        auto* func = reinterpret_cast<std::function<void()>*>(lParam);
        if (func) {
            (*func)();
            delete func;
        }
        return 0;
    }

    // Resize video child window when parent is resized
    if (uMsg == WM_SIZE && m_videoHwnd) {
        syncVideoWindowPos();
    }

    // Cleanup on close
    if (uMsg == WM_CLOSE) {
        if (m_viewModel) {
            m_viewModel->closeSession();
        }
    }

    return __super::HandleMessage(uMsg, wParam, lParam);
}
