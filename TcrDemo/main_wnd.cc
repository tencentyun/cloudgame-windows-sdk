#include "main_wnd.h"

#include <math.h>
#include <string>
#include <thread>
#include <chrono>

#include "json/json.h"
#include "keyboard.h"
#include "mouse.h"
#include "cloud_game_api.h"
#include "log_utils.h"
#include "my_tcr_logger.h"

ATOM MainWnd::wnd_class_ = 0;
const wchar_t MainWnd::kClassName[] = L"TcrMainWnd";
const char* const MainWnd::TAG = "TcrMainWnd";

namespace {

    const char kConnecting[] = "Connecting...(no incoming video) ";

    void CalculateWindowSizeForText(HWND wnd,
        const wchar_t* text,
        size_t* width,
        size_t* height) {
        HDC dc = ::GetDC(wnd);
        RECT text_rc = { 0 };
        ::DrawTextW(dc, text, -1, &text_rc, DT_CALCRECT | DT_SINGLELINE);
        ::ReleaseDC(wnd, dc);
        RECT client, window;
        ::GetClientRect(wnd, &client);
        ::GetWindowRect(wnd, &window);

        *width = text_rc.right - text_rc.left;
        *width += (window.right - window.left) - (client.right - client.left);
        *height = text_rc.bottom - text_rc.top;
        *height += (window.bottom - window.top) - (client.bottom - client.top);
    }

    HFONT GetDefaultFont() {
        static HFONT font = reinterpret_cast<HFONT>(GetStockObject(DEFAULT_GUI_FONT));
        return font;
    }

    std::string GetWindowText(HWND wnd) {
        char text[MAX_PATH] = { 0 };
        ::GetWindowTextA(wnd, &text[0], ARRAYSIZE(text));
        return text;
    }

}  // namespace

MainWnd::MainWnd()
    : ui_(UI::CONNECT_TO_SERVER),
    wnd_(NULL),
    edit1_(NULL),
    label1_(NULL),
    button_(NULL),
    destroyed_(false),
    nested_msg_(NULL) {
}

MainWnd::~MainWnd() {

}

bool MainWnd::Create() {
    if (wnd_ != nullptr) {
        return false;
    }
    if (!RegisterWindowClass())
        return false;

    ui_thread_id_ = ::GetCurrentThreadId();
    wnd_ =
        ::CreateWindowExW(WS_EX_OVERLAPPEDWINDOW, kClassName, L"TcrDemo",
            WS_OVERLAPPEDWINDOW | WS_VISIBLE | WS_CLIPCHILDREN,
            CW_USEDEFAULT, CW_USEDEFAULT, CW_USEDEFAULT,
            CW_USEDEFAULT, NULL, NULL, GetModuleHandle(NULL), this);

    ::SendMessage(wnd_, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
        TRUE);

    CreateChildWindows();
    SwitchToConnectUI();

    return wnd_ != NULL;
}

bool MainWnd::Destroy() {
    BOOL ret = FALSE;
    if (IsWindow()) {
        ret = ::DestroyWindow(wnd_);
    }

    return ret != FALSE;
}

bool MainWnd::IsWindow() {
    return wnd_ && ::IsWindow(wnd_) != FALSE;
}

bool MainWnd::PreTranslateMessage(MSG* msg) {
    bool ret = false;
    if (msg->message == WM_CHAR && msg->wParam == VK_RETURN) {
        OnButtonClick();
        ret = true;
    }
    return ret;
}

void MainWnd::SwitchToConnectUI() {
    ui_ = UI::CONNECT_TO_SERVER;
    LayoutConnectUI(true);
    ::SetFocus(edit1_);

}

void MainWnd::SwitchToStreamingUI() {
    LayoutConnectUI(false);
    ui_ = UI::STREAMING;
}

void MainWnd::MessageBox(const char* caption, const char* text, bool is_error) {
    DWORD flags = MB_OK;
    if (is_error)
        flags |= MB_ICONERROR;

    ::MessageBoxA(handle(), text, caption, flags);
}

void MainWnd::QueueUIThreadCallback(int msg_id, void* data) {
    ::PostThreadMessage(ui_thread_id_, WM_APP + 1,
        static_cast<WPARAM>(msg_id),
        reinterpret_cast<LPARAM>(data));
}

void MainWnd::OnPaint() {
    PAINTSTRUCT ps;
    HDC hdc = ::BeginPaint(handle(), &ps);

    RECT rc;
    ::GetClientRect(handle(), &rc);
    if (ui_ == UI::STREAMING && !is_connected_) {
        PAINTSTRUCT ps;
        HDC hdc = ::BeginPaint(handle(), &ps);

        RECT rc;
        ::GetClientRect(handle(), &rc);

        // We're still waiting for the video stream to be initialized.
        HBRUSH brush = ::CreateSolidBrush(RGB(0, 0, 0));
        ::FillRect(ps.hdc, &rc, brush);
        ::DeleteObject(brush);

        HGDIOBJ old_font = ::SelectObject(ps.hdc, GetDefaultFont());
        ::SetTextColor(ps.hdc, RGB(0xff, 0xff, 0xff));
        ::SetBkMode(ps.hdc, TRANSPARENT);

        std::string text(kConnecting);

        ::DrawTextA(ps.hdc, text.c_str(), -1, &rc,
            DT_SINGLELINE | DT_CENTER | DT_VCENTER);
        ::SelectObject(ps.hdc, old_font);
        
    }
    else {
        HBRUSH brush = ::CreateSolidBrush(::GetSysColor(COLOR_WINDOW));
        ::FillRect(ps.hdc, &rc, brush);
        ::DeleteObject(brush);
    }

    ::EndPaint(handle(), &ps);
}

void MainWnd::OnDestroyed() {
    PostQuitMessage(0);
}

void MainWnd::OnButtonClick() {
    if (ui_ == UI::CONNECT_TO_SERVER) {
        exprience_code_ = GetWindowText(edit1_);

        // Create and set TcrLogger for custom printing of internal logs within TCRSDK.
        logger_ = std::make_shared<MyTcrLogger>();
        tcrsdk::LogUtils::SetLogger(logger_);

        // Create and initialize TcrSdk
        tcr_session_ = std::make_unique<tcrsdk::TcrSession>(shared_from_this());
        tcr_session_->Init();

        SwitchToStreamingUI();
    }
}

bool MainWnd::OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result) {
    switch (msg) {
    case WM_CREATE:
        SetFocus(wnd_);
        break;
    case WM_ACTIVATE:
        if (LOWORD(wp) != WA_INACTIVE)
        {
            SetFocus(wnd_);
        }
        break;
    case WM_ERASEBKGND:
        *result = TRUE;
        return true;

    case WM_PAINT:
        OnPaint();
        return true;

    case WM_SETFOCUS:
        if (ui_ == UI::CONNECT_TO_SERVER) {
            SetFocus(edit1_);
        }
        return true;

    case WM_SIZE: {
        if (ui_ == UI::CONNECT_TO_SERVER) {
            LayoutConnectUI(true);
        }

        if (remote_renderer_) {
            int width = LOWORD(lp);
            int height = HIWORD(lp);
            remote_renderer_->OnSurfaceChanged(width, height);
        }
        break;
    }
    case WM_CTLCOLORSTATIC:
        *result = reinterpret_cast<LRESULT>(GetSysColorBrush(COLOR_WINDOW));
        return true;

    case WM_COMMAND:
        if (button_ == reinterpret_cast<HWND>(lp)) {
            if (BN_CLICKED == HIWORD(wp))
                OnButtonClick();
            SetFocus(wnd_);
        }
        return true;
    case WM_LBUTTONDOWN:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::LEFT, true);
        }
        break;
    case WM_LBUTTONUP:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::LEFT, false);
        }
        break;
    case WM_RBUTTONDOWN:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::RIGHT, true);
        }
        break;
    case WM_RBUTTONUP:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::RIGHT, false);
        }
        break;
    case WM_MBUTTONDOWN:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::MIDDLE, true);
        }
        break;
    case WM_MBUTTONUP:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::MIDDLE, false);
        }
        break;
    case WM_XBUTTONDOWN:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::FORWARD, true);
        }
        break;
    case WM_XBUTTONUP:
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseKey(tcrsdk::Mouse::MouseKeyType::FORWARD, false);
        }
        break;
    case WM_MOUSEMOVE: {
        int x_pos = LOWORD(lp);
        int y_pos = HIWORD(lp);
        tcrsdk::LogUtils::t(TAG, ("x = " + std::to_string(x_pos) + ", y = " + std::to_string(y_pos)).c_str());
        CalculateMouselocation(x_pos, y_pos);
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseMoveTo(x_pos, y_pos);
        }
        break;
    }
    case WM_MOUSEWHEEL: {
        short zDelta = GET_WHEEL_DELTA_WPARAM(wp);
        tcrsdk::LogUtils::t(TAG, ("mouse scroll delta = " + std::to_string(zDelta)).c_str());
        if (tcr_session_) {
            tcr_session_->GetMouse()->OnMouseScroll(zDelta);
        }
        break;
    }
    case WM_KEYDOWN: {
        int keyCode = static_cast<int>(wp);

        if (tcr_session_) {

            if (keyCode == VK_SHIFT || keyCode == VK_CONTROL || keyCode == VK_MENU)
            {
                // Get the status of left and right Shift, Ctrl, and Alt keys
                short lShiftState = GetKeyState(VK_LSHIFT);
                short rShiftState = GetKeyState(VK_RSHIFT);
                short lCtrlState = GetKeyState(VK_LCONTROL);
                short rCtrlState = GetKeyState(VK_RCONTROL);
                short lAltState = GetKeyState(VK_LMENU);
                short rAltState = GetKeyState(VK_RMENU);

                // Check if the pressed Shift key is on the left or the right side
                if (keyCode == VK_SHIFT)
                {
                    if (lShiftState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, true);
                    }
                    else if (rShiftState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, false);
                    }
                }

                // �Check if the pressed Ctrl key is on the left or the right side
                if (keyCode == VK_CONTROL)
                {
                    if (lCtrlState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, true);
                    }
                    else if (rCtrlState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, false);
                    }
                }

                // Check if the pressed Alt key is on the left or the right side
                if (keyCode == VK_MENU)
                {
                    if (lAltState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, true);
                    }
                    else if (rAltState & 0x8000)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true, false);
                    }
                }
            }
            else {
                tcrsdk::LogUtils::t(TAG, ("KEYDOWN keycode = " + std::to_string(keyCode)).c_str());
                tcr_session_->GetKeyboard()->OnKeyboard(keyCode, true);
            }

        }
        break;
    }
    case WM_KEYUP: {
        int keyCode = static_cast<int>(wp);
        if (tcr_session_) {
            if (keyCode == VK_SHIFT || keyCode == VK_CONTROL || keyCode == VK_MENU) {
                int scanCode = (lp & 0x00FF0000) >> 16;

                // ʹConvert scan code to virtual key code using MapVirtualKey
                int mappedKeyCode = MapVirtualKey(scanCode, MAPVK_VSC_TO_VK_EX);

                if (keyCode == VK_SHIFT)
                {
                    if (mappedKeyCode == VK_LSHIFT)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, true);
                    }
                    else if (mappedKeyCode == VK_RSHIFT)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, false);
                    }
                }
                else if (keyCode == VK_CONTROL)
                {
                    if (mappedKeyCode == VK_LCONTROL)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, true);
                    }
                    else if (mappedKeyCode == VK_RCONTROL)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, false);
                    }
                }
                else if (keyCode == VK_MENU)
                {
                    if (mappedKeyCode == VK_LMENU)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", left").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, true);
                    }
                    else if (mappedKeyCode == VK_RMENU)
                    {
                        tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode) + ", right").c_str());
                        tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false, false);
                    }
                }
            }
            else {
                tcrsdk::LogUtils::t(TAG, ("KEYUP keycode = " + std::to_string(keyCode)).c_str());
                tcr_session_->GetKeyboard()->OnKeyboard(keyCode, false);
            }
        }


        break;
    }


    }
    return false;
}

// static
LRESULT CALLBACK MainWnd::WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp) {
    MainWnd* me =
        reinterpret_cast<MainWnd*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
    if (!me && WM_CREATE == msg) {
        CREATESTRUCT* cs = reinterpret_cast<CREATESTRUCT*>(lp);
        me = reinterpret_cast<MainWnd*>(cs->lpCreateParams);
        me->wnd_ = hwnd;
        ::SetWindowLongPtr(hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(me));
    }

    LRESULT result = 0;
    if (me) {
        void* prev_nested_msg = me->nested_msg_;
        me->nested_msg_ = &msg;

        bool handled = me->OnMessage(msg, wp, lp, &result);
        if (WM_NCDESTROY == msg) {
            me->destroyed_ = true;
        }
        else if (!handled) {
            result = ::DefWindowProc(hwnd, msg, wp, lp);
        }

        if (me->destroyed_ && prev_nested_msg == NULL) {
            me->OnDestroyed();
            me->wnd_ = NULL;
            me->destroyed_ = false;
        }

        me->nested_msg_ = prev_nested_msg;
    }
    else {
        result = ::DefWindowProc(hwnd, msg, wp, lp);
    }

    return result;
}

// static
bool MainWnd::RegisterWindowClass() {
    if (wnd_class_)
        return true;

    WNDCLASSEXW wcex = { sizeof(WNDCLASSEX) };
    wcex.style = CS_DBLCLKS;
    wcex.hInstance = GetModuleHandle(NULL);
    wcex.hbrBackground = reinterpret_cast<HBRUSH>(COLOR_WINDOW + 1);
    wcex.hCursor = ::LoadCursor(NULL, IDC_ARROW);
    wcex.lpfnWndProc = &WndProc;
    wcex.lpszClassName = kClassName;
    wnd_class_ = ::RegisterClassExW(&wcex);
    return wnd_class_ != 0;
}

void MainWnd::CreateChildWindow(HWND* wnd,
    MainWnd::ChildWindowID id,
    const wchar_t* class_name,
    DWORD control_style,
    DWORD ex_style) {
    if (::IsWindow(*wnd))
        return;

    // Child windows are invisible at first, and shown after being resized.
    DWORD style = WS_CHILD | control_style;
    *wnd = ::CreateWindowExW(ex_style, class_name, L"", style, 100, 100, 100, 100,
        wnd_, reinterpret_cast<HMENU>(id),
        GetModuleHandle(NULL), NULL);
    ::SendMessage(*wnd, WM_SETFONT, reinterpret_cast<WPARAM>(GetDefaultFont()),
        TRUE);
}

void MainWnd::CreateChildWindows() {
    // Create the child windows in tab order.
    CreateChildWindow(&label1_, ChildWindowID::LABEL1_ID, L"Static", ES_CENTER | ES_READONLY, 0);
    CreateChildWindow(&edit1_, ChildWindowID::EDIT_ID, L"Edit",
        ES_LEFT | ES_NOHIDESEL | WS_TABSTOP, WS_EX_CLIENTEDGE);
    CreateChildWindow(&button_, ChildWindowID::BUTTON_ID, L"Button", BS_CENTER | WS_TABSTOP, 0);
}

void MainWnd::LayoutConnectUI(bool show) {
    struct Windows {
        HWND wnd;
        const wchar_t* text;
        size_t width;
        size_t height;
    } windows[] = {
        {label1_, L"Exprience Code"},  {edit1_, L"XXXyyyYYYgggXXXyyyYYYggg"},
        {button_, L"Connect"},
    };

    if (show) {
        const size_t kSeparator = 5;
        size_t total_width = (ARRAYSIZE(windows) - 1) * kSeparator;

        for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
            CalculateWindowSizeForText(windows[i].wnd, windows[i].text,
                &windows[i].width, &windows[i].height);
            total_width += windows[i].width;
        }

        RECT rc;
        ::GetClientRect(wnd_, &rc);
        size_t x = (rc.right / 2) - (total_width / 2);
        size_t y = rc.bottom / 2;
        for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
            size_t top = y - (windows[i].height / 2);
            ::MoveWindow(windows[i].wnd, static_cast<int>(x), static_cast<int>(top),
                static_cast<int>(windows[i].width),
                static_cast<int>(windows[i].height), TRUE);
            x += kSeparator + windows[i].width;
            if (windows[i].text[0] != 'X')
                ::SetWindowTextW(windows[i].wnd, windows[i].text);
            ::ShowWindow(windows[i].wnd, SW_SHOWNA);
        }
    }
    else {
        for (size_t i = 0; i < ARRAYSIZE(windows); ++i) {
            ::ShowWindow(windows[i].wnd, SW_HIDE);
        }
    }
}

void MainWnd::GetViewport(int32_t& width, int32_t& height) {
    HWND hWnd = GetForegroundWindow();
    RECT rect;
    GetClientRect(hWnd, &rect);
    width = rect.right - rect.left;
    height = rect.bottom - rect.top;
}

void MainWnd::CalculateMouselocation(int32_t& x_pos, int32_t& y_pos) {
    // GetWindowSize
    int32_t width, height;
    GetViewport(width, height);
    if (width > 0 && height > 0) {
        x_pos = x_pos * 8192 / width;
        y_pos = y_pos * 8192 / height;
    }
}

void MainWnd::onEvent(tcrsdk::TcrSession::Event event, const char* eventData)
{
    std::string data = std::string(eventData);
    switch (event)
    {
    case tcrsdk::TcrSession::Event::STATE_INITED: {
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_STATE_INITED: " + data).c_str());
        int32_t width, height;
        GetViewport(width, height);
        remote_renderer_ = std::make_shared<VideoRenderer>(handle(), width, height);

        tcr_session_->SetVideoFrameObserver(remote_renderer_);

        cloud_game_api_.reset(new CloudGameApi());
        std::string clientSession = data;
        cloud_game_api_->startGame(cloud_game_api_->createGameStartParam(clientSession, exprience_code_), this);
        break;
    }

    case tcrsdk::TcrSession::Event::STATE_CONNECTED:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_STATE_CONNECTED: " + data).c_str());
        is_connected_ = true;
        break;
    case tcrsdk::TcrSession::Event::STATE_CLOSED:
        MessageBox(NULL, data.c_str(), MB_OK);
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_STATE_CLOSED: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::CLIENT_STATS:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_CLIENT_STATS: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::GAME_START_COMPLETE:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_GAME_START_COMPLETE: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::ARCHIVE_LOAD_STATUS:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_ARCHIVE_LOAD_STATUS: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::ARCHIVE_SAVE_STATUS:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_ARCHIVE_SAVE_STATUS: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::INPUT_STATUS_CHANGED:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_INPUT_STATUS_CHANGED: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::REMOTE_DESKTOP_INFO:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_REMOTE_DESKTOP_INFO: " + data).c_str());
        break;
        break;
    case tcrsdk::TcrSession::Event::SCREEN_CONFIG_CHANGE:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_SCREEN_CONFIG_CHANGE: " + data).c_str());
        break;
    case tcrsdk::TcrSession::Event::CURSOR_IMAGE_INFO: {
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_CURSOR_IMAGE_INFO: " + data).c_str());
        break;
    }
    case tcrsdk::TcrSession::Event::CURSOR_STATE_CHANGE:
        tcrsdk::LogUtils::i(TAG, ("TCREVENT_CURSOR_STATE_CHANGE: " + data).c_str());
        break;
    default:
        break;
    }
}

void MainWnd::onSuccess(std::string body) {
    // Parse the response
    Json::Reader reader;
    Json::Value root;
    if (reader.parse(body, root)) {
        int32_t code = root["Code"].asInt();
        std::string serverSession = root["ServerSession"].asString();

        if (code == 0) {
            //tcrsdk::LogUtils::i(TAG, "starting connect to game";
            if (!tcr_session_->Start(serverSession.c_str())) {
                MessageBox(NULL, "Failed to launch the game.", MB_OK);
                tcrsdk::LogUtils::e(TAG, "start game failed");
            }
        }
        else {
            tcrsdk::LogUtils::w(TAG, "start game failed");
            if (code == 402) {
                MessageBox(NULL, "The experience code is invalid. Please contact Tencent Cloud Rendering to obtain a valid experience code.", MB_OK);
            }
            else if (code == 408) {
                MessageBox(NULL, "The service is busy, please try again.", MB_OK);
            }
            else {
                MessageBox(NULL, "Unknown", MB_OK);
            }
        }
    }
}

void MainWnd::onFailed(std::string msg) {
    tcrsdk::LogUtils::e(TAG, ("connect to get server session failed" + msg).c_str());
}
