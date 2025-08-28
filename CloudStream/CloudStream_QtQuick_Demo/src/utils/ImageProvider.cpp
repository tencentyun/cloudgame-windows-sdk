#include "ImageProvider.h"
#include <QMutexLocker>
#include <QImage>

ImageProvider::ImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
    // 基类构造时指定提供 Image 类型，无需额外初始化
}

QImage ImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    // 线程安全地读取缓存
    QMutexLocker locker(&m_mutex);
    return m_imageCache.value(id, QImage()); // 找不到时返回空图
}

void ImageProvider::updateImage(const QString& instanceId, const QImage& image)
{
    // 线程安全地更新缓存；若已存在则覆盖
    QMutexLocker locker(&m_mutex);
    m_imageCache.insert(instanceId, image);
}

void ImageProvider::clearCache()
{
    // 线程安全地清空缓存
    QMutexLocker locker(&m_mutex);
    m_imageCache.clear();
}
