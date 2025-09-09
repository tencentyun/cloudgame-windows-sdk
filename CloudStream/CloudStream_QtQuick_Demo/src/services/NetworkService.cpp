#include "NetworkService.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>

NetworkService::NetworkService(QObject *parent) 
    : QObject(parent) {
}


QNetworkReply* NetworkService::postRequest(const QString& endpoint, const QJsonObject& data) {
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    
    Logger::debug("Request URL: " + url.toString());
    Logger::debug("Request data: " + QJsonDocument(data).toJson(QJsonDocument::Compact));
    
    // 设置请求头
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 发送请求
    QJsonDocument doc(data);
    QNetworkReply* reply = m_manager.post(request, doc.toJson());
    
    // 不再在此处处理响应，由调用方负责
    return reply;
}
