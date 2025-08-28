#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>
#include <QImage>
#include <QTimer>
#include <QStringList>

class TcrInstanceManager;

/**
 * @brief 负责批量下载 Android 实例截图的后台工作类
 *
 * 工作流程：
 * 1. 外部调用 startDownloading 设置需要下载的实例列表
 * 2. 内部 QTimer 每 1 秒触发一次 downloadBatch()
 * 3. downloadBatch() 遍历列表，对未在下载中的实例发起 HTTP 请求
 * 4. 下载完成后通过 QtConcurrent 解码图片，并发出 imageDownloaded 信号
 * 5. 支持暂停、恢复、动态更新实例列表
 */
class ImageDownloader : public QObject
{
    Q_OBJECT

public:
    explicit ImageDownloader(QObject* parent = nullptr);

    /**
     * @brief 设置TcrInstanceManager，用于获取实例截图 URL
     */
    void setTcrInstanceManager(TcrInstanceManager* model);

    /**
     * @brief 开始/重新开始下载指定实例列表
     * @param instanceIds 需要下载截图的实例 ID 列表
     */
    void startDownloading(const QStringList& instanceIds);

    /**
     * @brief 完全停止下载，清空状态
     */
    void stopDownloading();

    /**
     * @brief 动态更新需要下载的实例列表（不会中断当前下载）
     */
    void updateInstanceList(const QStringList& newInstances);

    /**
     * @brief 暂停下载，保留已下载缓存
     */
    void pauseDownloading();

    /**
     * @brief 恢复下载
     */
    void resumeDownloading();

signals:
    /**
     * @brief 单张图片下载并解码成功后发出
     * @param instanceId 实例 ID
     * @param image      解码后的 QImage
     */
    void imageDownloaded(const QString& instanceId, const QImage& image);

private slots:
    /**
     * @brief 定时器触发，批量检查并启动新下载任务
     */
    void downloadBatch();

    /**
     * @brief 单个网络请求完成后的回调
     * @param reply      网络响应对象
     * @param instanceId 对应的实例 ID
     */
    void onImageDownloaded(QNetworkReply* reply, const QString& instanceId);

private:
    QNetworkAccessManager* m_networkManager; ///< 全局网络访问管理器
    QTimer* m_downloadTimer;                 ///< 周期性触发下载的定时器
    QStringList m_instanceIds;               ///< 当前需要下载的实例列表
    QSet<QString> m_downloadingSet;          ///< 正在下载中的实例集合，防止重复
    bool m_paused = false;                   ///< 暂停标志位
    TcrInstanceManager* m_instanceManager = nullptr; ///< 用于获取图片 URL 的 Model
};
