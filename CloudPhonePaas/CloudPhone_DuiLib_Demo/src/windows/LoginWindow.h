#pragma once

#include <functional>
#include <string>

#include <UIlib.h>

using namespace DuiLib;

class ApiService;

class LoginWindow : public WindowImplBase {
public:
    explicit LoginWindow(ApiService* apiService);
    ~LoginWindow() override = default;

    LPCTSTR GetWindowClassName() const override;
    CDuiString GetSkinFile() override;
    CDuiString GetSkinFolder() override;

    void InitWindow() override;
    void Notify(TNotifyUI& msg) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // Called by main.cpp after ShowModal to check if login succeeded
    bool loginSucceeded() const { return m_loginSucceeded; }

private:
    ApiService* m_apiService;
    bool m_isBusy = false;
    bool m_loginSucceeded = false;

    CEditUI* m_editUserId = nullptr;
    CEditUI* m_editPassword = nullptr;
    CButtonUI* m_btnLogin = nullptr;
    CLabelUI* m_lblStatus = nullptr;

    void doLogin();
    void setLoginEnabled(bool enabled);
    void onLoginResult(bool success, const std::string& message);
};
