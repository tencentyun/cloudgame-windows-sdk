#include "ImageDownloader.h"
#include "tcr_c_api.h"
#include "utils/Logger.h"
#include <core/TcrInstanceManager.h>
#include <QtConcurrent>

ImageDownloader::ImageDownloader(QObject* parent)
    : QObject(parent),
      m_networkManager(new QNetworkAccessManager(this)),
      m_downloadTimer(new QTimer(this))
{
    connect(m_downloadTimer, &QTimer::timeout, this, &ImageDownloader::downloadBatch);
}

void ImageDownloader::setTcrInstanceManager(TcrInstanceManager* model)
{
    m_instanceManager = model;
}

void ImageDownloader::startDownloading(const QStringList& instanceIds)
{
    m_instanceIds = instanceIds;
    m_paused = false;
    m_downloadingSet.clear();
    m_downloadTimer->start(1000);
}

void ImageDownloader::stopDownloading()
{
    m_downloadTimer->stop();
    m_paused = false;
    m_downloadingSet.clear();
}

void ImageDownloader::updateInstanceList(const QStringList& newInstances)
{
    m_instanceIds = newInstances;
}

void ImageDownloader::pauseDownloading()
{
    m_paused = true;
    m_downloadTimer->stop();
}

void ImageDownloader::resumeDownloading()
{
    m_paused = false;
    m_downloadTimer->start(1000);
}

void ImageDownloader::downloadBatch()
{
    if (m_paused || m_instanceIds.isEmpty()) {
        return;
    }

    for (const QString& instanceId : m_instanceIds) {
        if (m_downloadingSet.contains(instanceId)) {
            continue;
        }

        QString imageUrl = m_instanceManager->getImageUrl(instanceId);
        if (imageUrl.isEmpty() || !imageUrl.startsWith("http") || !imageUrl.startsWith("https")) {
            Logger::error(QString("ImageDownloader: Invalid image URL for instance %1: %2").arg(instanceId).arg(imageUrl));
            continue;
        }

        QNetworkRequest request(imageUrl);
        QNetworkReply* reply = m_networkManager->get(request);

        m_downloadingSet.insert(instanceId);

        connect(reply, &QNetworkReply::finished, this, [this, reply, instanceId]() {
            onImageDownloaded(reply, instanceId);
        });
    }
}

void ImageDownloader::onImageDownloaded(QNetworkReply* reply, const QString& instanceId)
{
    m_downloadingSet.remove(instanceId);

    if (reply->error() == QNetworkReply::NoError) {
        QByteArray imageData = reply->readAll();
        QtConcurrent::run([this, instanceId, imageData]() {
            QImage image;
            if (image.loadFromData(imageData)) {
                emit imageDownloaded(instanceId, image);
            } else {
                Logger::error(QString("ImageDownloader: Failed to decode image data for instance %1").arg(instanceId));
            }
        });
    } else {
        Logger::error(QString("ImageDownloader: Network error for instance %1: %2 (%3)")
                      .arg(instanceId)
                      .arg(reply->errorString())
                      .arg(reply->error()));
    }
    reply->deleteLater();
}
