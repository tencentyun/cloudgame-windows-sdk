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
    , m_currentRotationAngle(0.0)
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

void StreamingViewModel::setVideoRenderItem(VideoRenderPaintedItem* item)
{
    m_videoRenderItem = item;
    if (m_videoRenderItem) {
        // 使用 QueuedConnection 确保线程安全
        // 视频帧从解码线程 -> 主线程 -> 渲染线程
        connect(this, &StreamingViewModel::newVideoFrame,
                m_videoRenderItem, &VideoRenderPaintedItem::setFrame,
                Qt::QueuedConnection);
        
            m_videoRenderItem->setRotationAngle(m_currentRotationAngle, m_currentVideoWidth, m_currentVideoHeight);
            Logger::info(QString("[setVideoRenderItem] 应用旋转角度 %1 到新实例: %2, 视频尺寸: %3x%4")
                             .arg(m_currentRotationAngle)
                             .arg(m_videoRenderItem->objectName())
                             .arg(m_currentVideoWidth)
                             .arg(m_currentVideoHeight));
    }
}

void StreamingViewModel::setVideoRenderItem(QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderPaintedItem*>(item);
    if (vrItem) {
        setVideoRenderItem(vrItem);
    } else {
        Logger::info("setVideoRenderItem: cast to VideoRenderPaintedItem* failed");
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
    // 自定义视频流参数
    config.stream_profile.video_width = 720;   // 指定短边的宽度
    config.stream_profile.fps = 30;            // 帧率
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

// ==================== 发送触摸输入到云端 ====================

void StreamingViewModel::sendTouchEvent(int x, int y, int width, int height,
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

void StreamingViewModel::onPauseVideoStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onPauseVideoStreamClicked] session not ready");
        return;
    }
    // SDK API: 暂停视频推流
    tcr_session_pause_streaming(m_session, "video");
}

void StreamingViewModel::onResumeVideoStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onResumeVideoStreamClicked] session not ready");
        return;
    }
    // SDK API: 恢复视频推流
    tcr_session_resume_streaming(m_session, "video");
}

void StreamingViewModel::onPauseAudioStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onPauseAudioStreamClicked] session not ready");
        return;
    }
    // SDK API: 暂停音频推流
    tcr_session_pause_streaming(m_session, "audio");
}

void StreamingViewModel::onResumeAudioStreamClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onPauseAudioStreamClicked] session not ready");
        return;
    }
    // SDK API: 恢复音频推流
    tcr_session_resume_streaming(m_session, "audio");
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

void StreamingViewModel::onVideoStreamSettingsClicked()
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[onVideoStreamSettingsClicked] session not ready");
        return;
    }
    tcr_session_set_remote_video_profile(m_session, 30, 1000, 2000, 720, 1280);
}

void StreamingViewModel::enableCameraWithDevice(const QString& deviceId)
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[enableCameraWithDevice] session not ready");
        return;
    }
    
    if (deviceId.isEmpty()) {
        Logger::warning("[enableCameraWithDevice] deviceId is empty");
        return;
    }
    
    Logger::info(QString("[enableCameraWithDevice] 启用摄像头设备: %1").arg(deviceId));
    
    // 【步骤1】创建视频配置结构体
    TcrVideoConfig videoConfig = tcr_video_config_default();
    
    // 【步骤2】设置设备ID
    QByteArray deviceIdBytes = deviceId.toUtf8();
    videoConfig.device_id = deviceIdBytes.constData();
    
    // 【步骤3】其他参数使用默认值（已通过 {} 初始化为0/nullptr）
    // 可选参数包括：
    // - width: 视频宽度（0表示使用默认值）
    // - height: 视频高度（0表示使用默认值）
    // - fps: 帧率（0表示使用默认值）
    
    // 【步骤4】调用SDK API启用摄像头
    // SDK API: tcr_session_enable_local_camera_with_config(session, &config)
    
    if (tcr_session_enable_local_camera_with_config(m_session, &videoConfig)) {
        Logger::info(QString("[enableCameraWithDevice] 摄像头启用成功: %1").arg(deviceId));
    } else {
        Logger::error(QString("[enableCameraWithDevice] 摄像头启用失败: deviceId=%1")
                          .arg(deviceId));
    }
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
        // 【事件5：屏幕配置变化】
        // 当云端手机画面旋转时触发此事件
        // eventData 是 JSON 格式，包含：
        //   - width: 云端手机屏幕宽度（像素）
        //   - height: 云端手机屏幕高度（像素）
        //   - degree: 云端手机旋转方向（"0_degree", "90_degree", "180_degree", "270_degree"）
        // 
        // 坐标转换逻辑说明：
        //   云端手机的 degree 表示其屏幕旋转方向，但客户端需要反向旋转才能正确显示
        //   例如：云端旋转90度（degree="90_degree"），客户端需要旋转270度来抵消
        //   
        //   映射关系：
        //     云端 0度   -> 客户端 0度   (无旋转)
        //     云端 90度  -> 客户端 270度 (逆时针90度)
        //     云端 180度 -> 客户端 180度 (旋转180度)
        //     云端 270度 -> 客户端 90度  (顺时针90度)
        else if (event == TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE) {
            Logger::info("[SessionEventCallback] 屏幕配置变化: " + eventDataCopy);
            
            QJsonDocument doc = QJsonDocument::fromJson(eventDataCopy.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                int width = obj.value("width").toInt();
                int height = obj.value("height").toInt();
                QString degree = obj.value("degree").toString();
                
                // 判断横竖屏：width > height 为横屏
                bool isLandscape = (width > height);
                
                // 解析云端旋转角度并转换为客户端旋转角度
                // 客户端旋转角度 = 360 - 云端旋转角度（取模360）
                qreal rotationAngle = 0.0;
                if (degree.contains("90")) {
                    rotationAngle = 270.0;  // 云端90度 -> 客户端270度
                } else if (degree.contains("180")) {
                    rotationAngle = 180.0;  // 云端180度 -> 客户端180度
                } else if (degree.contains("270")) {
                    rotationAngle = 90.0;   // 云端270度 -> 客户端90度
                } else if (degree.contains("0")) {
                    rotationAngle = 0.0;    // 云端0度 -> 客户端0度
                }
                
                Logger::info(QString("[SessionEventCallback] 屏幕方向变化: width=%1, height=%2, degree=%3, isLandscape=%4, rotationAngle=%5")
                                 .arg(width)
                                 .arg(height)
                                 .arg(degree)
                                 .arg(isLandscape ? "true" : "false")
                                 .arg(rotationAngle));
                
                // 保存当前旋转角度和视频流尺寸
                // 这些参数用于后续的坐标转换和渲染
                self->m_currentRotationAngle = rotationAngle;
                self->m_currentVideoWidth = width;
                self->m_currentVideoHeight = height;
                
                // 设置视频渲染组件的旋转角度
                // width/height 是云端屏幕的实际尺寸，用于触摸坐标转换
                if (self->m_videoRenderItem) {
                    self->m_videoRenderItem->setRotationAngle(rotationAngle, width, height);
                    Logger::info(QString("[SessionEventCallback] 已设置视频旋转角度: %1 到实例: %2, 视频尺寸: %3x%4")
                                     .arg(rotationAngle)
                                     .arg(self->m_videoRenderItem->objectName())
                                     .arg(width)
                                     .arg(height));
                } else {
                    Logger::warning("[SessionEventCallback] m_videoRenderItem 为空，无法设置旋转角度");
                }
                
                // 发射信号通知 QML 层屏幕方向变化
                emit self->screenOrientationChanged(isLandscape);
            } else {
                Logger::warning("[SessionEventCallback] 屏幕配置变化数据解析失败");
            }
        }
        
    }, Qt::QueuedConnection);
}

/**
 * @brief 视频帧回调（SDK 解码线程调用）
 * 
 * 处理流程：
 *   1. 增加帧引用计数（防止 SDK 提前释放）
 *   2. 获取帧数据（支持 I420 CPU 格式和 D3D11 GPU 格式）
 *   3. 封装为智能指针（自动管理生命周期）
 *   4. 通过信号发送到主线程
 * 
 * 注意：此函数调用频率较高，需要高效处理
 */
void StreamingViewModel::VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    if (!self || !frame_handle) return;
    
    // 【步骤1】获取视频帧缓冲区
    // SDK API: tcr_video_frame_get_buffer(frame_handle)
    const TcrVideoFrameBuffer* frame_buffer = tcr_video_frame_get_buffer(frame_handle);
    if (!frame_buffer) {
        return;
    }
    
    // 【步骤2】增加引用计数
    // SDK API: tcr_video_frame_add_ref(frame_handle)
    // 防止 SDK 在我们使用期间释放帧数据
    tcr_video_frame_add_ref(frame_handle);

    // 【步骤3】根据缓冲区类型创建对应的VideoFrameData对象
    // VideoFrameDataPtr 析构时会自动调用 tcr_video_frame_release()
    VideoFrameDataPtr frameDataPtr;
    
    if (frame_buffer->type == TCR_VIDEO_BUFFER_TYPE_I420) {
        // I420格式：CPU内存中的YUV数据
        const TcrI420Buffer& i420Buffer = frame_buffer->buffer.i420;
        
        // 使用I420构造函数创建对象
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
        
        // 构造D3D11TextureData结构
        D3D11TextureData textureData;
        textureData.texture = d3d11Buffer.texture;
        textureData.device = d3d11Buffer.device;
        textureData.array_index = d3d11Buffer.array_index;
        textureData.format = d3d11Buffer.format;
        
        // 使用D3D11构造函数创建对象
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

    // 【步骤6】发送到主线程进行渲染
    emit self->newVideoFrame(frameDataPtr);
}

// ==================== 摄像头设备管理 ====================

QStringList StreamingViewModel::getCameraDeviceList()
{
    QStringList deviceList;
    
    if (!m_session) {
        Logger::debug("[getCameraDeviceList] session not ready");
        return deviceList;
    }
    
    // 【步骤1】获取摄像头设备数量
    // SDK API: tcr_session_get_camera_device_count(session)
    int32_t deviceCount = tcr_session_get_camera_device_count(m_session);
    Logger::info(QString("[getCameraDeviceList] 检测到 %1 个摄像头设备").arg(deviceCount));
    
    // 【步骤2】遍历获取每个设备的信息
    for (int32_t i = 0; i < deviceCount; ++i) {
        TcrCameraDeviceInfo deviceInfo;
        
        // SDK API: tcr_session_get_camera_device(session, index, &deviceInfo)
        if (tcr_session_get_camera_device(m_session, i, &deviceInfo)) {
            QString deviceId = QString::fromUtf8(deviceInfo.device_id);
            QString deviceName = QString::fromUtf8(deviceInfo.device_name);
            
            Logger::info(QString("[getCameraDeviceList] 设备 %1: ID=%2, Name=%3")
                             .arg(i)
                             .arg(deviceId)
                             .arg(deviceName));
            
            // 将设备ID添加到列表（也可以选择添加设备名称）
            deviceList << deviceId;
        } else {
            Logger::error(QString("[getCameraDeviceList] 获取设备 %1 信息失败")
                              .arg(i));
        }
    }
    
    return deviceList;
}
