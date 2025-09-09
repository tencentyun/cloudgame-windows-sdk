#pragma once

#include <QObject>
#include <QStringList>
#include "../services/ApiService.h"

/**
 * @brief 实例访问视图模型，处理实例访问令牌创建
 * @note 正常情况您应该是从服务端获取到云手机实例ID，在演示工程中您是通过输入到qml界面来获取实例ID的
 */
class InstanceTokenViewModel : public QObject {
    Q_OBJECT
    Q_PROPERTY(bool isBusy READ isBusy NOTIFY isBusyChanged)
    
public:
    explicit InstanceTokenViewModel(ApiService* apiService, QObject *parent = nullptr);
    
    Q_INVOKABLE void createAccessToken(const QString& instanceIds, const QString& userIp);
    
    bool isBusy() const { return m_isBusy; }
    
signals:
    void accessTokenCreated(const QString& accessInfo, const QString& token);
    void errorOccurred(const QString& errorMessage);
    void isBusyChanged();
    
private slots:
    void onAccessTokenCreated(const QString& accessInfo, const QString& token);
    void onApiError(const QString& errorCode, const QString& message);
    
private:
    ApiService* m_apiService;
    bool m_isBusy = false;
};