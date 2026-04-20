#include "InstanceImageDownloader.h"

#include <QSslConfiguration>
#include <QSslSocket>
#include <QtConcurrent>

#include "utils/Logger.h"

InstanceImageDownloader::InstanceImageDownloader(BatchTaskOperator* tcrOperator, QObject* parent)
    : QObject(parent),
      m_tcrOperator(tcrOperator),
      m_networkManager(new QNetworkAccessManager(this)),
      m_downloadTimer(new QTimer(this)) {
  connect(m_downloadTimer, &QTimer::timeout, this, &InstanceImageDownloader::downloadBatch);
}

void InstanceImageDownloader::startDownloading(const QStringList& instanceIds) {
  m_instanceIds = instanceIds;
  m_paused = false;  // 重置暂停状态
  m_downloadingSet.clear();
  m_downloadTimer->start(1000);
}

void InstanceImageDownloader::stopDownloading() {
  m_downloadTimer->stop();
  m_paused = false;  // 重置暂停状态
  m_downloadingSet.clear();
}

void InstanceImageDownloader::updateInstanceList(const QStringList& newInstances) { m_instanceIds = newInstances; }

// 添加暂停下载方法
void InstanceImageDownloader::pauseDownloading() {
  m_paused = true;
  m_downloadTimer->stop();
}

// 添加恢复下载方法
void InstanceImageDownloader::resumeDownloading() {
  m_paused = false;
  m_downloadTimer->start(1000);
}

void InstanceImageDownloader::downloadBatch() {
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

    Logger::info(QString("Starting download for instance %1: %2").arg(instanceId).arg(imageUrl));

    QNetworkRequest request(imageUrl);
    // 关键修复：配置 SSL 以接受自签名证书（用于开发/测试环境）
    QSslConfiguration sslConfig = QSslConfiguration::defaultConfiguration();
    sslConfig.setPeerVerifyMode(QSslSocket::VerifyNone);  // 禁用证书验证
    request.setSslConfiguration(sslConfig);

    QNetworkReply* reply = m_networkManager->get(request);

    // 标记为正在下载
    m_downloadingSet.insert(instanceId);

    // 连接信号，处理 SSL 错误和其他错误
    connect(reply, &QNetworkReply::errorOccurred, this, [this, reply, instanceId](QNetworkReply::NetworkError) {
      Logger::warning(QString("Network error for instance %1: %2").arg(instanceId).arg(reply->errorString()));
    });

    connect(reply, &QNetworkReply::finished, this,
            [this, reply, instanceId]() { onImageDownloaded(reply, instanceId); });
  }
}

void InstanceImageDownloader::onImageDownloaded(QNetworkReply* reply, const QString& instanceId) {
  // 下载完成，移除标记
  m_downloadingSet.remove(instanceId);

  // 检查是否有错误
  if (reply->error() != QNetworkReply::NoError) {
    // SSL 错误（HTTPS 证书验证失败）或其他网络错误
    Logger::error(QString("Failed to download image for instance %1: %2 (error code: %3)")
                      .arg(instanceId)
                      .arg(reply->errorString())
                      .arg(reply->error()));
    reply->deleteLater();
    return;
  }

  // 读取图片数据
  QByteArray imageData = reply->readAll();

  if (imageData.isEmpty()) {
    Logger::error(QString("Image data is empty for instance: %1").arg(instanceId));
    reply->deleteLater();
    return;
  }

  // 将图片解码放到后台线程
  (void)QtConcurrent::run([this, instanceId, imageData]() {
    QImage image;
    if (image.loadFromData(imageData)) {
      // 解码完成后发出信号
      Logger::info(QString("Image loaded successfully for instance: %1, size: %2x%3")
                       .arg(instanceId)
                       .arg(image.width())
                       .arg(image.height()));
      emit imageDownloaded(instanceId, image);
    } else {
      Logger::error(QString("Failed to decode image data for instance: %1").arg(instanceId));
    }
  });

  reply->deleteLater();
}
