#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>
#include <QMap>

// 图像提供者类
class InstanceImageProvider : public QQuickImageProvider
{
public:
    InstanceImageProvider();
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;
    
    void updateImage(const QString& instanceId, const QImage& image);
    void clearCache();
    
private:
    QMap<QString, QImage> m_imageCache;
    QMutex m_mutex;
};

