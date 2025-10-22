#include "NetworkService.h"
#include "../utils/Logger.h"
#include <QNetworkRequest>
#include <QJsonDocument>

// ============================================================================
// 构造函数
// ============================================================================

NetworkService::NetworkService(QObject *parent) 
    : QObject(parent) {
    // 网络服务初始化完成
}

// ============================================================================
// 公共接口实现
// ============================================================================

QNetworkReply* NetworkService::postRequest(const QString& endpoint, const QJsonObject& data) {
    // 构建完整的请求URL
    QUrl url(m_baseUrl + endpoint);
    QNetworkRequest request(url);
    
    // 记录请求信息（用于调试）
    Logger::debug("Request URL: " + url.toString());
    Logger::debug("Request data: " + QJsonDocument(data).toJson(QJsonDocument::Compact));
    
    // 设置请求头：指定内容类型为JSON
    request.setHeader(QNetworkRequest::ContentTypeHeader, "application/json");
    
    // 将JSON对象转换为字节数组并发送POST请求
    QJsonDocument doc(data);
    QNetworkReply* reply = m_manager.post(request, doc.toJson());
    
    // 注意：响应处理由调用方负责，此处不做处理
    return reply;
}
