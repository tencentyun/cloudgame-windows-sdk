#pragma once

#include <QObject>
#include <QNetworkAccessManager>
#include <QNetworkReply>   // 添加QNetworkReply头文件
#include <QImage>          // 添加QImage头文件
#include <QTimer>
#include <QStringList>
#include "core/BatchTaskOperator.h"

class InstanceImageDownloader : public QObject
{
    Q_OBJECT

public:
    explicit InstanceImageDownloader(BatchTaskOperator* tcrOperator, QObject* parent = nullptr);
    
    void startDownloading(const QStringList& instanceIds);
    void stopDownloading();
    
    void updateInstanceList(const QStringList& newInstances);
    
    // 添加暂停下载方法
    void pauseDownloading();
    
    // 添加恢复下载方法
    void resumeDownloading();

signals:
    void imageDownloaded(const QString& instanceId, const QImage& image);

private slots:
    void downloadBatch();
    void onImageDownloaded(QNetworkReply* reply, const QString& instanceId);

private:
    BatchTaskOperator* m_tcrOperator;
    QNetworkAccessManager* m_networkManager;
    QTimer* m_downloadTimer;
    
    QStringList m_instanceIds;       // 添加实例ID列表成员
    QSet<QString> m_downloadingSet;
    bool m_paused = false;
};
