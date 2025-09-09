#pragma once

#include <QObject>
#include "services/ApiService.h"

/**
 * @brief 登录视图模型类，封装登录逻辑
 */
class LoginViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    
public:
    explicit LoginViewModel(ApiService* apiService, QObject* parent = nullptr);
    
    bool isBusy() const;
    
    Q_INVOKABLE void login(const QString& userId, const QString& password);
    
signals:
    void loginSuccess(const QString& userType);
    void authFailed(const QString& error);
    void isBusyChanged();
    
private:
    ApiService* m_apiService;
    bool m_isBusy = false;
    
    void setIsBusy(bool busy);
};