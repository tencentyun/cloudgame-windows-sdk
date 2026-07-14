#include "LoginWindow.h"

#include "services/ApiService.h"
#include "utils/Logger.h"
#include "utils/StringUtils.h"
#include "utils/UiThreadHelper.h"

LoginWindow::LoginWindow(ApiService* apiService) : m_apiService(apiService) {}

LPCTSTR LoginWindow::GetWindowClassName() const { return _T("LoginWindowClass"); }

CDuiString LoginWindow::GetSkinFile() { return _T("login.xml"); }

CDuiString LoginWindow::GetSkinFolder() { return _T("skin"); }

void LoginWindow::InitWindow() {
    m_editUserId = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_userid")));
    m_editPassword = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_password")));
    m_btnLogin = static_cast<CButtonUI*>(m_PaintManager.FindControl(_T("btn_login")));
    m_lblStatus = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_status")));
}

void LoginWindow::Notify(TNotifyUI& msg) {
    if (msg.sType == DUI_MSGTYPE_CLICK) {
        CDuiString name = msg.pSender->GetName();
        if (name == _T("btn_close")) {
            Close(IDCANCEL);
            return;
        }
        if (name == _T("btn_login")) {
            doLogin();
            return;
        }
    }
    // Handle Enter key in edit controls to trigger login
    if (msg.sType == DUI_MSGTYPE_RETURN) {
        doLogin();
        return;
    }
    __super::Notify(msg);
}

LRESULT LoginWindow::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    if (uMsg == WM_UI_CALLBACK) {
        auto* func = reinterpret_cast<std::function<void()>*>(lParam);
        if (func) {
            (*func)();
            delete func;
        }
        return 0;
    }
    return __super::HandleMessage(uMsg, wParam, lParam);
}

void LoginWindow::setLoginEnabled(bool enabled) {
    m_isBusy = !enabled;
    if (m_btnLogin) {
        m_btnLogin->SetEnabled(enabled);
        m_btnLogin->SetText(enabled ? _T("登录") : _T("登录中..."));
    }
}

void LoginWindow::doLogin() {
    if (m_isBusy)
        return;

    if (!m_editUserId || !m_editPassword)
        return;

    CDuiString userId = m_editUserId->GetText();
    CDuiString password = m_editPassword->GetText();

    if (userId.IsEmpty() || password.IsEmpty()) {
        if (m_lblStatus)
            m_lblStatus->SetText(_T("请输入账号和密码"));
        return;
    }

    // Clear previous error
    if (m_lblStatus)
        m_lblStatus->SetText(_T(""));

    setLoginEnabled(false);

    std::string userIdUtf8 = StringUtils::WideToUtf8(std::wstring(userId.GetData()));
    std::string passwordUtf8 = StringUtils::WideToUtf8(std::wstring(password.GetData()));

    HWND hwnd = GetHWND();
    m_apiService->login(userIdUtf8, passwordUtf8, [hwnd](bool success, const std::string& message) {
        // Post result back to UI thread
        UiThreadHelper::postToUiThread(hwnd, [hwnd, success, message]() {
            // Find the LoginWindow instance from HWND
            // WindowImplBase stores 'this' as window user data
            LoginWindow* self =
                reinterpret_cast<LoginWindow*>(::GetWindowLongPtr(hwnd, GWLP_USERDATA));
            if (self) {
                self->onLoginResult(success, message);
            }
        });
    });
}

void LoginWindow::onLoginResult(bool success, const std::string& message) {
    setLoginEnabled(true);

    if (success) {
        Logger::info("Login success, user type: " + message);
        m_loginSucceeded = true;
        Close(IDOK);
    } else {
        Logger::error("Login failed: " + message);
        if (m_lblStatus) {
            std::wstring errorMsg = L"登录失败: " + StringUtils::Utf8ToWide(message);
            m_lblStatus->SetText(errorMsg.c_str());
        }
    }
}
