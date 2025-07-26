#include "InstanceImageProvider.h"  
#include <QMutexLocker>            
#include <QImage> 

// 图像提供者实现
InstanceImageProvider::InstanceImageProvider()
    : QQuickImageProvider(QQuickImageProvider::Image)
{
}

QImage InstanceImageProvider::requestImage(const QString &id, QSize *size, const QSize &requestedSize)
{
    QMutexLocker locker(&m_mutex);
    if (m_imageCache.contains(id)) {
        return m_imageCache.value(id);
    }
    return QImage(); // 返回空图片
}

void InstanceImageProvider::updateImage(const QString& instanceId, const QImage& image)
{
    QMutexLocker locker(&m_mutex);
    m_imageCache.insert(instanceId, image);
}

void InstanceImageProvider::clearCache()
{
    QMutexLocker locker(&m_mutex);
    m_imageCache.clear();
}