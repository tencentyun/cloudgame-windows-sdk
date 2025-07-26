#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>

/**
 * @brief 网络服务类，负责处理HTTP请求
 */
class NetworkService : public QObject {
    Q_OBJECT
public:
    explicit NetworkService(QObject *parent = nullptr);
    
    /**
     * @brief 发送POST请求
     * @param endpoint API端点
     * @param data 请求数据
     * @return 网络回复对象
     */
    QNetworkReply* postRequest(const QString& endpoint, const QJsonObject& data);
    
    /**
     * @brief 设置授权令牌
     * @param token 令牌字符串
     */
    void setToken(const QByteArray& token);
    
    /**
     * @brief 设置环境（测试或生产）
     * @param isProduction 是否为生产环境
     */
    void setEnvironment(bool isProduction);
    
    /**
     * @brief 获取当前令牌
     * @return 令牌字符串
     */
    QByteArray getToken() const;
    
signals:
    /**
     * @brief 请求完成信号
     * @param responseData 响应数据
     */
    void requestCompleted(const QByteArray &responseData);
    
    /**
     * @brief 请求失败信号
     * @param errorString 错误信息
     */
    void requestFailed(const QString &errorString);
    
private:
    QNetworkAccessManager m_manager;
    QByteArray m_token;
    QString m_baseUrl; // 基础URL
};
