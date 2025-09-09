#include "LoginViewModel.h"
#include "../utils/Logger.h"

LoginViewModel::LoginViewModel(ApiService* apiService, QObject* parent)
    : QObject(parent), m_apiService(apiService) {
    Logger::info("LoginViewModel initialized");
}

bool LoginViewModel::isBusy() const {
    return m_isBusy;
}

void LoginViewModel::setIsBusy(bool busy) {
    if (m_isBusy != busy) {
        m_isBusy = busy;
        emit isBusyChanged();
    }
}

void LoginViewModel::login(const QString& userId, const QString& password) {
    setIsBusy(true);
    
    // 连接ApiService的信号
    connect(m_apiService, &ApiService::loginSuccess, this, [this](const QString& userType) {
        setIsBusy(false);
        emit loginSuccess(userType);
        disconnect(m_apiService, nullptr, this, nullptr);
    });
    
    connect(m_apiService, &ApiService::authFailed, this, [this](const QString& error) {
        setIsBusy(false);
        emit authFailed(error);
        disconnect(m_apiService, nullptr, this, nullptr);
    });
    
    // 调用ApiService的登录方法
    m_apiService->login(userId, password);
}