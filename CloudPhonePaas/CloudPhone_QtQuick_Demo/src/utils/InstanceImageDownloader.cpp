#include "InstanceImageDownloader.h"
#include "utils/Logger.h"
#include <QtConcurrent>


InstanceImageDownloader::InstanceImageDownloader(BatchTaskOperator* tcrOperator, QObject* parent)
    : QObject(parent), m_tcrOperator(tcrOperator),
      m_networkManager(new QNetworkAccessManager(this)),
      m_downloadTimer(new QTimer(this))
{
    connect(m_downloadTimer, &QTimer::timeout, this, &InstanceImageDownloader::downloadBatch);
}

void InstanceImageDownloader::startDownloading(const QStringList& instanceIds)
{
    m_instanceIds = instanceIds;
    m_paused = false; // 重置暂停状态
    m_downloadingSet.clear();
    m_downloadTimer->start(1000);
}

void InstanceImageDownloader::stopDownloading()
{
    m_downloadTimer->stop();
    m_paused = false; // 重置暂停状态
    m_downloadingSet.clear();
}

void InstanceImageDownloader::updateInstanceList(const QStringList& newInstances)
{
    m_instanceIds = newInstances;
}

// 添加暂停下载方法
void InstanceImageDownloader::pauseDownloading()
{
    m_paused = true;
    m_downloadTimer->stop();
}

// 添加恢复下载方法
void InstanceImageDownloader::resumeDownloading()
{
    m_paused = false;
    m_downloadTimer->start(1000);
}

void InstanceImageDownloader::downloadBatch()
{
    if (m_paused || m_instanceIds.isEmpty()) {
        Logger::info("InstanceImageDownloader is paused or instance list is empty");
        return;
    }

    // 每次都遍历最新的 m_instanceIds
    for (const QString& instanceId : m_instanceIds) {
        if (m_downloadingSet.contains(instanceId)) {
            // 已经在下载中，跳过
            continue;
        }

        QString imageUrl = QString::fromStdString(m_tcrOperator->getInstanceImage(instanceId).toStdString());

        if (imageUrl.isEmpty() || !imageUrl.startsWith("http")) {
            Logger::error("Invalid image URL for instance: " + instanceId);
            continue;
        }

        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = m_networkManager->get(request);

        // 标记为正在下载
        m_downloadingSet.insert(instanceId);

        connect(reply, &QNetworkReply::finished, this, [this, reply, instanceId]() {
            onImageDownloaded(reply, instanceId);
        });
    }
}

void InstanceImageDownloader::onImageDownloaded(QNetworkReply* reply, const QString& instanceId)
{
    // 下载完成，移除标记
    m_downloadingSet.remove(instanceId);

    if (reply->error() == QNetworkReply::NoError) {
        // 读取图片数据
        QByteArray imageData = reply->readAll();
        
        // 将图片解码放到后台线程
        QtConcurrent::run([this, instanceId, imageData]() {
            QImage image;
            if (image.loadFromData(imageData)) {
                // 解码完成后发出信号
                emit imageDownloaded(instanceId, image);
            }
        });
    }
    reply->deleteLater();
}
