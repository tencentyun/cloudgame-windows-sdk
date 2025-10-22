#include "StreamingViewModel.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "utils/VariantListConverter.h"
#include <QVariant>
#include <QDebug>
#include <QMetaType>
#include <QGuiApplication>

// ==================== 构造与析构 ====================

StreamingViewModel::StreamingViewModel(QObject *parent)
    : QObject(parent)
    , m_sessionConnected(false)
{
    Logger::info("[StreamingViewModel] 构造函数");

    // 注册元类型，确保跨线程信号槽可用
    qRegisterMetaType<VideoFrameData>("VideoFrameData");
    qRegisterMetaType<VideoFrameDataPtr>("VideoFrameDataPtr");

    // 初始化会话事件回调结构体
    m_sessionObserver.user_data = this;
    m_sessionObserver.on_event = &StreamingViewModel::SessionEventCallback;

    // 初始化视频帧回调结构体
    m_videoFrameObserver.user_data = this;
    m_videoFrameObserver.on_frame = &StreamingViewModel::VideoFrameCallback;
}

StreamingViewModel::~StreamingViewModel()
{
    closeSession();
}

// ==================== 视频渲染相关 ====================

void StreamingViewModel::setVideoRenderItem(VideoRenderItem* item)
{
    m_videoRenderItem = item;
    if (m_videoRenderItem) {
        // 使用 QueuedConnection 确保线程安全
        // 视频帧从解码线程 -> 主线程 -> 渲染线程
        connect(this, &StreamingViewModel::newVideoFrame,
                m_videoRenderItem, &VideoRenderItem::setFrame,
                Qt::QueuedConnection);
    }
}

void StreamingViewModel::setVideoRenderItem(QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderItem*>(item);
    if (vrItem) {
        setVideoRenderItem(vrItem);
    } else {
        Logger::info("setVideoRenderItem: cast to VideoRenderItem* failed");
    }
}

// ==================== 会话管理 ====================

void StreamingViewModel::connectSession(const QVariantList& instanceIds, bool isGroupControl)
{
    if (instanceIds.isEmpty()) {
        Logger::debug("[connectSession] 实例ID列表为空");
        return;
    }

    // 更新群控实例列表
    m_groupInstanceIds.clear();
    for (const QVariant& id : instanceIds) {
        m_groupInstanceIds << id.toString();
    }
    
    Logger::debug(QString("[connectSession] %1模式，实例ID: %2")
                      .arg(isGroupControl ? "群控" : "单实例")
                      .arg(m_groupInstanceIds.join(",")));

    // 【步骤1】创建并初始化会话
    createAndInitSession();
    
    if (m_session) {
        // 【步骤2】连接到云手机实例
        // SDK API: tcr_session_access(session, instanceIds, count, isGroupControl)
        auto result = VariantListConverter::convert(instanceIds);
        if (!result.pointers.empty()) {
            tcr_session_access(m_session,
                               result.pointers.data(),
                               static_cast<int32_t>(result.pointers.size()),
                               isGroupControl);
        } else {
            Logger::debug("[connectSession] 没有可用的InstanceId");
        }
    } else {
        Logger::debug("[connectSession] 创建Session失败");
    }
}

void StreamingViewModel::closeSession()
{
    if (m_session) {
        // 【步骤1】取消观察者（防止回调访问已释放的对象）
        // SDK API: tcr_session_set_observer(session, nullptr)
        tcr_session_set_observer(m_session, nullptr);
        
        // SDK API: tcr_session_set_video_frame_observer(session, nullptr)
        tcr_session_set_video_frame_observer(m_session, nullptr);
        
        // 【步骤2】销毁会话
        // SDK API: tcr_client_destroy_session(client, session)
        if (m_tcrClient) {
            tcr_client_destroy_session(m_tcrClient, m_session);
        }
        m_session = nullptr;
    }
}

void StreamingViewModel::createAndInitSession()
{
    // 先关闭旧会话
    closeSession();
    
    // 【步骤1】获取 TcrClient 实例（单例模式）
    // SDK API: tcr_client_get_instance()
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance();
    }
    
    // 【步骤2】获取 Android 实例操作句柄
    // SDK API: tcr_client_get_android_instance(client)
    m_instance = tcr_client_get_android_instance(m_tcrClient);

    // 【步骤3】创建会话
    // SDK API: tcr_client_create_session(client, config)
    TcrSessionConfig config = tcr_session_config_default();
    m_session = tcr_client_create_session(m_tcrClient, &config);

    // 【步骤4】设置观察者
    setSessionObservers();
}

void StreamingViewModel::setSessionObservers()
{
    if (m_session) {
        // 【设置会话事件观察者】
        // SDK API: tcr_session_set_observer(session, observer)
        // 用于接收连接状态、摄像头状态、统计数据等事件
        tcr_session_set_observer(m_session, &m_sessionObserver);
        
        // 【设置视频帧观察者】
        // SDK API: tcr_session_set_video_frame_observer(session, observer)
        // 用于接收解码后的视频帧数据
        tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
    }
}

void StreamingViewModel::updateCheckedInstanceIds(const QVariantList& instanceIds, 
                                                  const QVariantList& addedInstanceIds)
{
    QStringList instanceIdList, addedIdList;
    for (const QVariant& id : instanceIds) instanceIdList << id.toString();
    for (const QVariant& id : addedInstanceIds) addedIdList << id.toString();
    
    Logger::info(QString("[updateCheckedInstanceIds] 勾选实例: [%1], 新增实例: [%2]")
                     .arg(instanceIdList.join(", "))
                     .arg(addedInstanceIds.isEmpty() ? "无" : addedIdList.join(", ")));
                     
    if (m_session == nullptr) {
        Logger::debug("updateCheckedInstanceIds() m_session == nullptr");
        return;
    }

    // 【步骤1】将新增实例加入群控组
    // SDK API: tcr_instance_join_group(instance, instanceIds, count)
    if (!addedInstanceIds.isEmpty()) {
        auto result = VariantListConverter::convert(addedInstanceIds);
        if (!result.pointers.empty()) {
            tcr_instance_join_group(m_instance,
                                    result.pointers.data(), 
                                    static_cast<int32_t>(result.pointers.size()));
        }
    }

    // 【步骤2】更新同步列表
    // SDK API: tcr_instance_set_sync_list(instance, instanceIds, count)
    if (!instanceIds.isEmpty()) {
        auto result = VariantListConverter::convert(instanceIds);
        if (!result.pointers.empty()) {
            tcr_instance_set_sync_list(m_instance,
                                       result.pointers.data(), 
                                       static_cast<int32_t>(result.pointers.size()));
        }
    }
}

// ==================== 触摸输入 ====================

void StreamingViewModel::handleMouseEvent(int x, int y, int width, int height,
                                          int eventType, qint64 timestamp)
{
    if (m_session && m_sessionConnected) {
        // SDK API: tcr_session_touchscreen_touch(session, x, y, eventType, width, height, timestamp)
        // eventType: 0=按下, 1=移动, 2=抬起
        tcr_session_touchscreen_touch(m_session, x, y, eventType, width, height, timestamp);
    }
}

// ==================== 系统按键 ====================

void StreamingViewModel::onBackClicked()
{
    if (m_session && m_sessionConnected) {
        // SDK API: tcr_session_send_keyboard_event(session, keycode, down)
        // Keycode 158 = 返回键
        tcr_session_send_keyboard_event(m_session, 158, true);   // 按下
        tcr_session_send_keyboard_event(m_session, 158, false);  // 抬起
    }
}

void StreamingViewModel::onHomeClicked()
{
    if (m_session && m_sessionConnected) {
        // Keycode 172 = Home 键
        tcr_session_send_keyboard_event(m_session, 172, true);
        tcr_session_send_keyboard_event(m_session, 172, false);
    }
}

void StreamingViewModel::onMenuClicked()
{
    if (m_session && m_sessionConnected) {
        // Keycode 139 = 菜单键
        tcr_session_send_keyboard_event(m_session, 139, true);
        tcr_session_send_keyboard_event(m_session, 139, false);
    }
}

void StreamingViewModel::onVolumUp()
{
    if (m_session && m_sessionConnected) {
        // Keycode 58 = 音量加
        tcr_session_send_keyboard_event(m_session, 58, true);
        tcr_session_send_keyboard_event(m_session, 58, false);
    }
}

void StreamingViewModel::onVolumDown()
{
    if (m_session && m_sessionConnected) {
        // Keycode 59 = 音量减
        tcr_session_send_keyboard_event(m_session, 59, true);
        tcr_session_send_keyboard_event(m_session, 59, false);
    }
}

// ==================== 流控制 ====================

void StreamingViewModel::onPauseStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onPauseStreamClicked] session not ready");
        return;
    }
    // SDK API: 暂停音视频推流
    tcr_session_pause_streaming(m_session);
}

void StreamingViewModel::onResumeStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onResumeStreamClicked] session not ready");
        return;
    }
    // SDK API: 恢复音视频推流
    tcr_session_resume_streaming(m_session);
}

// ==================== 设备控制 ====================

void StreamingViewModel::onEnableCameraClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onEnableCameraClicked] session not ready");
        return;
    }
    // SDK API: 启用摄像头
    tcr_session_enable_local_camera(m_session, true);
}

void StreamingViewModel::onDisableCameraClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onDisableCameraClicked] session not ready");
        return;
    }
    // SDK API: 关闭摄像头
    tcr_session_disable_local_camera(m_session);
}

void StreamingViewModel::onEnableMicrophoneClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onEnableMicrophoneClicked] session not ready");
        return;
    }
    // SDK API: 启用麦克风
    tcr_session_enable_local_microphone(m_session, true);
}

void StreamingViewModel::onDisableMicrophoneClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onDisableMicrophoneClicked] session not ready");
        return;
    }
    // SDK API: 关闭麦克风
    tcr_session_enable_local_microphone(m_session, false);
}

// ==================== SDK 回调函数实现 ====================

/**
 * @brief 会话事件回调（SDK 内部线程调用）
 * 
 * 处理流程：
 *   1. 在 SDK 线程中接收事件
 *   2. 通过 QMetaObject::invokeMethod 切换到主线程
 *   3. 在主线程中处理业务逻辑
 */
void StreamingViewModel::SessionEventCallback(void* user_data,
                                              TcrSessionEvent event,
                                              const char* eventData)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();

    // 切换到主线程处理
    QMetaObject::invokeMethod(self, [self, event, eventDataCopy]() {
        
        // 【事件1：连接成功】
        if (event == TCR_SESSION_EVENT_STATE_CONNECTED) {
            Logger::info("[SessionEventCallback] 会话连接成功");
            self->m_sessionConnected = true;

            // 调整视频流参数
            // SDK API: 设置视频流参数
            tcr_session_set_remote_video_profile(self->m_session, 
                                                  30,    // fps: 30帧/秒
                                                  100,   // minBitrate: 100 kbps
                                                  200,   // maxBitrate: 200 kbps
                                                  720,   // height: 720p
                                                  1280); // width: 1280p

            // ========== 创建自定义数据通道示例 ==========
            // 数据通道用于与云端App进行自定义数据交互
            static TcrDataChannelObserver s_dc_observer = {
                nullptr, // user_data，后面会设置
                
                // 【回调1：连接成功】
                [](void* user_data, int32_t port) {
                    auto* self = static_cast<StreamingViewModel*>(user_data);
                    Logger::info(QString("[DataChannel] connected on port %1").arg(port));

                    // 发送测试消息
                    auto data = QByteArray("{\"action\":0,\"content\":\"{\\\"type\\\":\\\"PullStreamConnected\\\"}\",\"coords\":[],\"heightPixels\":0,\"isOpenScreenFollowRotation\":false,\"keyCode\":0,\"pointCount\":0,\"properties\":[],\"text\":\"\",\"touchType\":\"eventSdk\",\"widthPixels\":0}").toStdString();
                    
                    // SDK API: 通过自定义数据通道发送数据
                    TcrErrorCode code = tcr_data_channel_send(
                        self->m_dataChannel, 
                        reinterpret_cast<const uint8_t*>(data.data()), 
                        data.size());
                    Logger::info(QString("[DataChannel] send result code=%1").arg(code));
                },
                
                // 【回调2：错误处理】
                [](void* user_data, int32_t port,
                   const TcrErrorCode& code, const char* msg) {
                    auto* self = static_cast<StreamingViewModel*>(user_data);
                    Logger::error(QString("[DataChannel] error on port %1, code=%2, msg=%3")
                                      .arg(port)
                                      .arg(code)
                                      .arg(msg ? msg : ""));
                },
                
                // 【回调3：接收消息】
                [](void* user_data, int32_t port,
                   const uint8_t* data, size_t size) {
                    auto* self = static_cast<StreamingViewModel*>(user_data);
                    Logger::debug(QString("[DataChannel] recv message on port %1, size=%2")
                                      .arg(port).arg(size));
                    if (data && size > 0) {
                        QByteArray rawData(reinterpret_cast<const char*>(data), static_cast<int>(size));
                        Logger::debug(QString("[DataChannel] data content: %1")
                                          .arg(QString::fromUtf8(rawData)));
                    }
                }
            };
            s_dc_observer.user_data = self;

            // SDK API: 创建自定义数据通道
            self->m_dataChannel = tcr_session_create_data_channel(
                self->m_session, 
                23332,              // 端口号
                &s_dc_observer,     // 观察者
                "android");         // 类型  "android" 或 "android_broadcast", 后者表示广播给群控被控实例
                
            if (self->m_dataChannel) {
                Logger::info("[DataChannel] create success, port=23332");
            } else {
                Logger::error("[DataChannel] create failed");
            }
            // ==========================================

        } 
        // 【事件2：摄像头状态变化】
        else if (event == TCR_SESSION_EVENT_CAMERA_STATUS) {
            QString eventDataStr = eventDataCopy;
            QJsonDocument doc = QJsonDocument::fromJson(eventDataStr.toUtf8());
            if (doc.isObject()) {
                QString status = doc.object().value("status").toString();
                Logger::info(QString("[云端摄像头状态变更] 最新状态:%1").arg(status));
            }
        } 
        // 【事件3：统计数据更新】
        else if (event == TCR_SESSION_EVENT_CLIENT_STATS) {
            // 更新统计数据（包含帧率、码率、延迟等信息）
            self->m_clientStats = eventDataCopy;
            emit self->clientStatsChanged();
        } 
        // 【事件4：连接断开】
        else if (event == TCR_SESSION_EVENT_STATE_CLOSED) {
            self->m_sessionConnected = false;
            Logger::error("[SessionEventCallback] 会话断开: " + eventDataCopy);
        }
    }, Qt::QueuedConnection);
}

/**
 * @brief 视频帧回调（SDK 解码线程调用）
 * 
 * 处理流程：
 *   1. 增加帧引用计数（防止 SDK 提前释放）
 *   2. 获取帧数据（YUV I420 格式）
 *   3. 封装为智能指针（自动管理生命周期）
 *   4. 通过信号发送到主线程
 * 
 * 注意：此函数调用频率较高，需要高效处理
 */
void StreamingViewModel::VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    if (!self || !frame_handle) return;
    
    // 【步骤1】增加引用计数
    // SDK API: tcr_video_frame_add_ref(frame_handle)
    // 防止 SDK 在我们使用期间释放帧数据
    tcr_video_frame_add_ref(frame_handle);

    // 【步骤2】创建智能指针封装帧数据
    // VideoFrameDataPtr 析构时会自动调用 tcr_video_frame_release()
    VideoFrameDataPtr frameDataPtr(new VideoFrameData);

    // 【步骤3】获取帧缓冲区数据
    // SDK API: tcr_video_frame_get_buffer(frame_handle)
    const TcrI420Buffer& buffer = tcr_video_frame_get_buffer(frame_handle)->buffer.i420;
    
    // 【步骤4】填充帧数据结构
    frameDataPtr->frame_handle = frame_handle;
    frameDataPtr->width = buffer.width;
    frameDataPtr->height = buffer.height;
    frameDataPtr->strideY = buffer.stride_y;
    frameDataPtr->strideU = buffer.stride_u;
    frameDataPtr->strideV = buffer.stride_v;
    frameDataPtr->data_y = buffer.data_y;
    frameDataPtr->data_u = buffer.data_u;
    frameDataPtr->data_v = buffer.data_v;

    // 【步骤5】发送到主线程进行渲染
    emit self->newVideoFrame(frameDataPtr);
}
