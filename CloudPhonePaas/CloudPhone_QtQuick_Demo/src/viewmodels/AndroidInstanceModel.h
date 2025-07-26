#pragma once

#include <QObject>
#include <QJsonArray>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QTimer>
#include "InstanceImageProvider.h"
#include "core/BatchTaskOperator.h"
#include "utils/InstanceImageDownloader.h"

class ApiService;

/**
 * @brief 表示单个云手机实例的信息结构体(仅包含云API返回的一部分字段)
 */
struct AndroidInstance {
    QString AndroidInstanceId;           ///< 实例ID
    QString AndroidInstanceRegion;       ///< 区域
    QString Name;                        ///< 实例名称
    QString State;                       ///< 实例状态
    QString UserId;                      ///< 用户ID
};

/**
 * @brief 云手机实例模型，管理实例列表及图片下载
 *
 * 该模型负责与ApiService交互，获取云手机实例列表，并通过InstanceImageDownloader异步下载实例图片。
 * 同时提供QML属性和方法，供前端界面调用。
 */
class AndroidInstanceModel : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param apiService API服务指针
     * @param tcrOperator 批量任务操作器指针
     * @param parent 父对象
     */
    explicit AndroidInstanceModel(ApiService* apiService, BatchTaskOperator* tcrOperator, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     */
    ~AndroidInstanceModel();

    /**
     * @brief 获取当前实例列表（QML属性）
     */
    Q_PROPERTY(QVariantList instances READ instances NOTIFY instancesChanged)
    QVariantList instances() const;

    /**
     * @brief 获取全局图像提供者实例
     * @return InstanceImageProvider*
     */
    static InstanceImageProvider* imageProvider();

    /**
     * @brief 暂停图片下载
     */
    Q_INVOKABLE void pauseImageDownload();

    /**
     * @brief 恢复图片下载
     */
    Q_INVOKABLE void resumeImageDownload();

    /**
     * @brief 设置需要下载图片的实例ID列表
     * @param instanceIds 实例ID列表
     */
    Q_INVOKABLE void setDownloadInstances(const QStringList& instanceIds);

    /**
     * @brief 刷新实例列表（重新拉取并清空缓存）
     */
    Q_INVOKABLE void refreshInstances();

signals:
    /**
     * @brief 实例列表发生变化时发出
     */
    void instancesChanged();

    /**
     * @brief 某个实例图片更新时发出
     * @param instanceId 实例ID
     */
    void imageUpdated(const QString& instanceId);

private slots:
    /**
     * @brief 登录成功后回调
     * @param userType 用户类型
     */
    void onLoginSuccess(const QString& userType);

    /**
     * @brief 收到实例列表后回调
     * @param instances 实例列表
     * @param totalCount 总数
     */
    void onInstancesReceived(const QList<AndroidInstance>& instances, int totalCount);

    /**
     * @brief 图片下载完成后回调
     * @param instanceId 实例ID
     * @param image 下载的图片
     */
    void onImageDownloaded(const QString& instanceId, const QImage& image);

private:
    ApiService* m_apiService;                    ///< API服务指针
    QList<AndroidInstance> m_instances;          ///< 当前实例列表
    bool m_tcrConfigured = false;                ///< TCR SDK是否已配置
    InstanceImageDownloader* m_imageDownloader;  ///< 图片下载器
    static InstanceImageProvider* s_imageProvider; ///< 静态图像提供者实例
};
