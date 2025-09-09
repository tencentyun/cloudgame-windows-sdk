#pragma once

#include "NetworkService.h"
#include <QObject>
#include <QJsonObject>
#include <QJsonArray>
#include <QStringList>
#include <functional>
#include "viewmodels/AndroidInstanceModel.h"

// 前置声明
class QNetworkReply;

/**
 * @brief API服务类，负责处理与云手机API的交互
 * 
 * 该类封装了所有与云手机API的交互逻辑，包括登录、查询实例、连接实例等功能。
 * 使用信号机制通知调用方API调用的结果。
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
    
    /**
     * @brief 用户登录
     * @param userId 用户ID
     * @param password 密码
     */
    Q_INVOKABLE void login(const QString& userId, const QString& password);
    
    /**
     * @brief 查询安卓实例列表
     * @param offset 查询偏移量
     * @param limit 查询数量限制
     * @param instanceIds 实例ID列表（可选）
     * @param region 实例区域（可选）
     */
    void describeAndroidInstances(int offset, int limit, 
                                 const QStringList& instanceIds = QStringList(),
                                 const QString& region = QString());
    
    /**
     * @brief 创建安卓实例访问令牌
     * @param androidInstanceIds 安卓实例ID列表
     */
    void createAndroidInstancesAccessToken(const QStringList& androidInstanceIds);
    
    /**
     * @brief 连接安卓实例组
     * @param androidInstanceIds 安卓实例ID列表
     * @param clientSessions 客户端会话列表
     */
    void connectAndroidGroupInstances(const QStringList& androidInstanceIds, 
                                     const QStringList& clientSessions);
    
    /**
     * @brief 连接单个安卓实例
     * @param androidInstanceId 安卓实例ID
     * @param clientSession 客户端会话
     */
    void connectAndroidInstance(const QString& androidInstanceId, 
                               const QString& clientSession);
    
    /**
     * @brief 解析JSON数组为AndroidInstance列表
     * @param instances JSON数组
     * @return QList<AndroidInstance>
     */
    QList<AndroidInstance> parseAndroidInstances(const QJsonArray& instances);
    
signals:
    /// @brief 登录成功信号
    void loginSuccess(const QString& userType);
    
    /// @brief 鉴权失败信号
    void authFailed(const QString& error);
    
    /// @brief 收到实例列表信号
    void instancesReceived(const QList<AndroidInstance>& instances, int totalCount);
    
    /// @brief 组连接成功信号
    void groupConnected(const QStringList& serverSessions);
    
    /// @brief 单实例连接成功信号
    void instanceConnected(const QString& serverSession);
    
    /// @brief 安卓实例访问令牌创建成功信号
    void androidInstancesAccessTokenCreated(const QString& accessInfo, const QString& token);
    
    /// @brief API错误信号
    void apiError(const QString& errorCode, const QString& message);
    
    
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
