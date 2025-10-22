#include "InstanceTokenViewModel.h"
#include <QStringList>
#include "utils/Logger.h"

// ============================================================================
// 构造函数
// ============================================================================

InstanceTokenViewModel::InstanceTokenViewModel(ApiService* apiService, QObject *parent)
    : QObject(parent)
    , m_apiService(apiService)
    , m_isBusy(false) {
    
    // 连接ApiService的成功信号
    connect(m_apiService, &ApiService::androidInstancesAccessTokenCreated,
            this, &InstanceTokenViewModel::onAccessTokenCreated);
    
    // 连接ApiService的错误信号
    connect(m_apiService, &ApiService::apiError,
            this, &InstanceTokenViewModel::onApiError);
}

// ============================================================================
// 公共方法
// ============================================================================

void InstanceTokenViewModel::createAccessToken(const QString& instanceIds, const QString& userIp) {
    Logger::info(QString("InstanceTokenViewModel::createAccessToken called with instanceIds: %1, userIp: %2")
                 .arg(instanceIds, userIp));
    
    // 防止重复请求：如果当前正在处理请求，则忽略新请求
    if (m_isBusy) {
        Logger::warning("Request ignored: already processing a request");
        return;
    }
    
    // 验证输入：实例ID列表不能为空
    if (instanceIds.trimmed().isEmpty()) {
        emit errorOccurred("请输入实例ID列表");
        return;
    }
    
    // 设置忙碌状态
    m_isBusy = true;
    emit isBusyChanged();
    
    // 解析实例ID列表：按逗号分割并去除空白字符
    QStringList idList = instanceIds.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < idList.size(); ++i) {
        idList[i] = idList[i].trimmed();
    }
    
    Logger::info(QString("Parsed %1 instance IDs").arg(idList.size()));
    
    // 调用ApiService创建访问令牌
    m_apiService->createAndroidInstancesAccessToken(idList, userIp.trimmed());
}

// ============================================================================
// 私有槽函数
// ============================================================================

void InstanceTokenViewModel::onAccessTokenCreated(const QString& accessInfo, const QString& token) {
    Logger::info("Access token created successfully");
    
    // 重置忙碌状态
    m_isBusy = false;
    emit isBusyChanged();
    
    // 向UI层发送成功信号
    emit accessTokenCreated(accessInfo, token);
}

void InstanceTokenViewModel::onApiError(const QString& errorCode, const QString& message) {
    Logger::error(QString("API error occurred - Code: %1, Message: %2").arg(errorCode, message));
    
    // 重置忙碌状态
    m_isBusy = false;
    emit isBusyChanged();
    
    // 向UI层发送错误信号（格式化错误信息）
    emit errorOccurred(QString("错误 %1: %2").arg(errorCode, message));
}