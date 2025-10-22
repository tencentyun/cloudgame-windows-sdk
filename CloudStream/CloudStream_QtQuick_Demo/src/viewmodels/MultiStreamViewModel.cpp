#include "MultiStreamViewModel.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "utils/VariantListConverter.h"
#include <QVariant>
#include <QDebug>
#include <QMetaType>
#include <QGuiApplication>
#include <algorithm>

// ==================== 构造与析构 ====================

MultiStreamViewModel::MultiStreamViewModel(QObject *parent)
    : QObject(parent)
{
    Logger::info("[MultiStreamViewModel] 构造函数");

    // 注册元类型，确保跨线程信号槽可用
    qRegisterMetaType<VideoFrameData>("VideoFrameData");
    qRegisterMetaType<VideoFrameDataPtr>("VideoFrameDataPtr");
}

MultiStreamViewModel::~MultiStreamViewModel()
{
    Logger::info("[MultiStreamViewModel] 开始析构");
    
    // 重要：先清理所有观察者，防止回调继续执行
    // 这是使用 TcrSdk 的关键步骤，必须在销毁会话前取消观察者
    for (auto& sessionInfo : m_sessions) {
        if (sessionInfo.session) {
            tcr_session_set_observer(sessionInfo.session, nullptr);
            tcr_session_set_video_frame_observer(sessionInfo.session, nullptr);
        }
    }
    
    // 清理 VideoRenderItem 映射
    m_videoRenderItems.clear();
    
    // 关闭所有会话
    closeAllSessions();
    
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
    TcrConfig config = {
        tokenStr.c_str(),      // Token: 访问令牌，用于身份验证
        accessInfoStr.c_str(), // AccessInfo: 访问信息，包含服务器地址等配置
        CloudStream            // 产品类型
    };
    
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

// ==================== 视频渲染项注册 ====================

void MultiStreamViewModel::registerVideoRenderItem(const QString& instanceId, 
                                                   int instanceIndex, 
                                                   QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderItem*>(item);
    if (!vrItem) {
        Logger::warning(QString("[registerVideoRenderItem] 类型转换失败: %1_%2")
                       .arg(instanceId).arg(instanceIndex));
        return;
    }
    
    // 使用 "instanceId_instanceIndex" 作为唯一标识
    // 这个标识符与 TcrSdk 视频帧回调中的 instance_id 和 instance_index 对应
    QString uniqueKey = QString("%1_%2").arg(instanceId).arg(instanceIndex);
    m_videoRenderItems[uniqueKey] = vrItem;
    
    // 连接信号槽，使用 QueuedConnection 确保线程安全
    // 视频帧回调可能在非主线程执行，需要通过信号槽切换到主线程
    connect(this, &MultiStreamViewModel::newVideoFrameForInstance,
            this, [this, uniqueKey, vrItem](const QString& targetUniqueKey, VideoFrameDataPtr frame) {
                if (targetUniqueKey == uniqueKey) {
                    vrItem->setFrame(frame);
                }
            }, Qt::QueuedConnection);
    
    Logger::debug(QString("[registerVideoRenderItem] 注册成功: %1").arg(uniqueKey));
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
    
    // 收集所有实例ID并更新连接状态
    QStringList allInstanceIds;
    for (const QVariant& sessionConfig : sessionConfigs) {
        QStringList sessionInstances = sessionConfig.toStringList();
        allInstanceIds.append(sessionInstances);
    }
    
    m_connectedInstanceIds = allInstanceIds;
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
        
        // 自定义视频流参数
        config.stream_profile.video_width = 144;   // 视频宽度
        config.stream_profile.video_height = 256;  // 视频高度
        config.stream_profile.fps = 1;             // 帧率
        config.stream_profile.max_bitrate = 4000;  // 最大码率
        config.stream_profile.min_bitrate = 1000;  // 最小码率
        config.stream_profile.unit = "Kbps";       // 码率单位
        config.enable_audio = false;               // 禁用音频
        
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
            tcr_session_access(
                m_sessions[i].session,
                result.pointers.data(),
                static_cast<int32_t>(result.pointers.size()),
                false  // isGroupControl: false 表示非群控模式
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
    emit connectedInstanceIdsChanged();
    
    Logger::info("[closeAllSessions] 所有会话已关闭");
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
            case TCR_SESSION_EVENT_STATE_CONNECTED:
                // 会话连接成功
                Logger::info(QString("[SessionEventCallback] 会话 %1 连接成功，实例: %2")
                            .arg(sessionIndex).arg(sessionInfo.instanceIds.join(",")));
                
                sessionInfo.connected = true;
                for (const QString& instanceId : sessionInfo.instanceIds) {
                    emit self->instanceConnectionChanged(instanceId, true);
                }
                break;
                
            case TCR_SESSION_EVENT_STATE_CLOSED:
                // 会话关闭
                Logger::error(QString("[SessionEventCallback] 会话 %1 断开: %2")
                             .arg(sessionIndex).arg(eventDataCopy));
                
                sessionInfo.connected = false;
                for (const QString& instanceId : sessionInfo.instanceIds) {
                    emit self->instanceConnectionChanged(instanceId, false);
                }
                break;
                
            default:
                // 其他事件类型
                Logger::debug(QString("[SessionEventCallback] 会话 %1 事件: %2")
                             .arg(sessionIndex).arg(static_cast<int>(event)));
                break;
        }
    }, Qt::QueuedConnection);
}

void MultiStreamViewModel::VideoFrameCallback(void* user_data,
                                             TcrVideoFrameHandle frame_handle)
{
    // 步骤1：参数有效性检查
    if (!user_data || !frame_handle) {
        return;
    }
    
    SessionUserData* userData = static_cast<SessionUserData*>(user_data);
    if (!userData || !userData->viewModel) {
        return;
    }
    
    MultiStreamViewModel* self = userData->viewModel;
    int sessionIndex = userData->sessionIndex;
    
    // 步骤2：获取视频帧缓冲区
    // tcr_video_frame_get_buffer() 返回帧数据的只读指针
    const TcrVideoFrameBuffer* frame_buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!frame_buffer) {
        return;
    }

    // 步骤3：提取实例标识信息
    // instance_id: 云手机实例ID
    // instance_index: 实例索引（用于区分同一实例的多个视频流）
    const char* instanceID = frame_buffer->instance_id;
    const int instanceIndex = frame_buffer->instance_index;

    QString instanceId = QString::fromUtf8(instanceID);
    QString uniqueKey = QString("%1_%2").arg(instanceId).arg(instanceIndex);
    
    // 步骤4：验证实例是否属于当前会话
    if (sessionIndex >= 0 && sessionIndex < self->m_sessions.size()) {
        const SessionInfo& sessionInfo = self->m_sessions[sessionIndex];
        if (!sessionInfo.instanceIds.contains(instanceId)) {
            Logger::warning(QString("[VideoFrameCallback] 实例 %1 不属于会话 %2")
                           .arg(instanceId).arg(sessionIndex));
            tcr_video_frame_release(frame_handle);
            return;
        }
    }
    
    // 步骤5：检查是否有对应的渲染项
    if (!self->m_videoRenderItems.contains(uniqueKey)) {
        // 没有注册的渲染项，直接释放帧资源
        tcr_video_frame_release(frame_handle);
        return;
    }

    // 步骤6：提取 I420 格式的视频数据
    const TcrI420Buffer& buffer = frame_buffer->buffer.i420;

    // 步骤7：增加引用计数
    // 重要：必须调用 tcr_video_frame_add_ref() 增加引用计数
    // 否则帧数据可能在使用前被释放
    tcr_video_frame_add_ref(frame_handle);
    
    // 步骤8：创建视频帧数据智能指针
    // VideoFrameDataPtr 是智能指针，析构时会自动调用 tcr_video_frame_release()
    VideoFrameDataPtr frameDataPtr(new VideoFrameData);
    
    frameDataPtr->frame_handle = frame_handle;
    frameDataPtr->width = buffer.width;
    frameDataPtr->height = buffer.height;
    frameDataPtr->strideY = buffer.stride_y;
    frameDataPtr->strideU = buffer.stride_u;
    frameDataPtr->strideV = buffer.stride_v;
    frameDataPtr->data_y = buffer.data_y;
    frameDataPtr->data_u = buffer.data_u;
    frameDataPtr->data_v = buffer.data_v;

    // 步骤9：发送视频帧信号
    // 通过信号槽机制将帧数据发送到对应的 VideoRenderItem
    emit self->newVideoFrameForInstance(uniqueKey, frameDataPtr);
}

