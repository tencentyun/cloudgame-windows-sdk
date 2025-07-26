#include "AndroidInstanceModel.h"
#include "services/ApiService.h"
#include "utils/Logger.h"

// 静态成员初始化
InstanceImageProvider* AndroidInstanceModel::s_imageProvider = new InstanceImageProvider();

AndroidInstanceModel::AndroidInstanceModel(ApiService* apiService, BatchTaskOperator* op, QObject* parent)
    : QObject(parent), m_apiService(apiService), m_imageDownloader(new InstanceImageDownloader(op, this))
{
    // 连接信号
    connect(m_apiService, &ApiService::loginSuccess, this, &AndroidInstanceModel::onLoginSuccess);
    connect(m_apiService, &ApiService::instancesReceived, this, &AndroidInstanceModel::onInstancesReceived);
    
    // 连接下载器的图片下载完成信号
    connect(m_imageDownloader, &InstanceImageDownloader::imageDownloaded, this, &AndroidInstanceModel::onImageDownloaded);
}


AndroidInstanceModel::~AndroidInstanceModel()
{
    m_imageDownloader->stopDownloading();
}

// 设置下载实例列表
void AndroidInstanceModel::setDownloadInstances(const QStringList& instanceIds)
{
    m_imageDownloader->updateInstanceList(instanceIds);
    Logger::info(QString("Set download instances: %1").arg(instanceIds.join(",")));
}

void AndroidInstanceModel::pauseImageDownload()
{
    m_imageDownloader->pauseDownloading();
}

void AndroidInstanceModel::resumeImageDownload()
{
    m_imageDownloader->resumeDownloading();
}

QVariantList AndroidInstanceModel::instances() const {
    QVariantList list;
    for (const auto& instance : m_instances) {
        QVariantMap map;
        map["AndroidInstanceId"] = instance.AndroidInstanceId;
        map["AndroidInstanceRegion"] = instance.AndroidInstanceRegion;
        map["State"] = instance.State;
        list.append(map);
    }
    return list;
}

void AndroidInstanceModel::onLoginSuccess(const QString& userType) {
    m_apiService->describeAndroidInstances(0, 200);
}

void AndroidInstanceModel::onInstancesReceived(const QList<AndroidInstance>& instances, int totalCount) {
    m_instances = instances;

    // 1. 提取所有实例ID
    QStringList instanceIds;
    for (const auto& instance : m_instances) {
        instanceIds.append(instance.AndroidInstanceId);
    }

    // 2. 调用创建安卓实例令牌API
    m_apiService->createAndroidInstancesAccessToken(instanceIds);

    // 3. 连接令牌创建成功信号（注意：避免重复连接，使用disconnect断开之前的连接）
    disconnect(m_apiService, &ApiService::androidInstancesAccessTokenCreated, this, nullptr);
    connect(m_apiService, &ApiService::androidInstancesAccessTokenCreated, this,
        [this, instanceIds](const QString& accessInfo, const QString& token) {
            // 4. 初始化TCR SDK
            std::string tokenStr = token.toStdString();
            std::string accessInfoStr = accessInfo.toStdString();

            TcrConfig config = {tokenStr.c_str(), accessInfoStr.c_str()};
            TcrClientHandle client = tcr_client_get_instance();
            TcrErrorCode result = tcr_client_init(client, &config);
            if (result != TCR_SUCCESS) {
                Logger::error(QString("Init TCR SDK failed"));
                return;
            }

            // 先只下载前10个实例的图片, 剩下的随着滚动位置按需下载
            QStringList firstTenInstances = instanceIds.mid(0, 10);        
            m_imageDownloader->startDownloading(firstTenInstances);
            emit instancesChanged();  // 通知QML刷新视图
        });

}

void AndroidInstanceModel::refreshInstances()
{
    Logger::debug("AndroidInstanceModel::refreshInstances");
    // 1. 停止当前图片下载
    m_imageDownloader->stopDownloading();

    // 2. 清空图片缓存
    s_imageProvider->clearCache();

    // 3. 清空本地实例列表
    m_instances.clear();
    emit instancesChanged();

    // 4. 重新拉取实例列表
    m_apiService->describeAndroidInstances(0, 200);
}


void AndroidInstanceModel::onImageDownloaded(const QString& instanceId, const QImage& image)
{
    // 更新图片缓存
    s_imageProvider->updateImage(instanceId, image);
    // 发出图片更新信号
    emit imageUpdated(instanceId);
}

InstanceImageProvider* AndroidInstanceModel::imageProvider()
{
    return s_imageProvider;
}

