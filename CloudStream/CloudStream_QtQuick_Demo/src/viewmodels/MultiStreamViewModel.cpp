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
#include <cmath>

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
    
    // ===== 第三层防护：取消所有session的观察者并等待回调完成 =====
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
    
    // 关闭会话
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
    
    // 更新上次的状态
    m_lastVisibleIds = visibleIds;
    m_lastInvisibleIds = invisibleIds;
}

// ==================== 计算session数量 ====================

int MultiStreamViewModel::calculateSessionCount(int totalInstances, int concurrentInstances)
{
    // 如果并发实例数不超过限制，只需要1个session
    if (concurrentInstances <= MAX_CONCURRENT_INSTANCES_PER_SESSION) {
        return 1;
    }
    
    // 计算需要的session数量：向上取整(concurrentInstances / MAX)
    int sessionCount = static_cast<int>(std::ceil(static_cast<double>(concurrentInstances) / MAX_CONCURRENT_INSTANCES_PER_SESSION));
    
    Logger::info(QString("[calculateSessionCount] 并发实例数: %1, 需要session数量: %2")
                .arg(concurrentInstances)
                .arg(sessionCount));
    
    return sessionCount;
}

// ==================== 分配实例到多个session ====================

QVector<QStringList> MultiStreamViewModel::reorderInstancesForSessions(const QStringList& allInstanceIds, int sessionCount)
{
    QVector<QStringList> result(sessionCount);
    
    if (sessionCount <= 0 || allInstanceIds.isEmpty()) {
        return result;
    }
    
    int totalInstances = allInstanceIds.size();
    int instancesPerSession = MAX_CONCURRENT_INSTANCES_PER_SESSION;  // 每个session默认拉100个
    
    // 为每个session创建重新排序的实例列表
    // 策略：Session i 的前100个实例是全局的第 i*100 到 (i+1)*100-1 个
    for (int i = 0; i < sessionCount; ++i) {
        QStringList& sessionList = result[i];
        
        // 计算这个session"优先"的实例范围
        int priorityStart = i * instancesPerSession;
        int priorityEnd = qMin(priorityStart + instancesPerSession, totalInstances);
        
        // 先添加优先的实例（这些会被默认拉流）
        for (int j = priorityStart; j < priorityEnd; ++j) {
            sessionList.append(allInstanceIds[j]);
        }
        
        // 再添加剩余的实例（保持顺序，先添加优先范围之后的，再添加之前的）
        // 优先范围之后的
        for (int j = priorityEnd; j < totalInstances; ++j) {
            sessionList.append(allInstanceIds[j]);
        }
        // 优先范围之前的
        for (int j = 0; j < priorityStart; ++j) {
            sessionList.append(allInstanceIds[j]);
        }
        
        Logger::info(QString("[reorderInstancesForSessions] Session %1 实例顺序: 前%2个优先 (索引%3-%4), 总计%5个")
                    .arg(i)
                    .arg(priorityEnd - priorityStart)
                    .arg(priorityStart)
                    .arg(priorityEnd - 1)
                    .arg(sessionList.size()));
    }
    
    return result;
}

QVector<QStringList> MultiStreamViewModel::distributeVisibleInstancesToSessions(const QStringList& visibleIds)
{
    int sessionCount = m_sessions.size();
    QVector<QStringList> result(sessionCount);
    
    if (sessionCount <= 0 || visibleIds.isEmpty()) {
        return result;
    }
    
    // 将可见实例均匀分配到各个session
    // 策略：按顺序分配，第i个实例分配给第 (i % sessionCount) 个session
    // 这样可以确保每个session分配到的实例数量接近 visibleIds.size() / sessionCount
    
    // 更好的策略：按块分配，前100个给session0，接下来100个给session1...
    int instancesPerSession = MAX_CONCURRENT_INSTANCES_PER_SESSION;
    
    for (int i = 0; i < visibleIds.size(); ++i) {
        int sessionIndex = i / instancesPerSession;
        if (sessionIndex >= sessionCount) {
            sessionIndex = sessionCount - 1;  // 超出的都放到最后一个session
        }
        result[sessionIndex].append(visibleIds[i]);
    }
    
    // 打印分配结果
    for (int i = 0; i < sessionCount; ++i) {
        Logger::debug(QString("[distributeVisibleInstancesToSessions] Session %1 分配到 %2 个可见实例")
                    .arg(i)
                    .arg(result[i].size()));
    }
    
    return result;
}

// ==================== 查找实例所属session ====================

int MultiStreamViewModel::findSessionIndexForInstance(const QString& instanceId) const
{
    return m_instanceToSessionIndex.value(instanceId, -1);
}

// ==================== 切换拉流实例 ====================

void MultiStreamViewModel::switchStreamingInstances(const QStringList& streamingIds)
{
    Logger::info(QString("[switchStreamingInstances] 切换拉流实例: %1 个").arg(streamingIds.size()));
    
    if (m_sessions.isEmpty()) {
        Logger::warning("[switchStreamingInstances] 没有可用的session");
        return;
    }
    
    // 将可见实例分配到各个session
    QVector<QStringList> streamingBySession = distributeVisibleInstancesToSessions(streamingIds);
    
    // 对每个session分别调用切换接口
    for (int i = 0; i < m_sessions.size(); ++i) {
        SessionInfo& sessionInfo = m_sessions[i];
        const QStringList& sessionStreamingIds = streamingBySession[i];
        
        if (!sessionInfo.session) {
            continue;
        }
        
        // 检查是否有变化
        QSet<QString> newSet(sessionStreamingIds.begin(), sessionStreamingIds.end());
        QSet<QString> currentSet(sessionInfo.currentStreamingIds.begin(), sessionInfo.currentStreamingIds.end());
        
        if (newSet == currentSet) {
            Logger::debug(QString("[switchStreamingInstances] Session %1 无变化，跳过").arg(i));
            continue;
        }
        
        // 验证要切换的实例是否都在该session的allInstanceIds中
        QStringList validIds;
        QSet<QString> sessionAllIds(sessionInfo.allInstanceIds.begin(), sessionInfo.allInstanceIds.end());
        for (const QString& id : sessionStreamingIds) {
            if (sessionAllIds.contains(id)) {
                validIds.append(id);
            } else {
                Logger::warning(QString("[switchStreamingInstances] 实例 %1 不在Session %2 的实例列表中")
                              .arg(id)
                              .arg(i));
            }
        }
        
        if (validIds.isEmpty()) {
            Logger::debug(QString("[switchStreamingInstances] Session %1 没有有效的切换实例").arg(i));
            continue;
        }
        
        // 转换为 C 字符串数组并调用 SDK
        auto result = VariantListConverter::convert(validIds);
        
        tcr_session_switch_streaming_instances(
            sessionInfo.session,
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        
        // 更新该session当前拉流实例列表
        sessionInfo.currentStreamingIds = validIds;
        
        Logger::info(QString("[switchStreamingInstances] Session %1 已切换到 %2 个实例")
                    .arg(i)
                    .arg(validIds.size()));
    }
    
    // 更新全局当前拉流实例列表
    m_currentStreamingIds = streamingIds;
    
    Logger::info(QString("[switchStreamingInstances] 总共切换到 %1 个实例").arg(streamingIds.size()));
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
    closeAllSessions();
    
    if (allInstanceIds.isEmpty()) {
        Logger::warning("[connectMultipleInstances] 实例ID列表为空");
        return;
    }
    
    // 保存所有实例ID和总并发数
    m_allInstanceIds = allInstanceIds;
    m_totalConcurrentInstances = concurrentStreamingInstances;
    
    // 确保 TcrClient 已初始化
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance();
    }
    
    // 计算需要的session数量
    int sessionCount = calculateSessionCount(allInstanceIds.size(), concurrentStreamingInstances);
    
    // 为每个session生成重新排序的实例列表（每个session都包含所有实例，但顺序不同）
    QVector<QStringList> instanceDistribution = reorderInstancesForSessions(allInstanceIds, sessionCount);
    
    // 初始化session列表和实例映射
    m_sessions.resize(sessionCount);
    m_instanceToSessionIndex.clear();
    
    // 建立实例到session的映射（用于视频帧路由）
    // 策略：每个实例映射到它作为"优先实例"的那个session
    for (int i = 0; i < sessionCount; ++i) {
        int priorityStart = i * MAX_CONCURRENT_INSTANCES_PER_SESSION;
        int priorityEnd = qMin(priorityStart + MAX_CONCURRENT_INSTANCES_PER_SESSION, allInstanceIds.size());
        
        for (int j = priorityStart; j < priorityEnd; ++j) {
            m_instanceToSessionIndex[allInstanceIds[j]] = i;
        }
    }
    
    // 创建每个session
    for (int i = 0; i < sessionCount; ++i) {
        const QStringList& sessionInstanceOrder = instanceDistribution[i];
        
        // 每个session的并发数都是100
        int actualConcurrent = MAX_CONCURRENT_INSTANCES_PER_SESSION;
        
        // 创建session，传入重新排序的实例列表
        createSession(i, sessionInstanceOrder, actualConcurrent);
    }
    
    // 设置所有实例为连接中状态
    for (const QString& instanceId : allInstanceIds) {
        m_instanceConnectionStates[instanceId] = InstanceConnectionState::Connecting;
        emit instanceConnectionChanged(instanceId, false);
    }
    
    m_connectedInstanceIds.clear();
    m_currentStreamingIds.clear();
    emit connectedInstanceIdsChanged();
    
    Logger::info(QString("[connectMultipleInstances] 已创建 %1 个session，每个session管理所有 %2 个实例")
                .arg(sessionCount)
                .arg(allInstanceIds.size()));
}

void MultiStreamViewModel::createSession(int sessionIndex, const QStringList& instanceIds, int concurrentStreamingInstances)
{
    Logger::info(QString("[createSession] 创建Session %1，传入 %2 个实例，同时串流数量: %3")
                .arg(sessionIndex)
                .arg(instanceIds.size())
                .arg(concurrentStreamingInstances));
    
    if (instanceIds.isEmpty()) {
        Logger::warning(QString("[createSession] Session %1 实例列表为空").arg(sessionIndex));
        return;
    }
    
    if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
        Logger::error(QString("[createSession] 无效的session索引: %1").arg(sessionIndex));
        return;
    }

    SessionInfo& sessionInfo = m_sessions[sessionIndex];

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
    
    // 设置并发拉流实例数量（确保不超过100）
    config.concurrentStreamingInstances = qMin(concurrentStreamingInstances, MAX_CONCURRENT_INSTANCES_PER_SESSION);
    
    Logger::info(QString("[createSession] Session %1 使用串流参数 - 宽度:%2, 帧率:%3, 码率:%4-%5, 同时串流数量:%6")
                .arg(sessionIndex)
                .arg(config.stream_profile.video_width)
                .arg(config.stream_profile.fps)
                .arg(config.stream_profile.min_bitrate)
                .arg(config.stream_profile.max_bitrate)
                .arg(config.concurrentStreamingInstances));
    
    // 步骤2：创建会话
    sessionInfo.session = tcr_client_create_session(m_tcrClient, &config);
    sessionInfo.allInstanceIds = instanceIds;  // 保存带顺序的所有实例ID
    sessionInfo.concurrentStreamingInstances = config.concurrentStreamingInstances;
    sessionInfo.currentStreamingIds.clear();
    
    if (!sessionInfo.session) {
        Logger::error(QString("[createSession] Session %1 创建失败").arg(sessionIndex));
        return;
    }
    
    Logger::info(QString("[createSession] Session %1 创建成功，前3个实例: %2...")
                .arg(sessionIndex)
                .arg(instanceIds.mid(0, 3).join(",")));
    
    // 步骤3：初始化用户数据和观察者结构体（堆分配，确保生命周期）
    sessionInfo.userData = new SessionUserData();
    sessionInfo.userData->viewModel = this;
    sessionInfo.userData->sessionIndex = sessionIndex;
    
    // 初始化会话事件观察者
    sessionInfo.sessionObserver.user_data = sessionInfo.userData;
    sessionInfo.sessionObserver.on_event = &MultiStreamViewModel::SessionEventCallback;
    
    // 初始化视频帧观察者
    sessionInfo.videoFrameObserver.user_data = sessionInfo.userData;
    sessionInfo.videoFrameObserver.on_frame = &MultiStreamViewModel::VideoFrameCallback;
    
    // 步骤4：设置观察者
    setSessionObservers(sessionIndex);
    
    // 步骤5：连接所有实例（使用 tcr_session_access_multi_stream）
    // 注意：传入的是带特定顺序的实例列表，前100个会被默认拉流
    auto result = VariantListConverter::convert(instanceIds);
    if (!result.pointers.empty()) {
        tcr_session_access_multi_stream(
            sessionInfo.session,
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        Logger::debug(QString("[createSession] Session %1 开始连接实例（多流模式），传入 %2 个实例")
                     .arg(sessionIndex)
                     .arg(result.pointers.size()));
    }
}

void MultiStreamViewModel::setSessionObservers(int sessionIndex)
{
    if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
        return;
    }
    
    SessionInfo& sessionInfo = m_sessions[sessionIndex];
    
    if (!sessionInfo.session) {
        return;
    }
    
    // 设置会话事件观察者
    tcr_session_set_observer(
        sessionInfo.session, 
        &sessionInfo.sessionObserver
    );
    
    // 设置视频帧观察者
    tcr_session_set_video_frame_observer(
        sessionInfo.session, 
        &sessionInfo.videoFrameObserver
    );
}

// ==================== 会话关闭 ====================

void MultiStreamViewModel::closeAllSessions()
{
    Logger::info(QString("[closeAllSessions] 开始关闭 %1 个会话").arg(m_sessions.size()));
    
    for (int i = 0; i < m_sessions.size(); ++i) {
        SessionInfo& sessionInfo = m_sessions[i];
        
        if (sessionInfo.session) {
            // 重要：必须先取消观察者，再销毁会话
            tcr_session_set_observer(sessionInfo.session, nullptr);
            tcr_session_set_video_frame_observer(sessionInfo.session, nullptr);
            
            // 销毁会话
            if (m_tcrClient) {
                tcr_client_destroy_session(m_tcrClient, sessionInfo.session);
            }
            sessionInfo.session = nullptr;
        }
        
        // 释放用户数据
        if (sessionInfo.userData) {
            delete sessionInfo.userData;
            sessionInfo.userData = nullptr;
        }
        
        sessionInfo.allInstanceIds.clear();
        sessionInfo.currentStreamingIds.clear();
        sessionInfo.connected = false;
    }
    
    m_sessions.clear();
    m_instanceToSessionIndex.clear();
    m_allInstanceIds.clear();
    m_connectedInstanceIds.clear();
    m_currentStreamingIds.clear();
    m_instanceConnectionStates.clear();
    m_totalConcurrentInstances = 0;
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
    
    // 按session分组
    QHash<int, QStringList> instancesBySession;
    for (const QString& instanceId : instanceIds) {
        int sessionIndex = findSessionIndexForInstance(instanceId);
        if (sessionIndex >= 0) {
            instancesBySession[sessionIndex].append(instanceId);
        }
    }
    
    // 对每个session分别调用暂停接口
    for (auto it = instancesBySession.begin(); it != instancesBySession.end(); ++it) {
        int sessionIndex = it.key();
        const QStringList& sessionInstances = it.value();
        
        if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
            continue;
        }
        
        SessionInfo& sessionInfo = m_sessions[sessionIndex];
        if (!sessionInfo.session) {
            continue;
        }
        
        auto result = VariantListConverter::convert(sessionInstances);
        if (result.pointers.empty()) {
            continue;
        }
        
        tcr_session_pause_streaming(
            sessionInfo.session,
            nullptr,
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        
        Logger::info(QString("[pauseStreaming] Session %1 已暂停流媒体，实例: %2")
                    .arg(sessionIndex)
                    .arg(sessionInstances.join(",")));
    }
}

void MultiStreamViewModel::resumeStreaming(const QStringList& instanceIds)
{
    Logger::info(QString("[resumeStreaming] 恢复流媒体，实例数: %1").arg(instanceIds.size()));
    
    if (instanceIds.isEmpty()) {
        Logger::warning("[resumeStreaming] 实例ID列表为空");
        return;
    }
    
    // 按session分组
    QHash<int, QStringList> instancesBySession;
    for (const QString& instanceId : instanceIds) {
        int sessionIndex = findSessionIndexForInstance(instanceId);
        if (sessionIndex >= 0) {
            instancesBySession[sessionIndex].append(instanceId);
        }
    }
    
    // 对每个session分别调用恢复接口
    for (auto it = instancesBySession.begin(); it != instancesBySession.end(); ++it) {
        int sessionIndex = it.key();
        const QStringList& sessionInstances = it.value();
        
        if (sessionIndex < 0 || sessionIndex >= m_sessions.size()) {
            continue;
        }
        
        SessionInfo& sessionInfo = m_sessions[sessionIndex];
        if (!sessionInfo.session) {
            continue;
        }
        
        auto result = VariantListConverter::convert(sessionInstances);
        if (result.pointers.empty()) {
            continue;
        }
        
        tcr_session_resume_streaming(
            sessionInfo.session,
            nullptr,
            result.pointers.data(),
            static_cast<int32_t>(result.pointers.size())
        );
        
        Logger::info(QString("[resumeStreaming] Session %1 已恢复流媒体，实例: %2")
                    .arg(sessionIndex)
                    .arg(sessionInstances.join(",")));
    }
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
    int sessionIndex = userData->sessionIndex;
    
    // 检查是否正在销毁
    if (self->m_isDestroying.load(std::memory_order_acquire)) {
        return;
    }
    
    // 检查session索引有效性
    if (sessionIndex < 0 || sessionIndex >= self->m_sessions.size()) {
        Logger::warning(QString("[SessionEventCallback] 无效的session索引: %1").arg(sessionIndex));
        return;
    }
    
    SessionInfo& sessionInfo = self->m_sessions[sessionIndex];
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();
    
    // 处理不同的事件类型
    switch (event) {
        case TCR_SESSION_EVENT_STATE_CONNECTED: {
            Logger::info(QString("[SessionEventCallback] Session %1 连接成功，管理 %2 个实例")
                        .arg(sessionIndex)
                        .arg(sessionInfo.allInstanceIds.size()));
            
            sessionInfo.connected = true;
            
            // 更新该session管理的所有实例为已连接状态
            for (const QString& instanceId : sessionInfo.allInstanceIds) {
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
            Logger::error(QString("[SessionEventCallback] Session %1 断开: %2")
                         .arg(sessionIndex)
                         .arg(eventDataCopy));
            
            sessionInfo.connected = false;
            
            // 更新该session管理的所有实例为未连接状态
            for (const QString& instanceId : sessionInfo.allInstanceIds) {
                self->m_instanceConnectionStates[instanceId] = InstanceConnectionState::Disconnected;
                self->m_connectedInstanceIds.removeAll(instanceId);
                emit self->instanceConnectionChanged(instanceId, false);
            }
            
            emit self->connectedInstanceIdsChanged();
            emit self->sessionClosed(sessionIndex, sessionInfo.allInstanceIds, eventDataCopy);
            break;
            
        case TCR_SESSION_EVENT_CLIENT_STATS:
            // TODO: 合并多个session的统计数据（这里简单地使用最后一个session的数据）
            self->m_clientStats = eventDataCopy;
            emit self->clientStatsChanged();
            break;
            
        default:
            Logger::debug(QString("[SessionEventCallback] Session %1 收到事件: %2, 数据: %3")
                         .arg(sessionIndex)
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
    int sessionIndex = userData->sessionIndex;
    
    if (self->m_isDestroying.load(std::memory_order_acquire)) {
        return;
    }
    
    // 检查session索引有效性
    if (sessionIndex < 0 || sessionIndex >= self->m_sessions.size()) {
        tcr_video_frame_release(frame_handle);
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
    
    // 验证实例是否属于当前session（使用allInstanceIds）
    const SessionInfo& sessionInfo = self->m_sessions[sessionIndex];
    if (!sessionInfo.allInstanceIds.contains(instanceId)) {
        Logger::warning(QString("[VideoFrameCallback] 实例 %1 不属于Session %2")
                       .arg(instanceId)
                       .arg(sessionIndex));
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
