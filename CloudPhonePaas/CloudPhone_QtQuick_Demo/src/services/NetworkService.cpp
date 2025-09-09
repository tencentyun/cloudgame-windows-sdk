#include "NetworkService.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>

NetworkService::NetworkService(QObject *parent) 
    : QObject(parent) {
    // 默认使用测试环境
    setEnvironment(false);
}

void NetworkService::setEnvironment(bool isProduction) {
    m_baseUrl = isProduction 
        ? "https://cai-server.cloud-device.crtrcloud.com" 
        : "https://test-cai-server.cloud-device.crtrcloud.com";
}

QNetworkReply* NetworkService::postRequest(const QString& endpoint, const QJsonObject& data) {
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    
    // Log request info
    Logger::debug("Request URL: " + url.toString());
    Logger::debug("Request data: " + QJsonDocument(data).toJson(QJsonDocument::Compact));
    
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    if (!m_token.isEmpty()) {
        request.setRawHeader("Authorization", "Bearer " + m_token);
    }
    
    // 发送请求
    QJsonDocument doc(data);
    QNetworkReply* reply = m_manager.post(request, doc.toJson());
    
    // 不再在此处处理响应，由调用方负责
    return reply;
}

QByteArray NetworkService::getToken() const {
    return m_token;
}

void NetworkService::setToken(const QByteArray& token) {
    m_token = token;
}
