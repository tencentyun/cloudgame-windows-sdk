#include "NetworkService.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>

NetworkService::NetworkService(QObject *parent) 
    : QObject(parent) {
    // 默认使用测试环境
    setEnvironment(true);
}

void NetworkService::setEnvironment(bool isProduction) {
    m_baseUrl = isProduction 
        ? "https://cai-server.cloud-device.crtrcloud.com" 
        : "http://test-cai-experience-server.crtrcloud.com/external";
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
    // if (!m_requestHost.isEmpty()) {
    //     request.setRawHeader("Request-Host", m_requestHost.toUtf8());
    // }
    // if (!m_origin.isEmpty()) {
    //     request.setRawHeader("Origin", m_origin.toUtf8());
    // }
    
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

void NetworkService::setRequestHost(const QString& requestHost) {
    m_requestHost = requestHost;
}

void NetworkService::setOrigin(const QString& origin) {
    m_origin = origin;
}
