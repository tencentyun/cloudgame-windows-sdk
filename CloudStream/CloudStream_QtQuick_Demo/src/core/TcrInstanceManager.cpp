#include "TcrInstanceManager.h"
#include "services/ApiService.h"
#include "utils/Logger.h"

ImageProvider* TcrInstanceManager::s_imageProvider = new ImageProvider();

TcrInstanceManager::TcrInstanceManager(ApiService* apiService, QObject* parent)
    : QObject(parent), m_apiService(apiService), m_imageDownloader(new ImageDownloader(this))
{
    connect(m_imageDownloader, &ImageDownloader::imageDownloaded, this, &TcrInstanceManager::onImageDownloaded);
}


TcrInstanceManager::~TcrInstanceManager()
{
    m_imageDownloader->stopDownloading();
}

void TcrInstanceManager::setDownloadInstances(const QStringList& instanceIds)
{
    m_imageDownloader->updateInstanceList(instanceIds);
    Logger::info(QString("Set download instances: %1").arg(instanceIds.join(",")));
}

void TcrInstanceManager::pauseImageDownload()
{
    m_imageDownloader->pauseDownloading();
}

void TcrInstanceManager::resumeImageDownload()
{
    m_imageDownloader->resumeDownloading();
}

QVariantList TcrInstanceManager::instances() const {
    QVariantList list;
    for (const auto& id : m_instances) {
        QVariantMap map;
        map["AndroidInstanceId"] = id;
        list.append(map);
    }
    return list;
}

void TcrInstanceManager::initialize(const QStringList& instanceIds, const QString& accessInfo, const QString& token)
{
    Logger::info(QString("TcrInstanceManager::initialize called"));
    Logger::info(QString("Instance IDs: %1").arg(instanceIds.join(", ")));
    Logger::info(QString("Access Info: %1").arg(accessInfo));
    Logger::info(QString("Token: %1").arg(token));

    m_instances = instanceIds;

    // 将Qt字符串转换为C风格字符串，供C API使用
    std::string tokenStr = token.toStdString();
    std::string accessInfoStr = accessInfo.toStdString();

    // 配置TcrSdk初始化参数
    // TcrConfig结构体包含两个必填字段：
    // - Token: 访问令牌，用于身份验证
    // - AccessInfo: 访问信息，包含服务器地址等配置
    TcrConfig config = {tokenStr.c_str(), accessInfoStr.c_str(), CloudStream};
    
    // 获取TcrClient单例句柄
    // tcr_client_get_instance()返回一个全局唯一的TcrClientHandle
    TcrClientHandle client = tcr_client_get_instance();
    
    // 初始化TcrClient
    // TcrSdk的所有后续接口调用必须在次函数返回成功之后进行
    TcrErrorCode result = tcr_client_init(client, &config);
    if (result != TCR_SUCCESS) {
        Logger::error(QString("Init TCR SDK failed"));
        return;
    }
    
    // 获取Android实例操作句柄
    // 通过此句柄可以获取实例的截图
    m_tcrAndroidInstance = tcr_client_get_android_instance(client);

    // 将当前TcrInstanceManager实例设置给ImageDownloader
    // 这样ImageDownloader就可以通过getImageUrl()获取图片URL
    m_imageDownloader->setTcrInstanceManager(this);

    // 启动图片下载任务
    // 传入实例ID列表，ImageDownloader会为每个实例下载截图
    m_imageDownloader->startDownloading(instanceIds);
    
    // 通知QML层实例列表已更新
    emit instancesChanged();
}

QString TcrInstanceManager::getImageUrl(const QString &instanceId) {
    constexpr int BUFFER_SIZE = 4096;
    std::vector<char> buffer(BUFFER_SIZE, 0);

    if (m_tcrAndroidInstance == nullptr) {
        Logger::error(QString("getInstanceImageUrl() m_tcrAndroidInstance is null"));
        return QString();
    }

    if (!tcr_instance_get_image(m_tcrAndroidInstance, buffer.data(), BUFFER_SIZE, instanceId.toUtf8().constData(),135, 240, 20)) {
        Logger::error(QString("cyy_test tcr_instance_get_image failed"));
        return QString();
    }
    
    return QString::fromUtf8(buffer.data());
}

void TcrInstanceManager::onImageDownloaded(const QString& instanceId, const QImage& image)
{
    // 更新图片缓存
    s_imageProvider->updateImage(instanceId, image);
    // 发出图片更新信号
    emit imageUpdated(instanceId);
}

ImageProvider* TcrInstanceManager::imageProvider()
{
    return s_imageProvider;
}
