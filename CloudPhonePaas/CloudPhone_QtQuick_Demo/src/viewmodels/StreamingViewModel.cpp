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
{
    QGuiApplication::instance()->installEventFilter(this);
    qRegisterMetaType<VideoFrameData>("VideoFrameData");
    qRegisterMetaType<VideoFrameDataPtr>("VideoFrameDataPtr");

    // 初始化 observer 结构体
    m_sessionObserver.user_data = this;
    m_sessionObserver.on_event = &StreamingViewModel::SessionEventCallback;

    m_videoFrameObserver.user_data = this;
    m_videoFrameObserver.on_frame = &StreamingViewModel::VideoFrameCallback;
}
StreamingViewModel::~StreamingViewModel()
{
    closeSession();
}

void StreamingViewModel::setTcrOperator(BatchTaskOperator* tcrOperator)
{
    m_tcrOperator = tcrOperator;
}

void StreamingViewModel::setVideoRenderItem(VideoRenderItem* item)
{
    m_videoRenderItem = item;
    if (m_videoRenderItem) {
        connect(this, &StreamingViewModel::newVideoFrame, m_videoRenderItem, &VideoRenderItem::setFrame, Qt::QueuedConnection);
    }
}

void StreamingViewModel::setVideoRenderItem(QObject* item)
{
    auto vrItem = qobject_cast<VideoRenderItem*>(item);
    if (vrItem) {
        setVideoRenderItem(vrItem);
    } else {
        qWarning() << "setVideoRenderItem: cast to VideoRenderItem* failed";
    }
}

void StreamingViewModel::handleMouseEvent(int x, int y, int width, int height, int eventType, qint64 timestamp)
{
    if (m_session) {
        tcr_session_touchscreen_touch(m_session, x, y, eventType, width, height, timestamp);
    }
}

void StreamingViewModel::updateCheckedInstanceIds(const QVariantList& instanceIds)
{
    Logger::info("[updateCheckedInstanceIds] 最新勾选的AndroidInstanceId:");
    for (const QVariant& instanceId : instanceIds) {
        Logger::info(QString("  %1").arg(instanceId.toString()));
    }

    auto result = VariantListConverter::convert(instanceIds);
    if (m_session && !result.pointers.empty()) {
        tcr_instance_join_group(m_instance, result.pointers.data(), static_cast<int32_t>(result.pointers.size()));
    } else {
        Logger::debug("[SetSyncList] 没有可用的InstanceId或Session未初始化");
    }
}

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

void StreamingViewModel::createAndInitSession()
{
    closeSession();
    if (!m_tcrClient) {
        m_tcrClient = tcr_client_get_instance();
    }
    m_instance = tcr_client_get_android_instance(m_tcrClient);
    m_session = tcr_client_create_session(m_tcrClient);
    setSessionObservers();
}

void StreamingViewModel::setSessionObservers()
{
    if (m_session) {
        tcr_session_set_observer(m_session, &m_sessionObserver);
        tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
    }
}

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

void StreamingViewModel::connectGroupSession(const QVariantList& instanceIds)
{
    m_groupInstanceIds.clear();
    for (const QVariant& id : instanceIds) {
        m_groupInstanceIds << id.toString();
    }
    Logger::debug(QString("[connectGroupSession] m_groupInstanceIds:%1").arg(m_groupInstanceIds.join(",")));

    createAndInitSession();
    if (m_session) {
        auto result = VariantListConverter::convert(instanceIds);
        if (!result.pointers.empty()) {
            tcr_session_access(m_session, result.pointers.data(), static_cast<int32_t>(result.pointers.size()), true);
        } else {
            Logger::debug("[connectGroupSession] 没有可用的InstanceId");
        }
    } else {
        Logger::debug("[connectGroupSession] 创建Session失败");
    }
}

void StreamingViewModel::onBackClicked()
{
    if (m_session) {
        tcr_session_send_keyboard_event(m_session, 158, true);
        tcr_session_send_keyboard_event(m_session, 158, false);
    }
}

void StreamingViewModel::onHomeClicked()
{
    if (m_session) {
        tcr_session_send_keyboard_event(m_session, 172, true);
        tcr_session_send_keyboard_event(m_session, 172, false);
    }
}

void StreamingViewModel::onMenuClicked()
{
    if (m_session) {
        tcr_session_send_keyboard_event(m_session, 139, true);
        tcr_session_send_keyboard_event(m_session, 139, false);
    }
}

void StreamingViewModel::onVolumUp()
{
    if (m_session) {
        tcr_session_send_keyboard_event(m_session, 58, true);
        tcr_session_send_keyboard_event(m_session, 58, false);
    }
}

void StreamingViewModel::onVolumDown()
{
    if (m_session) {
        tcr_session_send_keyboard_event(m_session, 59, true);
        tcr_session_send_keyboard_event(m_session, 59, false);
    }
}

void StreamingViewModel::onPauseStreamClicked()
{
    if (m_session) {
        tcr_session_pause_streaming(m_session);
    }
}

void StreamingViewModel::onResumeStreamClicked()
{
    if (m_session) {
        tcr_session_resume_streaming(m_session);
    }
}

void StreamingViewModel::onClickPaste(const QString& inputText)
{
    if (m_session) {
        tcr_session_paste_text(m_session, inputText.toStdString().c_str());
    }
}

void StreamingViewModel::onChangeBitrateClicked(int framerate, int minBitrate, int maxBitrate)
{
    Logger::debug(QString("Set bitrate: [%1, %2] kbps, framerate: %3 fps").arg(minBitrate).arg(maxBitrate).arg(framerate));
    if (m_session) {
        tcr_session_set_remote_video_profile(m_session, framerate, minBitrate, maxBitrate);
    }
}

bool StreamingViewModel::eventFilter(QObject* obj, QEvent* event)
{
    const QEvent::Type type = event->type();
    if (type != QEvent::KeyPress && type != QEvent::KeyRelease)
        return QObject::eventFilter(obj, event);

    const int keycode = static_cast<QKeyEvent*>(event)->key();

    // 输入框聚焦的情况下, 所有按键事件交给QML输入框处理
    if (QGuiApplication::focusObject() && QGuiApplication::focusObject()->objectName() == "popupInputField") {
        return false;
    }

    // 本地输入法
    if (m_imeType == "local") {
        // 在按下非回车键、非删除键、非退格键时弹出输入框，
        if (keycode != Qt::Key_Return
            && keycode != Qt::Key_Enter
            && keycode != Qt::Key_Delete
            && keycode != Qt::Key_Control
            && keycode != Qt::Key_Backspace) {
            emit requestShowInputBox();
            return false;
        }
    }

    // 非 local 模式
    if (!m_session)
        return false;

    int winVKcode = static_cast<QKeyEvent*>(event)->nativeVirtualKey();

    Logger::debug(QString("[eventFilter] keycode: %1, type: %2").arg(winVKcode).arg(type));
    // 发送键盘事件，true=按下，false=抬起
    tcr_session_send_keyboard_event(m_session, winVKcode, type == QEvent::KeyPress);
    // 拦截事件，不让继续传递
    return true;
}

void StreamingViewModel::onInputTextFromQml(const QString& text)
{
    // 你可以在这里处理输入的文字，比如发送到远端
    qDebug() << "Received input from QML:" << text;
    emit inputTextReceived(text);
    if (m_session) {
        tcr_session_paste_text(m_session, text.toStdString().c_str());
    }
}

void StreamingViewModel::setLocalIME() {
    if (m_session) {
        tcr_session_switch_ime(m_session, "local");
    }
}

void StreamingViewModel::setCloudIME() {
    if (m_session) {
        tcr_session_switch_ime(m_session, "cloud");
    }
}

// ========== 会话观察者实现 ==========

void StreamingViewModel::SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();

    QMetaObject::invokeMethod(self, [self, event, eventDataCopy]() {
        if (event == TCR_SESSION_EVENT_STATE_CONNECTED) {
            // 这部分操作只有群控会话才需要做
            bool isGroupConnection = !self->m_groupInstanceIds.isEmpty();
            if (isGroupConnection) {
                Logger::debug(QString("[SessionEventCallback] m_groupInstanceIds:%1").arg(self->m_groupInstanceIds.join(",")));
                std::string masterId = self->m_groupInstanceIds.first().toStdString();
                const char* masterIdStr = masterId.c_str();
                tcr_instance_set_master(self->m_instance, masterIdStr, true);
                tcr_instance_request_stream(self->m_instance, masterIdStr, true, "high");
            }
        } else if (event == TCR_SESSION_EVENT_STATE_CLOSED) {
            // 会话关闭，或连接失败
            Logger::error("[SessionEventCallback] 会话断开: " + eventDataCopy);
        } else if (event == TCR_SESSION_EVENT_IME_STATUS_CHANGE) {
            QJsonParseError parseError;
            QJsonDocument doc = QJsonDocument::fromJson(eventDataCopy.toUtf8(), &parseError);
            if (parseError.error != QJsonParseError::NoError) {
                Logger::error(QString("[SessionEventCallback] 解析IME状态变更eventData失败: %1, 原始数据: %2")
                            .arg(parseError.errorString(), eventDataCopy));
                return;
            }
            if (!doc.isObject()) {
                Logger::error(QString("[SessionEventCallback] IME状态变更eventData不是JSON对象: %1").arg(eventDataCopy));
                return;
            }
            QJsonObject obj = doc.object();
            QString imeType = obj.value("ime_type").toString();
            Logger::info(QString("[SessionEventCallback] IME状态变更, ime_type: %1").arg(imeType));
            if (self->m_imeType != imeType) {
                self->m_imeType = imeType;
                emit self->imeTypeChanged();
            }
        }

        
    }, Qt::QueuedConnection);
}


void StreamingViewModel::VideoFrameCallback(void* user_data, const TcrVideoFrame* video_frame)
{
    StreamingViewModel* self = static_cast<StreamingViewModel*>(user_data);
    if (!video_frame) return;

    const TcrI420Buffer& buffer = video_frame->i420_buffer;

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
