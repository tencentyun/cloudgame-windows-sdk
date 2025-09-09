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
    QString m_baseUrl = "https://test-xxxx-server.cloud-device.crtrcloud.com";
};

