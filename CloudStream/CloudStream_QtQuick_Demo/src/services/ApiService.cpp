#include "ApiService.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QUuid>
#include <functional>

// ============================================================================
// 构造函数
// ============================================================================

ApiService::ApiService(NetworkService* networkService, QObject *parent)
    : QObject(parent)
    , m_networkService(networkService) {
    Logger::info("API service initialized");
}

// ============================================================================
// 私有辅助方法实现
// ============================================================================

/**
 * @brief 统一请求处理模板实现
 * 
 * 此模板方法封装了API请求的通用流程：
 * 1. 发送网络请求
 * 2. 等待响应
 * 3. 处理公共响应逻辑
 * 4. 调用业务特定的成功处理函数
 */
template<typename SuccessHandler>
void ApiService::sendRequest(
    const QString& path, 
    const QJsonObject& data, 
    SuccessHandler onSuccess,
    bool handleHeaders) {
    
    Logger::info(QString("API request sent: %1").arg(path));
    
    // 发送POST请求
    QNetworkReply* reply = m_networkService->postRequest(path, data);
    
    // 连接响应完成信号
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess, handleHeaders]() {
        // 处理公共响应逻辑
        processCommonResponse(reply, [this, reply, onSuccess, handleHeaders](const QJsonObject& response) {
            // 调用具体业务的成功处理函数
            onSuccess(response);
        });
        
        // 清理网络回复对象
        reply->deleteLater();
    });
}

/**
 * @brief 公共响应处理实现
 * 
 * 处理所有API响应的通用逻辑，包括：
 * - 网络层错误检查
 * - JSON解析
 * - 业务层错误检查
 * - 成功回调调用
 */
void ApiService::processCommonResponse(
    QNetworkReply* reply, 
    const std::function<void(const QJsonObject&)>& onSuccess) {
    
    // 步骤1：检查网络层错误
    if (reply->error() != QNetworkReply::NoError) {
        Logger::error(QString("Network error: %1").arg(reply->errorString()));
        emit apiError("NetworkError", reply->errorString());
        return;
    }
    
    // 步骤2：读取并解析响应数据
    QByteArray responseData = reply->readAll();
    Logger::debug(QString("API response: %1").arg(QString::fromUtf8(responseData)));
    
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {
        Logger::error("Invalid JSON response");
        emit apiError("InvalidResponse", "Invalid JSON format");
        return;
    }
    
    QJsonObject root = doc.object();
    
    // 步骤3：检查业务层错误
    // 业务后台返回的错误格式：{"Error": {"Code": "...", "Message": "..."}, "RequestId": "..."}
    if (root.contains("Error")) {
        QJsonObject error = root["Error"].toObject();
        QString requestId = root["RequestId"].toString();
        QString errorCode = error["Code"].toString();
        QString errorMsg = error["Message"].toString();
        
        QString logMsg = QString("API error: code=%1, message=%2, requestId=%3")
                            .arg(errorCode, errorMsg, requestId);
        Logger::error(logMsg);
        
        emit apiError(errorCode, errorMsg);
        return;
    }
    
    // 步骤4：调用成功处理回调
    onSuccess(root);
}

// ============================================================================
// 业务API接口实现
// ============================================================================

/**
 * @brief 创建Android实例访问令牌
 * 
 * 实现CreateAndroidInstancesAccessToken API调用。
 * API文档：https://cloud.tencent.com/document/api/1162/119708
 * 
 * 请求参数：
 * - AndroidInstanceIds: Android实例ID数组
 * - UserIp: 用户IP地址（可选）
 * 
 * 响应参数：
 * - AccessInfo: 访问信息（包含连接地址等）
 * - Token: 访问令牌
 * - RequestId: 请求ID（回显）
 */
void ApiService::createAndroidInstancesAccessToken(
    const QStringList& androidInstanceIds, 
    const QString& userIp) {
    
    // 构建请求数据
    QJsonObject data;
    
    // 生成唯一的请求ID
    data["RequestId"] = QUuid::createUuid().toString();
    
    // 添加用户IP（如果提供）
    if (!userIp.isEmpty()) {
        data["UserIp"] = userIp;
    }
    
    // 转换实例ID列表为JSON数组
    QJsonArray instanceIdsArray;
    for (const QString& id : androidInstanceIds) {
        instanceIdsArray.append(id);
    }
    data["AndroidInstanceIds"] = instanceIdsArray;
    
    // 发送请求并处理响应
    sendRequest(
        "/CreateAndroidInstancesAccessToken", 
        data, 
        [this](const QJsonObject& response) {
            // 解析响应数据
            QString accessInfo = response["AccessInfo"].toString();
            QString token = response["Token"].toString();
            QString requestId = response["RequestId"].toString();
            
            // 记录成功日志
            Logger::info(
                QString("CreateAndroidInstancesAccessToken success, accessInfo: %1, token: %2")
                    .arg(accessInfo, token)
            );
            
            // 发出成功信号
            emit androidInstancesAccessTokenCreated(accessInfo, token);
        }, 
        false  // 不需要处理响应头
    );
}
