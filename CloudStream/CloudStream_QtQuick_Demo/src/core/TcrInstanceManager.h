#pragma once

#include <QObject>
#include <QJsonArray>
#include <QVariant>
#include <QVariantList>
#include <QVariantMap>
#include <QNetworkAccessManager>
#include <QTimer>
#include "utils/ImageProvider.h"
#include "utils/ImageDownloader.h"

#include "tcr_c_api.h"
#include "tcr_types.h"

class ApiService;

/**
 * @class TcrInstanceManager
 * @brief 演示如何使用 TcrSdk 并获取云手机实例画面截图进行渲染的示例类
 *
 * 本类作为 QtQuick 演示程序的核心组件，展示了：
 * 1. 如何初始化 TcrSdk
 * 2. 如何获取云手机实例的实时截图
 * 3. 如何通过自定义 ImageProvider 将截图渲染到 QML 界面
 * 4. 如何管理多个实例的图片下载和缓存
 *
 * 使用流程：
 * - 通过 initialize() 方法传入实例ID列表和认证信息
 * - 通过 instances() 属性向 QML 提供实例数据
 * - 通过 ImageProvider 机制实现截图的异步加载和显示
 * - 支持图片下载的暂停/恢复以优化性能
 */
class TcrInstanceManager : public QObject {
    Q_OBJECT
public:
    /**
     * @brief 构造函数
     * @param apiService API服务指针，用于与后端服务通信
     * @param parent 父对象，用于Qt对象树管理
     */
    explicit TcrInstanceManager(ApiService* apiService, QObject* parent = nullptr);

    /**
     * @brief 析构函数
     * 负责清理资源，包括停止图片下载和释放相关对象
     */
    ~TcrInstanceManager();

    /**
     * @brief 获取当前实例列表（QML属性）
     * @return QVariantList 包含所有云手机实例信息的列表
     * @note 每个实例包含AndroidInstanceId等基本信息
     */
    Q_PROPERTY(QVariantList instances READ instances NOTIFY instancesChanged)
    QVariantList instances() const;

    /**
     * @brief 获取全局图像提供者实例
     * @return ImageProvider* 用于向QML提供实例截图的图像提供者
     * @note 这是一个单例对象，所有截图请求都通过它来处理
     */
    static ImageProvider* imageProvider();

    /**
     * @brief 暂停图片下载
     * @note 通常在用户快速滚动列表时调用，避免不必要的网络请求
     */
    Q_INVOKABLE void pauseImageDownload();

    /**
     * @brief 恢复图片下载
     * @note 当用户停止滚动后调用，开始加载可见区域的截图
     */
    Q_INVOKABLE void resumeImageDownload();

    /**
     * @brief 设置需要下载图片的实例ID列表
     * @param instanceIds 当前可见区域的实例ID列表
     * @note 用于优化性能，只下载用户可见的实例截图
     */
    Q_INVOKABLE void setDownloadInstances(const QStringList& instanceIds);

    /**
     * @brief 初始化TcrSdk
     * @param instanceIds 需要管理的云手机实例ID列表
     * @param accessInfo 访问信息，从业务后台获取
     * @param token 访问令牌，从业务后台获取
     * @note 必须在创建对象后调用此函数才能正常使用其他功能
     */
    Q_INVOKABLE void initialize(const QStringList& instanceIds, const QString& accessInfo, const QString& token);

    /**
     * @brief 获取指定实例的截图URL
     * @param instanceId 云手机实例ID
     * @return QString 截图的URL
     */
    QString getImageUrl(const QString &instanceId);

signals:
    /**
     * @brief 实例列表发生变化时发出
     * @note 当实例信息更新或新增/删除实例时触发
     */
    void instancesChanged();

    /**
     * @brief 某个实例图片更新时发出
     * @param instanceId 发生更新的实例ID
     * @note 当获取到新的截图时触发，QML界面可据此刷新显示
     */
    void imageUpdated(const QString& instanceId);

private slots:
    /**
     * @brief 图片下载完成后回调
     * @param instanceId 完成下载的实例ID
     * @param image 下载的截图图片
     * @note 将下载的图片缓存到ImageProvider中，并通知界面更新
     */
    void onImageDownloaded(const QString& instanceId, const QImage& image);

private:
    ApiService* m_apiService;                    ///< API服务指针，用于后端通信
    QStringList m_instances;                     ///< 当前管理的实例ID列表
    bool m_tcrConfigured = false;                ///< TCR SDK是否已配置完成
    ImageDownloader* m_imageDownloader;          ///< 图片下载器，负责异步获取实例截图
    static ImageProvider* s_imageProvider;       ///< 静态图像提供者实例，单例模式
    TcrAndroidInstance m_tcrAndroidInstance;     ///< 用于获取截图的TcrAndroidInstance对象
};
