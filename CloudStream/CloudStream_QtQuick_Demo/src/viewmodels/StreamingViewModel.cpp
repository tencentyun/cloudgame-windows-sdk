#include "StreamingViewModel.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "utils/VariantListConverter.h"
#include <QVariant>
#include <QDebug>
#include <QMetaType>
#include <QGuiApplication>

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
    m_videoFrameObserver.on_frame_buffer = &StreamingViewModel::VideoFrameCallback;
}

StreamingViewModel::~StreamingViewModel()
{
    closeSession();
}

// 把 QML 侧的 VideoRenderItem 传入 C++，用于渲染视频帧
void StreamingViewModel::setVideoRenderItem(VideoRenderItem* item)
{
    m_videoRenderItem = item;
    if (m_videoRenderItem) {
        // 使用 QueuedConnection 确保线程安全
        connect(this, &StreamingViewModel::newVideoFrame,
                m_videoRenderItem, &VideoRenderItem::setFrame,
                Qt::QueuedConnection);
    }
}

// QML 侧调用的重载版本，自动做类型转换
void StreamingViewModel::setVideoRenderItem(QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderItem*>(item);
    if (vrItem) {
        setVideoRenderItem(vrItem);
    } else {
        qWarning() << "setVideoRenderItem: cast to VideoRenderItem* failed";
    }
}

// 发送触摸事件到云端实例
void StreamingViewModel::handleMouseEvent(int x, int y, int width, int height,
                                          int eventType, qint64 timestamp)
{
    if (m_session && m_sessionConnected) {
        tcr_session_touchscreen_touch(m_session, x, y, eventType, width, height, timestamp);
    }
}

// 群控场景下动态更新需要同步的实例列表
void StreamingViewModel::updateCheckedInstanceIds(const QVariantList& instanceIds)
{
    Logger::info("[updateCheckedInstanceIds] 最新勾选的AndroidInstanceId:");
    for (const QVariant& instanceId : instanceIds) {
        Logger::info(QString("  %1").arg(instanceId.toString()));
    }

    auto result = VariantListConverter::convert(instanceIds);
    if (m_session && !result.pointers.empty()) {
        tcr_instance_join_group(m_instance,
                                result.pointers.data(),
                                static_cast<int32_t>(result.pointers.size()));
    } else {
        Logger::debug("[SetSyncList] 没有可用的InstanceId或Session未初始化");
    }
}

// 主动关闭当前会话，释放 TcrSdk 资源
void StreamingViewModel::closeSession()
{
    if (m_session) {
        tcr_session_set_observer(m_session, nullptr);
        tcr_session_set_video_frame_observer(m_session, nullptr);
        if (m_tcrClient) {
            tcr_client_destroy_session(m_tcrClient, m_session);
        }
        m_session = nullptr;
    }
}

// 创建并初始化会话
void StreamingViewModel::createAndInitSession()
{
    closeSession();
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance(); // 单例，不会重复创建
    }
    m_instance = tcr_client_get_android_instance(m_tcrClient);
    m_session = tcr_client_create_session(m_tcrClient);
    setSessionObservers();
}

// 给当前 m_session 注册 observer
void StreamingViewModel::setSessionObservers()
{
    if (m_session) {
        tcr_session_set_observer(m_session, &m_sessionObserver);
        tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
    }
}

void StreamingViewModel::onBackClicked()
{
    if (m_session && m_sessionConnected) {
        tcr_session_send_keyboard_event(m_session, 158, true);
        tcr_session_send_keyboard_event(m_session, 158, false);
    }
}

void StreamingViewModel::onHomeClicked()
{
    if (m_session && m_sessionConnected) {
        tcr_session_send_keyboard_event(m_session, 172, true);
        tcr_session_send_keyboard_event(m_session, 172, false);
    }
}

void StreamingViewModel::onMenuClicked()
{
    if (m_session && m_sessionConnected) {
        tcr_session_send_keyboard_event(m_session, 139, true);
        tcr_session_send_keyboard_event(m_session, 139, false);
    }
}

void StreamingViewModel::onVolumUp()
{
    if (m_session && m_sessionConnected) {
        tcr_session_send_keyboard_event(m_session, 58, true);
        tcr_session_send_keyboard_event(m_session, 58, false);
    }
}

void StreamingViewModel::onVolumDown()
{
    if (m_session && m_sessionConnected) {
        tcr_session_send_keyboard_event(m_session, 59, true);
        tcr_session_send_keyboard_event(m_session, 59, false);
    }
}

void StreamingViewModel::requestStreaming(bool paused)
{
    if (!m_session || !m_sessionConnected) {
        Logger::debug("[requestStreaming] session not ready");
        return;
    }

    const bool isGroup = !m_groupInstanceIds.isEmpty();
    if (isGroup) { 
        // 群控场景下改变推流状态
        if (m_groupInstanceIds.isEmpty()) return;
        const std::string masterId = m_groupInstanceIds.first().toStdString();
        tcr_instance_request_stream(m_instance,
                                    masterId.c_str(),
                                    !paused,
                                    "high");
        Logger::debug(QString("[requestStreaming] group %1 stream")
                          .arg(paused ? "pause" : "resume"));
    } else { 
        // 单实例场景下改变推流状态
        if (paused)
            tcr_session_pause_streaming(m_session);
        else
            tcr_session_resume_streaming(m_session);
        Logger::debug(QString("[requestStreaming] single %1 stream")
                          .arg(paused ? "pause" : "resume"));
    }
}

// 调整：QML 触发的暂停 -> 直接调用内部函数
void StreamingViewModel::onPauseStreamClicked()
{
    requestStreaming(true);
}

// 调整：QML 触发的恢复 -> 直接调用内部函数
void StreamingViewModel::onResumeStreamClicked()
{
    requestStreaming(false);
}

// 单实例连接（QML 调用）
void StreamingViewModel::connectSession(const QString& instanceId)
{
    Logger::debug(QString("[connectSession] 点击卡片，AndroidInstanceId:%1").arg(instanceId));
    createAndInitSession();
    if (m_session) {
        std::string idStr = instanceId.toStdString();
        const char* idPtr = idStr.c_str();
        tcr_session_access(m_session, &idPtr, 1, false);
    } else {
        Logger::debug("[connectSession] 创建Session失败");
    }
}

// 群控连接（QML 调用）
void StreamingViewModel::connectGroupSession(const QVariantList& instanceIds)
{
    m_groupInstanceIds.clear();
    for (const QVariant& id : instanceIds) {
        m_groupInstanceIds << id.toString();
    }
    Logger::debug(QString("[connectGroupSession] m_groupInstanceIds:%1")
                      .arg(m_groupInstanceIds.join(",")));

    createAndInitSession();
    if (m_session) {
        auto result = VariantListConverter::convert(instanceIds);
        if (!result.pointers.empty()) {
            tcr_session_access(m_session,
                               result.pointers.data(),
                               static_cast<int32_t>(result.pointers.size()),
                               true);
        } else {
            Logger::debug("[connectGroupSession] 没有可用的InstanceId");
        }
    } else {
        Logger::debug("[connectGroupSession] 创建Session失败");
    }
}

// ========== 会话观察者实现 ==========
// 会话事件回调
// SDK 会在以下时机调用：
// - 连接成功：TCR_SESSION_EVENT_STATE_CONNECTED
// - 连接断开：TCR_SESSION_EVENT_STATE_CLOSED
// - 摄像头状态变化：TCR_SESSION_EVENT_CAMERA_STATUS
// 内部通过 QMetaObject::invokeMethod 切换到主线程处理
void StreamingViewModel::SessionEventCallback(void* user_data,
                                              TcrSessionEvent event,
                                              const char* eventData)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();

    QMetaObject::invokeMethod(self, [self, event, eventDataCopy]() {
        if (event == TCR_SESSION_EVENT_STATE_CONNECTED) {
            Logger::info("[SessionEventCallback] 会话连接成功");
            self->m_sessionConnected = true;

            if (!self->m_groupInstanceIds.isEmpty()) {
                Logger::debug(QString("[SessionEventCallback] group ids:%1")
                                  .arg(self->m_groupInstanceIds.join(",")));

                std::string masterId = self->m_groupInstanceIds.first().toStdString();
                tcr_instance_set_master(self->m_instance, masterId.c_str(), true);

                self->requestStreaming(false);
            }
        } else if (event == TCR_SESSION_EVENT_CAMERA_STATUS) {
            // 处理摄像头状态变更事件
            QString eventDataStr = eventDataCopy;
            QJsonDocument doc = QJsonDocument::fromJson(eventDataStr.toUtf8());
            if (doc.isObject()) {
                QJsonObject obj = doc.object();
                QString status = obj.value("status").toString();

                if (status == "open_back" || status == "open_front") {
                    // 开启本地摄像头
                    tcr_session_enable_local_camera(self->m_session, true);
                } else if (status == "close") {
                    // 关闭本地摄像头
                    tcr_session_disable_local_camera(self->m_session);
                }
            }
        } else if (event == TCR_SESSION_EVENT_STATE_CLOSED) {
            // 会话关闭，或连接失败
            self->m_sessionConnected = false;
            Logger::error("[SessionEventCallback] 会话断开: " + eventDataCopy);
        }
    }, Qt::QueuedConnection);
}

// 视频帧回调（解码线程 -> 主线程）
// 把视频帧裸数据通过 Qt 信号抛到主线程
void StreamingViewModel::VideoFrameCallback(void* user_data,
                                            const TcrVideoFrameBuffer* frame_buffer,
                                            int64_t timestamp_us,
                                            TcrVideoRotation rotation)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    if (!frame_buffer) return;

    const TcrI420Buffer& buffer = frame_buffer->buffer.i420;

    VideoFrameDataPtr frameDataPtr(new VideoFrameData);
    frameDataPtr->width = buffer.width;
    frameDataPtr->height = buffer.height;
    frameDataPtr->strideY = buffer.stride_y;
    frameDataPtr->strideU = buffer.stride_u;
    frameDataPtr->strideV = buffer.stride_v;

    int ySize = frameDataPtr->strideY * frameDataPtr->height;
    int uSize = frameDataPtr->strideU * (frameDataPtr->height / 2);
    int vSize = frameDataPtr->strideV * (frameDataPtr->height / 2);

    frameDataPtr->y.resize(ySize);
    frameDataPtr->u.resize(uSize);
    frameDataPtr->v.resize(vSize);

    memcpy(frameDataPtr->y.data(), buffer.data_y, ySize);
    memcpy(frameDataPtr->u.data(), buffer.data_u, uSize);
    memcpy(frameDataPtr->v.data(), buffer.data_v, vSize);

    emit self->newVideoFrame(frameDataPtr);
}
