#include "ApiService.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QUuid>
#include <functional>

ApiService::ApiService(NetworkService* networkService, QObject *parent)
    : QObject(parent), m_networkService(networkService) {
    Logger::info("API service initialized");
}

// 统一请求处理模板实现
template<typename SuccessHandler>
void ApiService::sendRequest(const QString& path, 
                            const QJsonObject& data, 
                            SuccessHandler onSuccess,
                            bool handleHeaders) {
    Logger::info(QString("API request sent: %1").arg(path));
    
    QNetworkReply* reply = m_networkService->postRequest(path, data);
    connect(reply, &QNetworkReply::finished, this, [this, reply, onSuccess, handleHeaders]() {
        // 处理公共响应逻辑
        processCommonResponse(reply, [this, reply, onSuccess, handleHeaders](const QJsonObject& response) {
            // 调用具体成功处理函数
            onSuccess(response);
        });
        reply->deleteLater();
    });
}

// 公共响应处理实现
void ApiService::processCommonResponse(QNetworkReply* reply, 
                                      const std::function<void(const QJsonObject&)>& onSuccess) {
    if (reply->error() != QNetworkReply::NoError) {
        Logger::error(QString("Network error: %1").arg(reply->errorString()));
        emit apiError("NetworkError", reply->errorString());
        return;
    }
    
    QByteArray responseData = reply->readAll();
    Logger::debug(QString("API response: %1").arg(QString::fromUtf8(responseData)));
    
    QJsonDocument doc = QJsonDocument::fromJson(responseData);
    if (doc.isNull()) {
        Logger::error("Invalid JSON response");
        emit apiError("InvalidResponse", "Invalid JSON format");
        return;
    }
    
    QJsonObject root = doc.object();
    
    // 检查API错误
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
    
    // 调用成功处理回调
    onSuccess(root);
}

void ApiService::createAndroidInstancesAccessToken(const QStringList& androidInstanceIds, const QString& userIp) {
    QJsonObject data;
    data["RequestId"] = QUuid::createUuid().toString();
    if (!userIp.isEmpty()) {
        data["UserIp"] = userIp;
    }
    
    QJsonArray instanceIdsArray;
    for (const QString& id : androidInstanceIds) {
        instanceIdsArray.append(id);
    }
    data["AndroidInstanceIds"] = instanceIdsArray;
    
    sendRequest("/CreateAndroidInstancesAccessToken", data, [this](const QJsonObject& response) {
        QString accessInfo = response["AccessInfo"].toString();
        QString token = response["Token"].toString();
        QString requestId = response["RequestId"].toString();
        
        Logger::info(QString("CreateAndroidInstancesAccessToken success, accessInfo: %1, token: %2")
                    .arg(accessInfo, token));
        emit androidInstancesAccessTokenCreated(accessInfo, token);
    }, false);
}
