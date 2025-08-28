#pragma once

#include "NetworkService.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>

class QNetworkReply;

/**
 * @brief API服务类，负责调用业务后台API
 */
class ApiService : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param networkService 网络服务实例
     * @param parent 父对象
     */
    explicit ApiService(NetworkService* networkService, QObject *parent = nullptr);
    
    // 创建AccessToken
    Q_INVOKABLE void createAndroidInstancesAccessToken(const QStringList& androidInstanceIds, const QString& userIp);

signals:
    /// @brief API错误信号
    void apiError(const QString& errorCode, const QString& message);

    /// @brief 创建AccessToken成功信号
    void androidInstancesAccessTokenCreated(const QString& accessInfo, const QString& token);

private:
    NetworkService* m_networkService; ///< 网络服务实例
    
    // 统一请求处理模板
    template<typename SuccessHandler>
    void sendRequest(const QString& path, 
                    const QJsonObject& data, 
                    SuccessHandler onSuccess,
                    bool handleHeaders = false);
    
    // 公共响应处理
    void processCommonResponse(QNetworkReply* reply, 
                              const std::function<void(const QJsonObject&)>& onSuccess);
};
