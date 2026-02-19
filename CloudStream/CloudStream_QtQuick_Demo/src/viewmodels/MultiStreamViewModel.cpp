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
    if (m_session) {
        tcr_session_set_observer(m_session, nullptr);
        tcr_session_set_video_frame_observer(m_session, nullptr);
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
    
    // 关闭会话
    closeSession();
    
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
    TcrConfig config = tcr_config_default();
    config.token = tokenStr.c_str();
    config.accessInfo = accessInfoStr.c_str();
    
    // 步骤2：获取 TcrClient 全局单例句柄
    TcrClientHandle client = tcr_client_get_instance();
    
    // 步骤3：初始化 TcrClient
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
    
    // 构建返回的 JSON 对象
    QJsonObject result;
    
    result["request_id"] = root["request_id"];
    result["session_id"] = root["session_id"];
    result["instance_id"] = instanceId;
    result["rtt"] = root["rtt"];
    result["raw_rtt"] = root["raw_rtt"];
    result["edge_rtt"] = root["edge_rtt"];
    
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
    
    // 检查是否有变化
    QSet<QString> newVisibleSet = QSet<QString>(visibleIds.begin(), visibleIds.end());
    QSet<QString> currentStreamingSet = QSet<QString>(m_currentStreamingIds.begin(), m_currentStreamingIds.end());
    
    if (newVisibleSet == currentStreamingSet) {
        Logger::info("[onVisibilityChanged] 可见实例列表无变化，跳过切换");
        return;
    }
    
    // 使用 tcr_session_switch_streaming_instances 动态切换拉流实例
    switchStreamingInstances(visibleIds);
}

// ==================== 切换拉流实例 ====================

void MultiStreamViewModel::switchStreamingInstances(const QStringList& streamingIds)
{
    Logger::info(QString("[switchStreamingInstances] 切换拉流实例: %1 个").arg(streamingIds.size()));
    
    if (!m_session) {
        Logger::warning("[switchStreamingInstances] 没有可用的session");
        return;
    }
    
    // 检查是否有变化
    QSet<QString> newSet(streamingIds.begin(), streamingIds.end());
    QSet<QString> currentSet(m_currentStreamingIds.begin(), m_currentStreamingIds.end());
    
    if (newSet == currentSet) {
        Logger::debug("[switchStreamingInstances] 无变化，跳过");
        return;
    }
    
    // 验证要切换的实例是否都在 allInstanceIds 中
    QStringList validIds;
    QSet<QString> allIdsSet(m_allInstanceIds.begin(), m_allInstanceIds.end());
    for (const QString& id : streamingIds) {
        if (allIdsSet.contains(id)) {
            validIds.append(id);
        } else {
            Logger::warning(QString("[switchStreamingInstances] 实例 %1 不在实例列表中").arg(id));
        }
    }
    
    if (validIds.isEmpty()) {
        Logger::debug("[switchStreamingInstances] 没有有效的切换实例");
        return;
    }
    
    // 转换为 C 字符串数组并调用 SDK
    auto result = VariantListConverter::convert(validIds);
    
    tcr_session_switch_streaming_instances(
        m_session,
        result.pointers.data(),
        static_cast<int32_t>(result.pointers.size())
    );
    
    // 更新当前拉流实例列表
    m_currentStreamingIds = validIds;
    
    Logger::info(QString("[switchStreamingInstances] 已切换到 %1 个实例").arg(validIds.size()));
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
    
    {
        QMutexLocker locker(&m_videoRenderItemsMutex);
        m_videoRenderItems[instanceId] = vrItem;
    }
    
    Logger::debug(QString("[registerVideoRenderItem] 注册成功: %1").arg(instanceId));
}

// ==================== 批量渲染处理 ====================

void MultiStreamViewModel::batchRenderFrames()
{
    QMap<QString, VideoFrameDataPtr> framesToRender;
    {
        QMutexLocker locker(&m_frameCacheMutex);
        if (m_frameCache.isEmpty()) {
            return;
        }
        framesToRender.swap(m_frameCache);
    }
    
    int renderedCount = 0;
    for (auto it = framesToRender.begin(); it != framesToRender.end(); ++it) {
        const QString& instanceId = it.key();
        VideoFrameDataPtr frame = it.value();
        
        QPointer<VideoRenderItem> renderItem;
        {
            QMutexLocker locker(&m_videoRenderItemsMutex);
            if (m_videoRenderItems.contains(instanceId)) {
                renderItem = m_videoRenderItems[instanceId];
            }
        }
        
        if (renderItem) {
            renderItem->setFrame(frame);
            renderedCount++;
        }
    }
}

// ==================== 多实例连接 ====================

void MultiStreamViewModel::connectMultipleInstances(const QStringList& allInstanceIds, int concurrentStreamingInstances)
{
    Logger::info(QString("[connectMultipleInstances] 开始连接 %1 个实例，同时串流数量: %2")
                .arg(allInstanceIds.size())
                .arg(concurrentStreamingInstances));
    
    // 关闭现有会话（如果有）
    closeSession();
    
    if (allInstanceIds.isEmpty()) {
        Logger::warning("[connectMultipleInstances] 实例ID列表为空");
        return;
    }
    
    // 保存所有实例ID和并发数
    m_allInstanceIds = allInstanceIds;
    m_concurrentStreamingInstances = concurrentStreamingInstances;
    
    // 确保 TcrClient 已初始化
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance();
    }
    
    // ===== TcrSdk API 使用步骤 =====
    
    // 步骤1：创建会话配置
    TcrSessionConfig config = tcr_session_config_default();
    
    // 从StreamConfig单例获取配置的串流参数
    StreamConfig* streamConfig = StreamConfig::instance();
    
    // 自定义视频流参数
    config.stream_profile.video_width = streamConfig->subStreamWidth();
    config.stream_profile.fps = streamConfig->subStreamFps();
    config.stream_profile.max_bitrate = streamConfig->subStreamMaxBitrate();
    config.stream_profile.min_bitrate = streamConfig->subStreamMinBitrate();
    config.enable_audio = false;
    
    // 设置并发拉流实例数量
    config.concurrentStreamingInstances = concurrentStreamingInstances;
    Logger::info(QString("[connectMultipleInstances] 使用串流参数 - 宽度:%1, 帧率:%2, 码率:%3-%4, 同时串流数量:%5")
                .arg(config.stream_profile.video_width)
                .arg(config.stream_profile.fps)
                .arg(config.stream_profile.min_bitrate)
                .arg(config.stream_profile.max_bitrate)
                .arg(config.concurrentStreamingInstances));
    
    // 步骤2：创建会话
    m_session = tcr_client_create_session(m_tcrClient, &config);
    
    if (!m_session) {
        Logger::error("[connectMultipleInstances] Session 创建失败");
        return;
    }
    
    Logger::info(QString("[connectMultipleInstances] Session 创建成功，管理 %1 个实例")
                .arg(allInstanceIds.size()));
    
    // 步骤3：初始化用户数据和观察者结构体（堆分配，确保生命周期）
    m_userData = new SessionUserData();
    m_userData->viewModel = this;
    
    // 初始化会话事件观察者
    m_sessionObserver.user_data = m_userData;
    m_sessionObserver.on_event = &MultiStreamViewModel::SessionEventCallback;
    
    // 初始化视频帧观察者
    m_videoFrameObserver.user_data = m_userData;
    m_videoFrameObserver.on_frame = &MultiStreamViewModel::VideoFrameCallback;
    
    // 步骤4：设置观察者
    setSessionObservers();
    
    // 步骤5：连接所有实例（使用 tcr_session_access_multi_stream）
    auto result = VariantListConverter::convert(allInstanceIds);
    if (!result.pointers.empty()) {
        tcr_session_access_multi_stream(
            m_session,
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        Logger::debug(QString("[connectMultipleInstances] 开始连接实例（多流模式），传入 %1 个实例")
                     .arg(result.pointers.size()));
    }
    
    // 设置所有实例为连接中状态
    for (const QString& instanceId : allInstanceIds) {
        m_instanceConnectionStates[instanceId] = InstanceConnectionState::Connecting;
        emit instanceConnectionChanged(instanceId, false);
    }
    
    m_connectedInstanceIds.clear();
    m_currentStreamingIds.clear();
    emit connectedInstanceIdsChanged();
    
    Logger::info(QString("[connectMultipleInstances] 已创建单个Session，管理 %1 个实例")
                .arg(allInstanceIds.size()));
}

void MultiStreamViewModel::setSessionObservers()
{
    if (!m_session) {
        return;
    }
    
    // 设置会话事件观察者
    tcr_session_set_observer(m_session, &m_sessionObserver);
    
    // 设置视频帧观察者
    tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
}

// ==================== 会话关闭 ====================

void MultiStreamViewModel::closeSession()
{
    Logger::info("[closeSession] 开始关闭会话");
    
    if (m_session) {
        // 重要：必须先取消观察者，再销毁会话
        tcr_session_set_observer(m_session, nullptr);
        tcr_session_set_video_frame_observer(m_session, nullptr);
        
        // 销毁会话
        if (m_tcrClient) {
            tcr_client_destroy_session(m_tcrClient, m_session);
        }
        m_session = nullptr;
    }
    
    // 释放用户数据
    if (m_userData) {
        delete m_userData;
        m_userData = nullptr;
    }
    
    m_allInstanceIds.clear();
    m_connectedInstanceIds.clear();
    m_currentStreamingIds.clear();
    m_instanceConnectionStates.clear();
    m_concurrentStreamingInstances = 0;
    m_isConnected = false;
    emit connectedInstanceIdsChanged();
    
    Logger::info("[closeSession] 会话已关闭");
}

void MultiStreamViewModel::pauseStreaming(const QStringList& instanceIds)
{
    Logger::info(QString("[pauseStreaming] 暂停流媒体，实例数: %1").arg(instanceIds.size()));
    
    if (instanceIds.isEmpty()) {
        Logger::warning("[pauseStreaming] 实例ID列表为空");
        return;
    }
    
    if (!m_session) {
        Logger::warning("[pauseStreaming] 没有可用的session");
        return;
    }
    
    auto result = VariantListConverter::convert(instanceIds);
    if (result.pointers.empty()) {
        return;
    }
    
    tcr_session_pause_streaming(
        m_session,
        nullptr,
        result.pointers.data(),
        static_cast<int32_t>(result.pointers.size())
    );
    
    Logger::info(QString("[pauseStreaming] 已暂停流媒体，实例: %1").arg(instanceIds.join(",")));
}

void MultiStreamViewModel::resumeStreaming(const QStringList& instanceIds)
{
    Logger::info(QString("[resumeStreaming] 恢复流媒体，实例数: %1").arg(instanceIds.size()));
    
    if (instanceIds.isEmpty()) {
        Logger::warning("[resumeStreaming] 实例ID列表为空");
        return;
    }
    
    if (!m_session) {
        Logger::warning("[resumeStreaming] 没有可用的session");
        return;
    }
    
    auto result = VariantListConverter::convert(instanceIds);
    if (result.pointers.empty()) {
        return;
    }
    
    tcr_session_resume_streaming(
        m_session,
        nullptr,
        result.pointers.data(),
        static_cast<int32_t>(result.pointers.size())
    );
    
    Logger::info(QString("[resumeStreaming] 已恢复流媒体，实例: %1").arg(instanceIds.join(",")));
}

// ==================== TcrSdk 回调函数实现 ====================

void MultiStreamViewModel::SessionEventCallback(void* user_data,
                                               TcrSessionEvent event,
                                               const char* eventData)
{
    SessionUserData* userData = static_cast<SessionUserData*>(user_data);
    if (!userData || !userData->viewModel) {
        return;
    }
    
    MultiStreamViewModel* self = userData->viewModel;
    
    // 检查是否正在销毁
    if (self->m_isDestroying.load(std::memory_order_acquire)) {
        return;
    }
    
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();
    
    // 处理不同的事件类型
    switch (event) {
        case TCR_SESSION_EVENT_STATE_CONNECTED: {
            Logger::info(QString("[SessionEventCallback] Session 连接成功，管理 %1 个实例")
                        .arg(self->m_allInstanceIds.size()));
            
            self->m_isConnected = true;
            
            // 更新所有实例为已连接状态
            for (const QString& instanceId : self->m_allInstanceIds) {
                self->m_instanceConnectionStates[instanceId] = InstanceConnectionState::Connected;
                
                if (!self->m_connectedInstanceIds.contains(instanceId)) {
                    self->m_connectedInstanceIds.append(instanceId);
                }
                
                emit self->instanceConnectionChanged(instanceId, true);
            }
            
            emit self->connectedInstanceIdsChanged();
            break;
        }
            
        case TCR_SESSION_EVENT_STATE_CLOSED:
            Logger::error(QString("[SessionEventCallback] Session 断开: %1").arg(eventDataCopy));
            
            self->m_isConnected = false;
            
            // 更新所有实例为未连接状态
            for (const QString& instanceId : self->m_allInstanceIds) {
                self->m_instanceConnectionStates[instanceId] = InstanceConnectionState::Disconnected;
                self->m_connectedInstanceIds.removeAll(instanceId);
                emit self->instanceConnectionChanged(instanceId, false);
            }
            
            emit self->connectedInstanceIdsChanged();
            emit self->sessionClosed(self->m_allInstanceIds, eventDataCopy);
            break;
            
        case TCR_SESSION_EVENT_CLIENT_STATS:
            self->m_clientStats = eventDataCopy;
            emit self->clientStatsChanged();
            break;
            
        default:
            Logger::debug(QString("[SessionEventCallback] 收到事件: %1, 数据: %2")
                         .arg(event)
                         .arg(eventDataCopy));
            break;
    }
}

void MultiStreamViewModel::VideoFrameCallback(void* user_data,
                                             TcrVideoFrameHandle frame_handle)
{
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
    
    if (self->m_isDestroying.load(std::memory_order_acquire)) {
        return;
    }
    
    const TcrVideoFrameBuffer* frame_buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!frame_buffer) {
        Logger::warning("[VideoFrameCallback] 无效的视频帧缓冲区");
        return;
    }

    QString instanceId;
    
    try {
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
    
    // 验证实例是否在 allInstanceIds 中
    if (!self->m_allInstanceIds.contains(instanceId)) {
        Logger::warning(QString("[VideoFrameCallback] 实例 %1 不在实例列表中").arg(instanceId));
        return;
    }
    
    bool hasValidRenderItem = false;
    {
        QMutexLocker locker(&self->m_videoRenderItemsMutex);
        
        if (self->m_isDestroying.load(std::memory_order_acquire)) {
            return;
        }
        
        if (self->m_videoRenderItems.contains(instanceId)) {
            QPointer<VideoRenderItem> renderItem = self->m_videoRenderItems[instanceId];
            if (renderItem) {
                hasValidRenderItem = true;
            } else {
                self->m_videoRenderItems.remove(instanceId);
            }
        }
    }
    
    if (!hasValidRenderItem) {
        return;
    }

    tcr_video_frame_add_ref(frame_handle);

    VideoFrameDataPtr frameDataPtr;

    if (frame_buffer->type == TCR_VIDEO_BUFFER_TYPE_I420) {
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
#ifdef _WIN32
    else if (frame_buffer->type == TCR_VIDEO_BUFFER_TYPE_D3D11) {
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
#endif
    else {
        Logger::warning(QString("[VideoFrameCallback] 未知的帧类型: %1").arg(frame_buffer->type));
        tcr_video_frame_release(frame_handle);
        return;
    }

    {
        QMutexLocker locker(&self->m_frameCacheMutex);
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
