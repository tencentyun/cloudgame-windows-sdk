#pragma once

#include "NetworkService.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>

class QNetworkReply;

/**
 * @brief API服务类
 * 
 * 封装业务后台API调用逻辑，提供高层业务接口。
 * 负责构造API请求参数、处理响应数据、统一错误处理等。
 */
class ApiService : public QObject {
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * @param networkService 网络服务实例指针（依赖注入）
     * @param parent 父对象指针
     */
    explicit ApiService(NetworkService* networkService, QObject *parent = nullptr);
    
    // ========================================================================
    // 业务API接口
    // ========================================================================
    
    /**
     * @brief 创建Android实例访问令牌
     * 
     * 调用业务后台的CreateAndroidInstancesAccessToken接口，
     * 为指定的Android实例创建访问令牌，用于后续的云渲染连接。
     * 
     * API文档：https://cloud.tencent.com/document/api/1162/119708
     * 
     * @param androidInstanceIds Android实例ID列表
     * @param userIp 用户IP地址（可选，用于模拟用户位置）
     * 
     * @note 调用成功后会发出androidInstancesAccessTokenCreated信号
     * @note 调用失败会发出apiError信号
     */
    Q_INVOKABLE void createAndroidInstancesAccessToken(
        const QStringList& androidInstanceIds, 
        const QString& userIp
    );

signals:
    /**
     * @brief API错误信号
     * 
     * 当API调用失败时发出此信号，包含错误码和错误描述。
     * 
     * @param errorCode 错误代码（如：NetworkError、InvalidResponse等）
     * @param message 错误描述信息
     */
    void apiError(const QString& errorCode, const QString& message);

    /**
     * @brief 创建AccessToken成功信号
     * 
     * 当CreateAndroidInstancesAccessToken接口调用成功时发出。
     * 
     * @param accessInfo 访问信息（包含连接地址等）
     * @param token 访问令牌
     */
    void androidInstancesAccessTokenCreated(
        const QString& accessInfo, 
        const QString& token
    );

private:
    // ========================================================================
    // 私有成员变量
    // ========================================================================
    
    NetworkService* m_networkService;  ///< 网络服务实例（不拥有所有权）
    
    // ========================================================================
    // 私有辅助方法
    // ========================================================================
    
    /**
     * @brief 统一请求处理模板
     * 
     * 封装通用的请求发送和响应处理逻辑，减少代码重复。
     * 
     * @tparam SuccessHandler 成功回调函数类型
     * @param path API路径
     * @param data 请求数据（JSON对象）
     * @param onSuccess 成功回调函数
     * @param handleHeaders 是否需要处理响应头（默认false）
     */
    template<typename SuccessHandler>
    void sendRequest(
        const QString& path, 
        const QJsonObject& data, 
        SuccessHandler onSuccess,
        bool handleHeaders = false
    );
    
    /**
     * @brief 公共响应处理
     * 
     * 处理API响应的通用逻辑：
     * - 检查网络错误
     * - 解析JSON响应
     * - 检查业务错误
     * - 调用成功回调
     * 
     * @param reply 网络回复对象
     * @param onSuccess 成功回调函数（接收解析后的JSON对象）
     */
    void processCommonResponse(
        QNetworkReply* reply, 
        const std::function<void(const QJsonObject&)>& onSuccess
    );
};
