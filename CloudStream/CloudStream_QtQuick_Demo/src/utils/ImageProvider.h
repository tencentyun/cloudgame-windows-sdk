#pragma once

#include <QQuickImageProvider>
#include <QImage>
#include <QMutex>
#include <QMap>

/**
 * @brief 为 QML 提供运行时图片的 ImageProvider
 *
 * 该类继承自 QQuickImageProvider，负责把内存中的 QImage 缓存
 * 暴露给 QML 的 Image 组件。QML 通过 "image://providerId/instanceId"
 * 形式的 URL 即可获取对应实例的截图。
 *
 * 内部使用线程安全的 QMap 做缓存，支持动态更新与清空。
 */
class ImageProvider : public QQuickImageProvider
{
public:
    /**
     * @brief 构造函数，指定提供 Image 类型的图片
     */
    ImageProvider();

    /**
     * @brief QML 请求图片时的回调
     * @param id            URL 中携带的实例 ID
     * @return              缓存中的图片；若不存在则返回空 QImage
     */
    QImage requestImage(const QString &id, QSize *size, const QSize &requestedSize) override;

    /**
     * @brief 将新截图更新到缓存
     * @param instanceId 实例唯一标识
     * @param image      截图数据
     */
    void updateImage(const QString& instanceId, const QImage& image);

    /**
     * @brief 清空所有缓存图片，通常在退出或切换账号时调用
     */
    void clearCache();

private:
    QMap<QString, QImage> m_imageCache; ///< 实例 ID → 截图的映射
    QMutex m_mutex;                     ///< 保护缓存的互斥锁
};
