#include "ApiService.h"
#include "../utils/Logger.h"
#include <QJsonDocument>
#include <QJsonArray>
#include <QNetworkReply>
#include <QNetworkCookie>
#include <QUuid>
#include <functional>

/*
 * 注意：本类中的API接口仅为演示目的，实际接入时需要业务方自行实现：
 * 1. 登录接口需要业务方根据自身情况实现，业务后台的接入流程参考：
 *    https://cloud.tencent.com/document/product/1162/113726
 * 2. CreateAndroidInstancesAccessToken 需要业务方在后台实现，参考文档：
 *    https://cloud.tencent.com/document/api/1162/119708
 * 3. ConnectAndroidInstance 和 ConnectAndroidGroupInstances 需要业务方在后台实现，参考文档：
 *    https://cloud.tencent.com/document/api/1162/117268
 */
ApiService::ApiService(NetworkService* networkService, QObject *parent)
    : QObject(parent), m_networkService(networkService) {
    Logger::info("API service initialized");
}
QList<AndroidInstance> ApiService::parseAndroidInstances(const QJsonArray& instances) {
    QList<AndroidInstance> result;
    for (const auto& instanceValue : instances) {
        QJsonObject obj = instanceValue.toObject();
        AndroidInstance instance;
        instance.AndroidInstanceId = obj["AndroidInstanceId"].toString();
        instance.AndroidInstanceRegion = obj["AndroidInstanceRegion"].toString();
        instance.Name = obj["Name"].toString();
        instance.State = obj["State"].toString();
        instance.UserId = obj["UserId"].toString();
        result.append(instance);
    }
    return result;
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
            // 处理需要响应头的情况（如登录）
            if (handleHeaders) {
                // 从响应头获取token
                QVariant cookieHeader = reply->header(QNetworkRequest::SetCookieHeader);
                if (cookieHeader.isValid()) {
                    QList<QNetworkCookie> cookies = qvariant_cast<QList<QNetworkCookie>>(cookieHeader);
                    for (const QNetworkCookie& cookie : cookies) {
                        if (cookie.name() == "authorization") {
                            m_networkService->setToken(cookie.value());
                            Logger::info("Authorization token obtained");
                            break;
                        }
                    }
                }
            }
            
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
        
        // 特殊处理登录错误
        if (errorCode == "AuthenticationFailed") {
            emit authFailed(errorMsg);
        } else {
            emit apiError(errorCode, errorMsg);
        }
        return;
    }
    
    // 调用成功处理回调
    onSuccess(root["Response"].toObject());
}

void ApiService::login(const QString& userId, const QString& password) {
    QJsonObject data;
    data["RequestId"] = QUuid::createUuid().toString();
    data["UserId"] = userId;
    data["Password"] = password;
    
    // 使用统一请求处理，并指定需要处理响应头
    sendRequest("/Login", data, [this](const QJsonObject& response) {
        QString userType = response["UserType"].toString();
        Logger::info(QString("Login success, user type: %1").arg(userType));
        emit loginSuccess(userType);
    }, true); // true表示需要处理响应头
}

void ApiService::describeAndroidInstances(int offset, int limit, 
                                        const QStringList& instanceIds,
                                        const QString& region) {
    QJsonObject data;
    data["Offset"] = offset;
    data["Limit"] = limit;
    data["AndroidInstanceZone"] = "ap-hangzhou-ec-1";;
    
    if (!instanceIds.isEmpty()) {
        QJsonArray idsArray;
        for (const auto& id : instanceIds) {
            idsArray.append(id);
        }
        data["AndroidInstanceIds"] = idsArray;
    }
    
    if (!region.isEmpty()) {
        data["AndroidInstanceRegion"] = region;
    }
    
    sendRequest("/DescribeAndroidInstances", data, [this](const QJsonObject& response) {
        int totalCount = response["TotalCount"].toInt();
        QJsonArray instances = response["AndroidInstances"].toArray();
        emit instancesReceived(parseAndroidInstances(instances), totalCount);
    });
}

void ApiService::connectAndroidGroupInstances(const QStringList& instanceIds, 
                                            const QStringList& clientSessions) {
    QJsonObject data;
    QJsonArray idsArray, sessionsArray;
    
    for (const auto& id : instanceIds) {
        idsArray.append(id);
    }
    for (const auto& session : clientSessions) {
        sessionsArray.append(session);
    }
    
    data["AndroidInstanceIds"] = idsArray;
    data["ClientSessions"] = sessionsArray;
    
    sendRequest("/ConnectAndroidGroupInstances", data, [this](const QJsonObject& response) {
        QJsonArray sessions = response["ServerSessions"].toArray();
        QStringList serverSessions;
        for (const auto& session : sessions) {
            serverSessions.append(session.toString());
        }
        emit groupConnected(serverSessions);
    });
}

void ApiService::connectAndroidInstance(const QString& instanceId, 
                                      const QString& clientSession) {
    QJsonObject data;
    data["AndroidInstanceId"] = instanceId;
    data["ClientSession"] = clientSession;
    
    sendRequest("/ConnectAndroidInstance", data, [this](const QJsonObject& response) {
        QString serverSession = response["ServerSession"].toString();
        emit instanceConnected(serverSession);
    });
}

void ApiService::createAndroidInstancesAccessToken(const QStringList& androidInstanceIds) {
    QJsonObject data;
    data["RequestId"] = QUuid::createUuid().toString();
    QJsonArray idsArray;
    for (const auto& id : androidInstanceIds) {
        idsArray.append(id);
    }
    data["AndroidInstanceIds"] = idsArray;

    sendRequest("/CreateAndroidInstancesAccessToken", data, [this](const QJsonObject& response) {
        QString accessInfo = response["AccessInfo"].toString();
        QString token = response["Token"].toString();
        emit androidInstancesAccessTokenCreated(accessInfo, token);
    });
}
