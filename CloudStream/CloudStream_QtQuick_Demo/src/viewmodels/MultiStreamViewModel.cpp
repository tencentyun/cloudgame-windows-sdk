#include "MultiStreamViewModel.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "utils/VariantListConverter.h"
#include <QVariant>
#include <QDebug>
#include <QMetaType>
#include <QGuiApplication>
#include <QClipboard>
#include <QJsonDocument>
#include <QJsonObject>
#include <QJsonArray>
#include <algorithm>

// ==================== 构造与析构 ====================

MultiStreamViewModel::MultiStreamViewModel(QObject *parent)
    : QObject(parent)
{
    Logger::info("[MultiStreamViewModel] 构造函数");

    // 注册元类型，确保跨线程信号槽可用
    qRegisterMetaType<VideoFrameData>("VideoFrameData");
    qRegisterMetaType<VideoFrameDataPtr>("VideoFrameDataPtr");
    
    // 初始化渲染定时器（60fps = 16ms间隔）
    m_renderTimer = new QTimer(this);
    m_renderTimer->setInterval(16);  // 可根据需要调整：16ms(60fps), 33ms(30fps)
    connect(m_renderTimer, &QTimer::timeout, this, &MultiStreamViewModel::batchRenderFrames);
    m_renderTimer->start();
}

MultiStreamViewModel::~MultiStreamViewModel()
{
    Logger::info("[MultiStreamViewModel] 开始析构");
    
    // ===== 第一层防护：设置析构标志，阻止新的回调处理 =====
    m_isDestroying.store(true, std::memory_order_release);
    
    // ===== 第二层防护：停止定时器，防止新的渲染任务 =====
    if (m_renderTimer) {
        m_renderTimer->stop();
    }
    
    // ===== 第三层防护：取消观察者并等待回调完成 =====
    // 重要：先清理所有观察者，防止回调继续执行
    // 这是使用 TcrSdk 的关键步骤，必须在销毁会话前取消观察者
    for (auto& sessionInfo : m_sessions) {
        if (sessionInfo.session) {
            tcr_session_set_observer(sessionInfo.session, nullptr);
            tcr_session_set_video_frame_observer(sessionInfo.session, nullptr);
        }
    }
    
    
    // ===== 第四层防护：加锁清理资源 =====
    {
        QMutexLocker locker(&m_videoRenderItemsMutex);
        m_videoRenderItems.clear();
    }
    
    // 清理帧缓存
    {
        QMutexLocker locker(&m_frameCacheMutex);
        m_frameCache.clear();
    }
    
    // 关闭所有会话
    closeAllSessions();
    
    tcr_client_release(tcr_client_get_instance());
    
    Logger::info("[MultiStreamViewModel] 析构完成");
}

// ==================== TcrSdk 初始化 ====================

void MultiStreamViewModel::initialize(const QStringList& instanceIds, 
                                     const QString& accessInfo, 
                                     const QString& token)
{
    Logger::info("[initialize] 开始初始化 TcrSdk");
    Logger::info(QString("[initialize] Instance IDs: %1").arg(instanceIds.join(", ")));
    Logger::info(QString("[initialize] Access Info: %1").arg(accessInfo));
    Logger::info(QString("[initialize] Token: %1").arg(token));

    // 将 Qt 字符串转换为 C 风格字符串
    std::string tokenStr = token.toStdString();
    std::string accessInfoStr = accessInfo.toStdString();

    // 步骤1：配置 TcrSdk 初始化参数
    // TcrConfig 包含两个必填字段：Token 和 AccessInfo
    TcrConfig config = tcr_config_default();
    config.token = tokenStr.c_str();
    config.accessInfo = accessInfoStr.c_str();
    
    // 步骤2：获取 TcrClient 全局单例句柄
    // tcr_client_get_instance() 返回全局唯一的 TcrClientHandle
    TcrClientHandle client = tcr_client_get_instance();
    
    // 步骤3：初始化 TcrClient
    // 注意：此函数可调用多次以更新配置，但只需成功初始化一次
    // 所有后续 TcrSdk 接口调用必须在此函数返回成功之后进行
    TcrErrorCode result = tcr_client_init(client, &config);
    if (result != TCR_SUCCESS) {
        Logger::error("[initialize] TcrSdk 初始化失败");
        return;
    }
    
    Logger::info("[initialize] TcrSdk 初始化成功");
}

// ==================== 状态查询 ====================

int MultiStreamViewModel::getInstanceConnectionState(const QString& instanceId) const
{
    if (m_instanceConnectionStates.contains(instanceId)) {
        return static_cast<int>(m_instanceConnectionStates[instanceId]);
    }
    return static_cast<int>(InstanceConnectionState::Disconnected);
}

QString MultiStreamViewModel::getInstanceStats(const QString& instanceId) const
{
    if (m_clientStats.isEmpty()) {
        return QString();
    }
    
    QJsonDocument doc = QJsonDocument::fromJson(m_clientStats.toUtf8());
    if (!doc.isObject()) {
        return QString();
    }
    
    QJsonObject root = doc.object();
    
    if (!root.contains("video_streams") || !root["video_streams"].isObject()) {
        return QString();
    }
    
    QJsonObject videoStreams = root["video_streams"].toObject();
    
    if (!videoStreams.contains(instanceId)) {
        return QString();
    }
    
    QJsonObject streamObj = videoStreams[instanceId].toObject();
    
    // 构建返回的 JSON 对象：包含全局统计 + 该实例的视频流统计
    QJsonObject result;
    
    // 复制全局统计数据（排除 video_streams）
    result["request_id"] = root["request_id"];
    result["session_id"] = root["session_id"];
    result["instance_id"] = instanceId;
    result["rtt"] = root["rtt"];
    result["raw_rtt"] = root["raw_rtt"];
    result["edge_rtt"] = root["edge_rtt"];
    
    // 直接复制该实例的完整视频流统计对象
    for (auto it = streamObj.begin(); it != streamObj.end(); ++it) {
        result[it.key()] = it.value();
    }
    
    QJsonDocument resultDoc(result);
    return QString::fromUtf8(resultDoc.toJson(QJsonDocument::Indented));
}

void MultiStreamViewModel::onVisibilityChanged(const QStringList& visibleIds, const QStringList& invisibleIds)
{
    Logger::info("=== 滚动停止 500ms 检测 ===");
    Logger::info(QString("可见实例 (%1): %2")
                .arg(visibleIds.length())
                .arg(visibleIds.join(", ")));
    Logger::info(QString("不可见实例 (%1): %2")
                .arg(invisibleIds.length())
                .arg(invisibleIds.join(", ")));
    
    // 过滤出真正需要恢复的实例（从不可见变为可见）
    QStringList idsToResume;
    for (const QString& id : visibleIds) {
        if (m_lastInvisibleIds.contains(id)) {
            idsToResume.append(id);
        }
    }
    
    // 过滤出真正需要暂停的实例（从可见变为不可见）
    QStringList idsToPause;
    for (const QString& id : invisibleIds) {
        if (m_lastVisibleIds.contains(id)) {
            idsToPause.append(id);
        }
    }
    
    Logger::info(QString("需要恢复的实例 (%1): %2")
                .arg(idsToResume.length())
                .arg(idsToResume.join(", ")));
    Logger::info(QString("需要暂停的实例 (%1): %2")
                .arg(idsToPause.length())
                .arg(idsToPause.join(", ")));
    
    // 对真正需要恢复的实例恢复流媒体
    if (!idsToResume.isEmpty()) {
        resumeStreaming(idsToResume);
    }
    
    // 对真正需要暂停的实例暂停流媒体
    if (!idsToPause.isEmpty()) {
        pauseStreaming(idsToPause);
    }
    
    // 更新上次的状态
    m_lastVisibleIds = visibleIds;
    m_lastInvisibleIds = invisibleIds;
}

// ==================== 视频渲染项注册 ====================

void MultiStreamViewModel::registerVideoRenderItem(const QString& instanceId,
                                                   QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderItem*>(item);
    if (!vrItem) {
        Logger::warning(QString("[registerVideoRenderItem] 类型转换失败: %1")
                       .arg(instanceId));
        return;
    }
    
    // 使用 instanceId 作为唯一标识
    // 加锁保护 m_videoRenderItems 的写操作
    {
        QMutexLocker locker(&m_videoRenderItemsMutex);
        m_videoRenderItems[instanceId] = vrItem;
    }
    
    Logger::debug(QString("[registerVideoRenderItem] 注册成功: %1").arg(instanceId));
}

// ==================== 批量渲染处理 ====================

void MultiStreamViewModel::batchRenderFrames()
{
    // 步骤1：加锁并交换缓存（最小化锁持有时间）
    QMap<QString, VideoFrameDataPtr> framesToRender;
    {
        QMutexLocker locker(&m_frameCacheMutex);
        if (m_frameCache.isEmpty()) {
            return;  // 没有待渲染的帧
        }
        framesToRender.swap(m_frameCache);  // 交换而非复制，高效
    }
    
    // 步骤2：批量渲染所有缓存的帧（需要加锁访问 m_videoRenderItems）
    int renderedCount = 0;
    for (auto it = framesToRender.begin(); it != framesToRender.end(); ++it) {
        const QString& instanceId = it.key();
        VideoFrameDataPtr frame = it.value();
        
        // 加锁检查渲染项是否仍然有效
        QPointer<VideoRenderItem> renderItem;
        {
            QMutexLocker locker(&m_videoRenderItemsMutex);
            if (m_videoRenderItems.contains(instanceId)) {
                renderItem = m_videoRenderItems[instanceId];
            }
        }
        
        // 在锁外执行渲染操作（避免长时间持锁）
        if (renderItem) {
            renderItem->setFrame(frame);
            renderedCount++;
        }
    }
}

// ==================== 多实例连接 ====================

void MultiStreamViewModel::connectMultipleInstances(const QVariantList& sessionConfigs)
{
    Logger::info(QString("[connectMultipleInstances] 开始连接 %1 个会话")
                .arg(sessionConfigs.size()));
    
    // 关闭现有会话（如果有）
    closeAllSessions();
    
    if (sessionConfigs.isEmpty()) {
        Logger::warning("[connectMultipleInstances] 会话配置列表为空");
        return;
    }
    
    // 确保 TcrClient 已初始化
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance();
    }
    
    // 创建会话并连接实例
    createSessionsWithConfigs(sessionConfigs);
    
    // 收集所有实例ID并设置为连接中状态
    QStringList allInstanceIds;
    for (const QVariant& sessionConfig : sessionConfigs) {
        QStringList sessionInstances = sessionConfig.toStringList();
        allInstanceIds.append(sessionInstances);
        
        // 设置为连接中状态
        for (const QString& instanceId : sessionInstances) {
            m_instanceConnectionStates[instanceId] = InstanceConnectionState::Connecting;
            emit instanceConnectionChanged(instanceId, false);
        }
    }
    
    m_connectedInstanceIds.clear();
    emit connectedInstanceIdsChanged();
}

void MultiStreamViewModel::createSessionsWithConfigs(const QVariantList& sessionConfigs)
{
    int sessionCount = sessionConfigs.size();
    Logger::info(QString("[createSessionsWithConfigs] 创建 %1 个会话").arg(sessionCount));
    
    m_sessions.resize(sessionCount);
    
    for (int i = 0; i < sessionCount; i++) {
        QStringList sessionInstanceIds = sessionConfigs[i].toStringList();
        
        if (sessionInstanceIds.isEmpty()) {
            Logger::warning(QString("[createSessionsWithConfigs] 会话 %1 的实例列表为空").arg(i));
            continue;
        }

        // ===== TcrSdk API 使用步骤 =====
        
        // 步骤1：创建会话配置
        // 使用 tcr_session_config_default() 获取默认配置，然后自定义需要的参数
        TcrSessionConfig config = tcr_session_config_default();
        
        // 从StreamConfig单例获取配置的串流参数
        StreamConfig* streamConfig = StreamConfig::instance();
        
        // 自定义视频流参数（使用大流参数）
        config.stream_profile.video_width = streamConfig->subStreamWidth();   // 指定短边的宽度
        config.stream_profile.fps = streamConfig->subStreamFps();             // 帧率
        config.stream_profile.max_bitrate = streamConfig->subStreamMaxBitrate();   // 最大码率
        config.stream_profile.min_bitrate = streamConfig->subStreamMinBitrate();    // 最小码率
        config.enable_audio = false; 
        
        Logger::info(QString("[createSessionsWithConfigs] 使用串流参数 - 宽度:%1, 帧率:%2, 码率:%3-%4")
                    .arg(config.stream_profile.video_width)
                    .arg(config.stream_profile.fps)
                    .arg(config.stream_profile.min_bitrate)
                    .arg(config.stream_profile.max_bitrate));
        
        // 步骤2：创建会话
        // tcr_client_create_session() 返回会话句柄，用于后续所有会话操作
        m_sessions[i].session = tcr_client_create_session(m_tcrClient, &config);
        m_sessions[i].instanceIds = sessionInstanceIds;
        
        if (!m_sessions[i].session) {
            Logger::error(QString("[createSessionsWithConfigs] 会话 %1 创建失败").arg(i));
            continue;
        }
        
        Logger::info(QString("[createSessionsWithConfigs] 会话 %1 创建成功，实例: %2")
                    .arg(i).arg(sessionInstanceIds.join(",")));
        
        // 步骤3：初始化用户数据和观察者结构体
        // 用户数据用于在 C 回调函数中传递上下文信息
        m_sessions[i].userData.viewModel = this;
        m_sessions[i].userData.sessionIndex = i;
        
        // 初始化会话事件观察者
        m_sessions[i].sessionObserver.user_data = &m_sessions[i].userData;
        m_sessions[i].sessionObserver.on_event = &MultiStreamViewModel::SessionEventCallback;
        
        // 初始化视频帧观察者
        m_sessions[i].videoFrameObserver.user_data = &m_sessions[i].userData;
        m_sessions[i].videoFrameObserver.on_frame = &MultiStreamViewModel::VideoFrameCallback;
        
        // 步骤4：设置观察者
        // 必须在连接实例前设置，否则无法接收回调
        setSessionObservers(i);
        
        // 步骤5：连接实例
        // tcr_session_access() 启动会话并连接到指定的云手机实例
        auto result = VariantListConverter::convert(sessionInstanceIds);
        if (!result.pointers.empty()) {
            // tcr_session_access(
            //     m_sessions[i].session,
            //     result.pointers.data(),
            //     static_cast<int32_t>(result.pointers.size()),
            //     false
            // );

            // tcr_session_access_multi_stream(
            //     m_sessions[i].session,
            //     result.pointers.data(),
            //     static_cast<int32_t>(result.pointers.size())
            // );
            tcr_session_access_multi_stream(
                m_sessions[i].session,
                result.pointers.data(),
                static_cast<int32_t>(result.pointers.size()),
                result.pointers.data(),
                static_cast<int32_t>(result.pointers.size())
            );
            Logger::debug(QString("[createSessionsWithConfigs] 会话 %1 开始连接实例").arg(i));
        }
    }
}

void MultiStreamViewModel::setSessionObservers(int sessionIndex)
{
    if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
        return;
    }
    
    if (!m_sessions[sessionIndex].session) {
        return;
    }
    
    // 设置会话事件观察者 - 接收连接状态、错误等事件
    tcr_session_set_observer(
        m_sessions[sessionIndex].session, 
        &m_sessions[sessionIndex].sessionObserver
    );
    
    // 设置视频帧观察者 - 接收解码后的视频帧
    tcr_session_set_video_frame_observer(
        m_sessions[sessionIndex].session, 
        &m_sessions[sessionIndex].videoFrameObserver
    );
}

// ==================== 会话关闭 ====================

void MultiStreamViewModel::closeAllSessions()
{
    Logger::info("[closeAllSessions] 开始关闭所有会话");
    
    for (auto& sessionInfo : m_sessions) {
        if (sessionInfo.session) {
            // 重要：必须先取消观察者，再销毁会话
            // 这是 TcrSdk 的正确使用方式，防止回调访问已释放的内存
            tcr_session_set_observer(sessionInfo.session, nullptr);
            tcr_session_set_video_frame_observer(sessionInfo.session, nullptr);
            
            // 销毁会话
            if (m_tcrClient) {
                tcr_client_destroy_session(m_tcrClient, sessionInfo.session);
            }
            sessionInfo.session = nullptr;
        }
    }
    
    m_sessions.clear();
    m_connectedInstanceIds.clear();
    m_instanceConnectionStates.clear();
    emit connectedInstanceIdsChanged();
    
    Logger::info("[closeAllSessions] 所有会话已关闭");
}

void MultiStreamViewModel::pauseStreaming(const QStringList& instanceIds)
{
    Logger::info(QString("[pauseStreaming] 暂停流媒体，实例数: %1").arg(instanceIds.size()));
    
    if (instanceIds.isEmpty()) {
        Logger::warning("[pauseStreaming] 实例ID列表为空");
        return;
    }
    
    // 转换 QStringList 到 C 字符串数组
    auto result = VariantListConverter::convert(instanceIds);
    if (result.pointers.empty()) {
        Logger::error("[pauseStreaming] 转换实例ID列表失败");
        return;
    }
    
    // 注意：为了简单演示功能，这里只对第一个会话调用暂停接口
    // 实际应用中需要根据实例ID查找对应的会话
    if (m_sessions.empty()) {
        Logger::warning("[pauseStreaming] 没有可用的会话");
        return;
    }
    
    const auto& sessionInfo = m_sessions[0];  // 只使用第一个会话
    if (sessionInfo.session) {
        // 调用 TcrSdk API 暂停流媒体
        // 参数说明：
        // - session: 会话句柄
        // - media_type: nullptr 表示暂停音视频流
        // - instanceIds: 实例ID数组
        // - instance_count: 实例数量
        tcr_session_pause_streaming(
            sessionInfo.session,
            nullptr,  // 暂停音视频流
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        
        Logger::info(QString("[pauseStreaming] 会话 0 已暂停流媒体，实例: %1")
                    .arg(instanceIds.join(",")));
    }
}

void MultiStreamViewModel::resumeStreaming(const QStringList& instanceIds)
{
    Logger::info(QString("[resumeStreaming] 恢复流媒体，实例数: %1").arg(instanceIds.size()));
    
    if (instanceIds.isEmpty()) {
        Logger::warning("[resumeStreaming] 实例ID列表为空");
        return;
    }
    
    // 转换 QStringList 到 C 字符串数组
    auto result = VariantListConverter::convert(instanceIds);
    if (result.pointers.empty()) {
        Logger::error("[resumeStreaming] 转换实例ID列表失败");
        return;
    }
    
    // 注意：为了简单演示功能，这里只对第一个会话调用恢复接口
    // 实际应用中需要根据实例ID查找对应的会话
    if (m_sessions.empty()) {
        Logger::warning("[resumeStreaming] 没有可用的会话");
        return;
    }
    
    const auto& sessionInfo = m_sessions[0];  // 只使用第一个会话
    if (sessionInfo.session) {
        // 调用 TcrSdk API 恢复流媒体
        // 参数说明：
        // - session: 会话句柄
        // - media_type: nullptr 表示恢复音视频流
        // - instanceIds: 实例ID数组
        // - instance_count: 实例数量
        tcr_session_resume_streaming(
            sessionInfo.session,
            nullptr,  // 恢复音视频流
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        
        Logger::info(QString("[resumeStreaming] 会话 0 已恢复流媒体，实例: %1")
                    .arg(instanceIds.join(",")));
    }
}

// ==================== TcrSdk 回调函数实现 ====================

void MultiStreamViewModel::SessionEventCallback(void* user_data,
                                               TcrSessionEvent event,
                                               const char* eventData)
{
    // 步骤1：从 user_data 中恢复上下文信息
    SessionUserData* userData = static_cast<SessionUserData*>(user_data);
    MultiStreamViewModel* self = userData->viewModel;
    int sessionIndex = userData->sessionIndex;
    
    // 步骤2：复制事件数据（避免生命周期问题）
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();

    // 步骤3：切换到主线程处理事件
    // 回调函数可能在非主线程执行，使用 QMetaObject::invokeMethod 确保线程安全
    QMetaObject::invokeMethod(self, [self, sessionIndex, event, eventDataCopy]() {
        // 验证会话索引有效性
        if (sessionIndex < 0 || sessionIndex >= self->m_sessions.size()) {
            Logger::warning(QString("[SessionEventCallback] 无效的会话索引: %1").arg(sessionIndex));
            return;
        }
        
        SessionInfo& sessionInfo = self->m_sessions[sessionIndex];
        
        // 处理不同的事件类型
        switch (event) {
            case TCR_SESSION_EVENT_STATE_CONNECTED: {
                // 会话连接成功
                Logger::info(QString("[SessionEventCallback] 会话 %1 连接成功，实例: %2")
                            .arg(sessionIndex).arg(sessionInfo.instanceIds.join(",")));
                
                sessionInfo.connected = true;
                
                // 更新为已连接状态
                for (const QString& instanceId : sessionInfo.instanceIds) {
                    self->m_instanceConnectionStates[instanceId] = InstanceConnectionState::Connected;
                    
                    // 添加到已连接列表
                    if (!self->m_connectedInstanceIds.contains(instanceId)) {
                        self->m_connectedInstanceIds.append(instanceId);
                    }
                    
                    emit self->instanceConnectionChanged(instanceId, true);
                }
                
                emit self->connectedInstanceIdsChanged();
                break;
            }
                
            case TCR_SESSION_EVENT_STATE_CLOSED:
                // 会话关闭
                Logger::error(QString("[SessionEventCallback] 会话 %1 断开: %2")
                             .arg(sessionIndex).arg(eventDataCopy));
                
                sessionInfo.connected = false;
                
                // 更新为未连接状态并从已连接列表中移除
                for (const QString& instanceId : sessionInfo.instanceIds) {
                    self->m_instanceConnectionStates[instanceId] = InstanceConnectionState::Disconnected;
                    self->m_connectedInstanceIds.removeAll(instanceId);
                    emit self->instanceConnectionChanged(instanceId, false);
                }
                
                // 触发列表变化信号，更新QML界面
                emit self->connectedInstanceIdsChanged();
                
                // 发送会话关闭信号（用于弹出对话框）
                emit self->sessionClosed(sessionIndex, sessionInfo.instanceIds, eventDataCopy);
                break;
            case TCR_SESSION_EVENT_CLIENT_STATS: // 更新统计数据（包含帧率、码率、延迟等信息）
                self->m_clientStats = eventDataCopy;
                emit self->clientStatsChanged();
                break;
        }
    }, Qt::QueuedConnection);
}

void MultiStreamViewModel::VideoFrameCallback(void* user_data,
                                             TcrVideoFrameHandle frame_handle)
{
    // ===== 步骤1：参数有效性检查 =====
    if (!user_data || !frame_handle) {
        Logger::warning("[VideoFrameCallback] 无效参数");
        return;
    }
    
    SessionUserData* userData = static_cast<SessionUserData*>(user_data);
    if (!userData || !userData->viewModel) {
        Logger::warning("[VideoFrameCallback] 无效的 user_data");
        return;
    }
    
    MultiStreamViewModel* self = userData->viewModel;
    
    // ===== 步骤2：快速检测析构状态（原子操作，无锁） =====
    // 使用 memory_order_acquire 确保看到 store 之前的所有写操作
    if (self->m_isDestroying.load(std::memory_order_acquire)) {
        // 对象正在析构，立即返回，不处理任何数据
        Logger::warning("[VideoFrameCallback] 对象正在析构，不处理数据");
        return;
    }
    
    int sessionIndex = userData->sessionIndex;
    
    // ===== 步骤3：获取视频帧缓冲区 =====
    // tcr_video_frame_get_buffer() 返回帧数据的只读指针
    const TcrVideoFrameBuffer* frame_buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!frame_buffer) {
        Logger::warning("[VideoFrameCallback] 无效的视频帧缓冲区");
        return;
    }

    // ===== 步骤4：立即复制字符串数据（防止指针失效）=====
    // 重要：instance_id 指针来自 TcrSdk 内部，可能随时失效
    // 必须在使用前立即复制到 QString 中
    QString instanceId;
    
    try {
        // 防御性编程：检查指针有效性
        if (frame_buffer->instance_id) {
            instanceId = QString::fromUtf8(frame_buffer->instance_id);
        } else {
            Logger::warning("[VideoFrameCallback] instance_id 为空指针");
            return;
        }
    } catch (...) {
        Logger::error("[VideoFrameCallback] 复制 instance_id 时发生异常");
        return;
    }
    
    // ===== 步骤5：验证实例是否属于当前会话 =====
    if (sessionIndex >= 0 && sessionIndex < self->m_sessions.size()) {
        const SessionInfo& sessionInfo = self->m_sessions[sessionIndex];
        if (!sessionInfo.instanceIds.contains(instanceId)) {
            Logger::warning(QString("[VideoFrameCallback] 实例 %1 不属于会话 %2")
                           .arg(instanceId).arg(sessionIndex));
            tcr_video_frame_release(frame_handle);
            return;
        }
    }
    
    // ===== 步骤6：加锁检查渲染项是否存在且有效 =====
    bool hasValidRenderItem = false;
    {
        QMutexLocker locker(&self->m_videoRenderItemsMutex);
        
        // 再次检查析构状态（双重检查）
        if (self->m_isDestroying.load(std::memory_order_acquire)) {
            Logger::warning("[VideoFrameCallback] 对象正在析构，不处理数据");
            return;
        }
        
        // 检查是否有对应的渲染项
        if (self->m_videoRenderItems.contains(instanceId)) {
            QPointer<VideoRenderItem> renderItem = self->m_videoRenderItems[instanceId];
            if (renderItem) {
                hasValidRenderItem = true;
            } else {
                // 对象已被销毁，从映射中移除
                self->m_videoRenderItems.remove(instanceId);
            }
        }
    }
    // 锁已释放
    
    // 如果没有有效的渲染项，直接返回
    if (!hasValidRenderItem) {
        tcr_video_frame_release(frame_handle);
        return;
    }

    // 步骤6：增加引用计数
    // 重要：必须调用 tcr_video_frame_add_ref() 增加引用计数
    // 否则帧数据可能在使用前被释放
    tcr_video_frame_add_ref(frame_handle);
    
    // 步骤7：根据缓冲区类型创建视频帧数据智能指针
    VideoFrameDataPtr frameDataPtr;
    
    if (frame_buffer->type == TCR_VIDEO_BUFFER_TYPE_I420) {
        // I420格式：CPU内存中的YUV数据
        const TcrI420Buffer& i420Buffer = frame_buffer->buffer.i420;
        
        frameDataPtr.reset(new VideoFrameData(
            frame_handle,
            i420Buffer.data_y,
            i420Buffer.data_u,
            i420Buffer.data_v,
            i420Buffer.stride_y,
            i420Buffer.stride_u,
            i420Buffer.stride_v,
            i420Buffer.width,
            i420Buffer.height,
            frame_buffer->timestamp_us
        ));
    }
    else if (frame_buffer->type == TCR_VIDEO_BUFFER_TYPE_D3D11) {
        // D3D11格式：GPU纹理数据
        const TcrD3D11Buffer& d3d11Buffer = frame_buffer->buffer.d3d11;
        
        D3D11TextureData textureData;
        textureData.texture = d3d11Buffer.texture;
        textureData.device = d3d11Buffer.device;
        textureData.array_index = d3d11Buffer.array_index;
        textureData.format = d3d11Buffer.format;
        
        frameDataPtr.reset(new VideoFrameData(
            frame_handle,
            textureData,
            d3d11Buffer.width,
            d3d11Buffer.height,
            frame_buffer->timestamp_us
        ));
    }
    else {
        // 未知类型，释放引用并返回
        Logger::warning(QString("[VideoFrameCallback] 未知的帧类型: %1").arg(frame_buffer->type));
        tcr_video_frame_release(frame_handle);
        return;
    }


    // 步骤9：将帧存入缓存
    // 使用互斥锁保护缓存，只保留每个实例的最新帧
    {
        QMutexLocker locker(&self->m_frameCacheMutex);
        
        // 如果已有旧帧，会自动释放（智能指针）
        self->m_frameCache[instanceId] = frameDataPtr;
    }
}

void MultiStreamViewModel::copyToClipboard(const QString& text)
{
    QClipboard* clipboard = QGuiApplication::clipboard();
    if (clipboard) {
        clipboard->setText(text);
        qDebug() << "Text copied to clipboard:" << text;
    } else {
        qWarning() << "Failed to access clipboard";
    }
}
