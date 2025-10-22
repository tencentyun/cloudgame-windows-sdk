#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QJsonObject>

/**
 * @brief 网络服务类
 * 
 * 负责处理HTTP请求，封装底层网络通信逻辑。
 * 提供统一的POST请求接口，供上层业务服务调用。
 */
class NetworkService : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param parent 父对象指针
     */
    explicit NetworkService(QObject *parent = nullptr);
    
    /**
     * @brief 发送POST请求
     * 
     * 向指定的API端点发送POST请求，请求体为JSON格式。
     * 
     * @param endpoint API端点路径（相对于baseUrl）
     * @param data 请求数据（JSON对象）
     * @return QNetworkReply* 网络回复对象指针，调用方需负责处理响应和释放资源
     */
    QNetworkReply* postRequest(const QString& endpoint, const QJsonObject& data);
    
signals:
    /**
     * @brief 请求完成信号
     * @param responseData 响应数据（原始字节数组）
     */
    void requestCompleted(const QByteArray& responseData);
    
    /**
     * @brief 请求失败信号
     * @param errorString 错误信息描述
     */
    void requestFailed(const QString& errorString);
    
private:
    QNetworkAccessManager m_manager;  ///< Qt网络访问管理器
    
    /**
     * @brief 业务后台基础URL
     * 
     * 云渲染开发团队搭建的体验后台地址，用于Demo开发和测试。
     * 生产环境需要替换为正式的业务后台地址。
     */
    QString m_baseUrl = "https://test-accelerator-biz-server.cai.crtrcloud.com";
};

