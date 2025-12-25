#pragma once

#include <QObject>
#include <QStringList>
#include <QVariantList>
#include <QHash>
#include <QVector>
#include <QJsonObject>
#include <QJsonDocument>
#include <QPointer>
#include <QTimer>
#include <QMutex>
#include <QMutexLocker>
#include <atomic>
#include "core/video/Frame.h"
#include "core/video/VideoRenderItem.h"
#include "core/StreamConfig.h"
#include "tcr_c_api.h"

/**
 * @brief 实例连接状态枚举
 */
enum class InstanceConnectionState {
    Disconnected = 0,  ///< 未连接
    Connecting = 1,    ///< 连接中
    Connected = 2      ///< 已连接
};

/**
 * @brief 多实例串流 ViewModel - TcrSdk 多会话管理示例
 * 
 * 本类演示如何使用 TcrSdk C API 实现多实例同时串流：
 * 
 * 核心功能：
 * 1. 多会话管理：支持创建多个独立的 TcrSession，每个会话可连接多个云手机实例
 * 2. 视频帧分发：根据 instance_id 和 instance_index 将视频帧路由到对应的渲染组件
 * 3. 事件监听：处理会话连接状态、错误等事件
 * 4. 资源管理：正确管理 TcrClient、TcrSession 和观察者的生命周期
 * 
 * TcrSdk API 使用流程：
 * 1. 初始化：tcr_client_get_instance() -> tcr_client_init()
 * 2. 创建会话：tcr_client_create_session()
 * 3. 设置观察者：tcr_session_set_observer() / tcr_session_set_video_frame_observer()
 * 4. 连接实例：tcr_session_access()
 * 5. 接收回调：SessionEventCallback / VideoFrameCallback
 * 6. 清理资源：tcr_client_destroy_session()
 */
class MultiStreamViewModel : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QStringList connectedInstanceIds READ connectedInstanceIds NOTIFY connectedInstanceIdsChanged)
    Q_PROPERTY(QString clientStats READ clientStats NOTIFY clientStatsChanged)

public:
    explicit MultiStreamViewModel(QObject *parent = nullptr);
    ~MultiStreamViewModel() override;

    // ==================== 属性访问 ====================
    
    /**
     * @brief 获取已连接的实例ID列表
     */
    QStringList connectedInstanceIds() const { return m_connectedInstanceIds; }

    /**
     * @brief 获取客户端统计数据（JSON 格式）
     * @return 包含帧率、码率、延迟等信息的 JSON 字符串
     */
    QString clientStats() const { return m_clientStats; }

    /**
     * @brief 根据实例ID获取该实例的统计数据（JSON 格式）
     * @param instanceId 实例ID
     * @return 该实例的统计数据 JSON 字符串，如果不存在则返回空字符串
     */
    Q_INVOKABLE QString getInstanceStats(const QString& instanceId) const;

    /**
     * @brief 获取指定实例的连接状态
     * @param instanceId 实例ID
     * @return 0=未连接, 1=连接中, 2=已连接
     */
    Q_INVOKABLE int getInstanceConnectionState(const QString& instanceId) const;

    // ==================== QML 调用接口 ====================

public slots:
    /**
     * @brief 处理实例可见性变化（从QML调用）
     * @param visibleIds 可见的实例ID列表
     * @param invisibleIds 不可见的实例ID列表
     */
    void onVisibilityChanged(const QStringList& visibleIds, const QStringList& invisibleIds);

public:
    /**
     * @brief 注册 VideoRenderItem 到指定实例
     * @param instanceId 实例ID
     * @param instanceIndex 实例索引（用于区分同一实例ID的多个视频流）
     * @param item VideoRenderItem 指针
     * 
     * 说明：建立 "instanceId_instanceIndex" 到 VideoRenderItem 的映射关系，
     *       用于视频帧回调时的路由分发
     */
    Q_INVOKABLE void registerVideoRenderItem(const QString& instanceId, int instanceIndex, QObject* item);

    /**
     * @brief 初始化 TcrSdk 客户端
     * @param instanceIds 需要管理的云手机实例ID列表（预留参数，当前未使用）
     * @param accessInfo 访问信息，从业务后台获取
     * @param token 访问令牌，从业务后台获取
     * 
     * TcrSdk API 调用：
     * - tcr_client_get_instance()：获取全局单例
     * - tcr_client_init()：初始化客户端，传入 Token 和 AccessInfo
     */
    Q_INVOKABLE void initialize(const QStringList& instanceIds, const QString& accessInfo, const QString& token);

    /**
     * @brief 根据 QML 指定的会话配置连接多个实例
     * @param sessionConfigs 会话配置列表，每个元素是一个实例ID数组
     * 格式示例: [[\"instance1\", \"instance2\"], [\"instance3\", \"instance4\"]]
     * 
     * 说明：
     * - 每个子数组代表一个会话（TcrSession）
     * - 一个会话可以连接多个实例（群控场景）
     * - 会自动创建对应数量的 TcrSession 并设置观察者
     */
    Q_INVOKABLE void connectMultipleInstances(const QVariantList& sessionConfigs);

    /**
     * @brief 主动关闭所有会话，释放资源
     * 
     * TcrSdk API 调用：
     * - tcr_session_set_observer(nullptr)：取消事件观察者
     * - tcr_session_set_video_frame_observer(nullptr)：取消视频帧观察者
     * - tcr_client_destroy_session()：销毁会话
     */
    Q_INVOKABLE void closeAllSessions();

    /**
     * @brief 暂停指定实例的流媒体
     * @param instanceIds 实例ID列表
     * 
     * TcrSdk API 调用：
     * - tcr_session_pause_streaming()：暂停流媒体
     */
    Q_INVOKABLE void pauseStreaming(const QStringList& instanceIds);

    /**
     * @brief 恢复指定实例的流媒体
     * @param instanceIds 实例ID列表
     * 
     * TcrSdk API 调用：
     * - tcr_session_resume_streaming()：恢复流媒体
     */
    Q_INVOKABLE void resumeStreaming(const QStringList& instanceIds);

    // ==================== 信号 ====================

signals:

    /**
     * @brief 实例连接状态变化
     * @param instanceId 实例ID
     * @param connected 是否已连接
     */
    void instanceConnectionChanged(const QString& instanceId, bool connected);

    /**
     * @brief 会话关闭信号
     * @param sessionIndex 会话索引
     * @param instanceIds 该会话包含的实例ID列表
     * @param reason 关闭原因
     */
    void sessionClosed(int sessionIndex, const QStringList& instanceIds, const QString& reason);

    void connectedInstanceIdsChanged();

    /**
     * @brief 统计数据更新信号
     * 
     * 当收到 TCR_SESSION_EVENT_CLIENT_STATS 事件时触发
     */
    void clientStatsChanged();

private:
    // ==================== 内部数据结构 ====================

    /**
     * @brief 会话用户数据 - 用于 C 回调函数的上下文传递
     * 
     * 说明：C API 的回调函数是静态函数，需要通过 user_data 传递对象指针
     */
    struct SessionUserData {
        MultiStreamViewModel* viewModel;  ///< ViewModel 对象指针
        int sessionIndex;                 ///< 会话索引，用于定位 m_sessions 中的元素
    };

    /**
     * @brief 会话信息 - 封装单个 TcrSession 的所有相关数据
     */
    struct SessionInfo {
        TcrSessionHandle session = nullptr;           ///< TcrSdk 会话句柄
        QStringList instanceIds;                      ///< 该会话连接的实例ID列表
        bool connected = false;                       ///< 连接状态
        
        // 观察者结构体（必须在整个会话生命周期内保持有效）
        TcrSessionObserver sessionObserver = {};      ///< 会话事件观察者
        TcrVideoFrameObserver videoFrameObserver = {};///< 视频帧观察者
        SessionUserData userData;                     ///< 用户数据，传递给回调函数
    };

    // ==================== 成员变量 ====================

    TcrClientHandle m_tcrClient = nullptr;  ///< TcrSdk 客户端句柄（全局单例）
    QVector<SessionInfo> m_sessions;        ///< 会话信息列表
    
    QStringList m_connectedInstanceIds;     ///< 已连接的实例ID列表
    QString m_clientStats;                  ///< 客户端统计数据（JSON 格式）
    
    /// 实例连接状态映射：instanceId -> InstanceConnectionState
    QHash<QString, InstanceConnectionState> m_instanceConnectionStates;
    
    /// 视频渲染项映射："instanceId_instanceIndex" -> VideoRenderItem
    /// 使用 QPointer 防止访问已销毁的对象
    QHash<QString, QPointer<VideoRenderItem>> m_videoRenderItems;

    // 帧缓存优化相关
    QMap<QString, VideoFrameDataPtr> m_frameCache;  // uniqueKey -> 最新帧
    QMutex m_frameCacheMutex;                       // 保护帧缓存的互斥锁
    QTimer* m_renderTimer = nullptr;                // 定时刷新定时器

    // 线程安全保护
    std::atomic<bool> m_isDestroying{false};        // 析构标志，用于快速检测
    QMutex m_videoRenderItemsMutex;                 // 保护 m_videoRenderItems 的互斥锁

    // ==================== 内部方法 ====================

    /**
     * @brief 根据会话配置创建多个 TcrSession
     * @param sessionConfigs 会话配置列表
     * 
     * TcrSdk API 调用流程：
     * 1. tcr_client_create_session()：创建会话
     * 2. 初始化观察者结构体
     * 3. setSessionObservers()：设置观察者
     * 4. tcr_session_access()：连接实例
     */
    void createSessionsWithConfigs(const QVariantList& sessionConfigs);

    /**
     * @brief 为指定会话设置观察者
     * @param sessionIndex 会话索引
     * 
     * TcrSdk API 调用：
     * - tcr_session_set_observer()：设置会话事件观察者
     * - tcr_session_set_video_frame_observer()：设置视频帧观察者
     */
    void setSessionObservers(int sessionIndex);

    // 批量渲染缓存的帧
    void batchRenderFrames();  

    // ==================== TcrSdk 回调函数（C 静态函数）====================

    /**
     * @brief 会话事件回调 - 处理连接状态、错误等事件
     * @param user_data 用户数据指针（SessionUserData*）
     * @param event 事件类型（如 TCR_SESSION_EVENT_STATE_CONNECTED）
     * @param eventData 事件数据（JSON 字符串）
     * 
     * 说明：通过 user_data 获取 ViewModel 对象和会话索引，
     *       使用 QMetaObject::invokeMethod 切换到主线程处理
     */
    static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData);

    /**
     * @brief 视频帧回调 - 接收解码后的视频帧
     * @param user_data 用户数据指针（SessionUserData*）
     * @param frame_handle 视频帧句柄
     * 
     * TcrSdk API 调用：
     * - tcr_video_frame_get_buffer()：获取帧缓冲区
     * - tcr_video_frame_add_ref()：增加引用计数
     * - tcr_video_frame_release()：释放引用（由智能指针自动调用）
     * 
     * 说明：
     * 1. 从 frame_buffer 中提取 instance_id 和 instance_index
     * 2. 构造唯一标识符 "instanceId_instanceIndex"
     * 3. 通过信号将帧数据发送到对应的 VideoRenderItem
     */
    static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle);
};