#include "InstanceTokenViewModel.h"
#include <QStringList>
#include "utils/Logger.h"

InstanceTokenViewModel::InstanceTokenViewModel(ApiService* apiService, QObject *parent)
    : QObject(parent), m_apiService(apiService) {
    
    // 连接ApiService的信号
    connect(m_apiService, &ApiService::androidInstancesAccessTokenCreated,
            this, &InstanceTokenViewModel::onAccessTokenCreated);
    connect(m_apiService, &ApiService::apiError,
            this, &InstanceTokenViewModel::onApiError);
}
void InstanceTokenViewModel::createAccessToken(const QString& instanceIds, const QString& userIp) {

    Logger::info(QString("InstanceTokenViewModel::createAccessToken called with instanceIds: %1, userIp: %2").arg(instanceIds, userIp));
    
    if (m_isBusy) {
        return;
    }
    
    if (instanceIds.trimmed().isEmpty()) {
        emit errorOccurred("请输入实例ID列表");
        return;
    }
    
    m_isBusy = true;
    emit isBusyChanged();
    
    // 分割实例ID列表
    QStringList idList = instanceIds.split(",", Qt::SkipEmptyParts);
    for (int i = 0; i < idList.size(); ++i) {
        idList[i] = idList[i].trimmed();
    }
    
    // 调用ApiService创建访问令牌
    m_apiService->createAndroidInstancesAccessToken(idList, userIp.trimmed());
}

void InstanceTokenViewModel::onAccessTokenCreated(const QString& accessInfo, const QString& token) {
    m_isBusy = false;
    emit isBusyChanged();
    emit accessTokenCreated(accessInfo, token);
}

void InstanceTokenViewModel::onApiError(const QString& errorCode, const QString& message) {
    m_isBusy = false;
    emit isBusyChanged();
    emit errorOccurred(QString("错误 %1: %2").arg(errorCode, message));
}