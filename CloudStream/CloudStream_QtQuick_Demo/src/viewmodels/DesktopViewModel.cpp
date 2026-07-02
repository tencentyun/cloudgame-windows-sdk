#include "viewmodels/DesktopViewModel.h"

#include <QDateTime>
#include <QGuiApplication>
#include <QJsonArray>
#include <QJsonDocument>
#include <QJsonObject>
#include <QPointF>
#include <QVariant>

#include "core/input/InputCaptureItem.h"
#include "core/StreamConfig.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "utils/VariantListConverter.h"

namespace {
// 数据通道配置：使用 label 型通道，label 直接由调用方指定
constexpr char DATA_CHANNEL_LABEL[] = "desktop_input";

// RequestId 缓冲区大小
constexpr size_t REQUEST_ID_BUFFER_SIZE = 256;
}  // namespace

// ==================== 构造与析构 ====================

DesktopViewModel::DesktopViewModel(QObject* parent) : QObject(parent) {
  Logger::info("[DesktopViewModel] 构造函数");

  qRegisterMetaType<VideoFrameData>("VideoFrameData");
  qRegisterMetaType<VideoFrameDataPtr>("VideoFrameDataPtr");

  m_sessionObserver.user_data = this;
  m_sessionObserver.on_event = &DesktopViewModel::SessionEventCallback;

  m_videoFrameObserver.user_data = this;
  m_videoFrameObserver.on_frame = &DesktopViewModel::VideoFrameCallback;
}

DesktopViewModel::~DesktopViewModel() {
  Logger::info("[~DesktopViewModel] 开始析构");
  m_isDestroying.store(true, std::memory_order_release);

  if (m_videoRenderItem) {
    disconnect(this, &DesktopViewModel::newVideoFrame, m_videoRenderItem, &VideoRenderPaintedItem::setFrame);
  }

  closeSession();
  Logger::info("[~DesktopViewModel] 析构完成");
}

// ==================== 渲染组件绑定 ====================

void DesktopViewModel::setVideoRenderItem(VideoRenderPaintedItem* item) {
  if (m_videoRenderItem) {
    disconnect(this, &DesktopViewModel::newVideoFrame, m_videoRenderItem, &VideoRenderPaintedItem::setFrame);
    Logger::info(QString("[setVideoRenderItem] 断开旧渲染组件: %1").arg(m_videoRenderItem->objectName()));
  }

  m_videoRenderItem = item;

  if (m_videoRenderItem) {
    // 注意：不调用 item->setStreamingViewModel(...)。
    // 云桌面场景下触摸/滚轮由 InputCaptureItem 接管，VideoRenderPaintedItem 的触摸转发逻辑不启用。
    connect(this, &DesktopViewModel::newVideoFrame, m_videoRenderItem, &VideoRenderPaintedItem::setFrame,
            static_cast<Qt::ConnectionType>(Qt::QueuedConnection | Qt::UniqueConnection));
    Logger::info(QString("[setVideoRenderItem] 已连接渲染组件: %1").arg(m_videoRenderItem->objectName()));
  } else {
    Logger::info("[setVideoRenderItem] 渲染组件已置空");
  }
}

void DesktopViewModel::setVideoRenderItem(QObject* item) {
  auto vrItem = qobject_cast<VideoRenderPaintedItem*>(item);
  if (vrItem) {
    setVideoRenderItem(vrItem);
  } else {
    Logger::warning("[setVideoRenderItem] cast to VideoRenderPaintedItem* failed");
  }
}

// ==================== 输入捕获组件绑定 ====================

void DesktopViewModel::setInputCaptureItem(InputCaptureItem* item) {
  if (m_inputCaptureItem) {
    disconnect(m_inputCaptureItem, nullptr, this, nullptr);
    Logger::info("[setInputCaptureItem] 断开旧输入组件");
  }

  m_inputCaptureItem = item;

  if (m_inputCaptureItem) {
    // C++ 内部建立信号连接（类比 StreamingViewModel 的 setVideoRenderItem 模式）
    connect(m_inputCaptureItem, &InputCaptureItem::keyEvent, this, &DesktopViewModel::onKeyEvent);
    connect(m_inputCaptureItem, &InputCaptureItem::mouseInput, this, &DesktopViewModel::onMouseEvent);
    connect(m_inputCaptureItem, &InputCaptureItem::wheelInput, this, &DesktopViewModel::onWheelEvent);
    connect(m_inputCaptureItem, &InputCaptureItem::focusLost, this, &DesktopViewModel::onFocusLost);
    Logger::info("[setInputCaptureItem] 已连接输入组件");
  } else {
    Logger::info("[setInputCaptureItem] 输入组件已置空");
  }
}

void DesktopViewModel::setInputCaptureItem(QObject* item) {
  auto icItem = qobject_cast<InputCaptureItem*>(item);
  if (icItem) {
    setInputCaptureItem(icItem);
  } else {
    Logger::warning("[setInputCaptureItem] cast to InputCaptureItem* failed");
  }
}

// ==================== TcrSdk 客户端初始化 ====================

void DesktopViewModel::initialize(const QStringList& instanceIds, const QString& accessInfo, const QString& token) {
  Logger::info("[initialize] 云桌面场景开始初始化 TcrSdk");
  Logger::info(QString("[initialize] Instance IDs: %1").arg(instanceIds.join(", ")));
  Logger::info(QString("[initialize] Access Info: %1").arg(accessInfo));

  // 配置 TcrSdk 初始化参数（与 MultiStreamViewModel::initialize 等价）
  std::string tokenStr = token.toStdString();
  std::string accessInfoStr = accessInfo.toStdString();

  TcrConfig config = tcr_config_default();
  config.token = tokenStr.c_str();
  config.accessInfo = accessInfoStr.c_str();

  // 获取 TcrClient 全局单例并初始化
  TcrClientHandle client = tcr_client_get_instance();
  TcrErrorCode result = tcr_client_init(client, &config);
  if (result != TCR_SUCCESS) {
    Logger::error(QString("[initialize] TcrSdk 初始化失败, code=%1").arg(result));
    return;
  }

  Logger::info("[initialize] TcrSdk 初始化成功");
}

// ==================== 会话管理 ====================

void DesktopViewModel::connectSession(const QVariantList& instanceIds, bool isGroupControl) {
  if (instanceIds.isEmpty()) {
    Logger::debug("[connectSession] 实例ID列表为空");
    return;
  }

  // 云桌面场景只渲染单画面，固定取第一个实例
  QString instanceId = instanceIds.first().toString();
  Logger::info(QString("[connectSession] 云桌面模式，实例ID: %1").arg(instanceId));

  createAndInitSession();

  if (m_session) {
    QVariantList singleList;
    singleList << instanceId;
    auto result = VariantListConverter::convert(singleList);
    if (!result.pointers.empty()) {
      tcr_session_access(m_session, result.pointers.data(), static_cast<int32_t>(result.pointers.size()), false);
    } else {
      Logger::error("[connectSession] 没有可用的InstanceId");
    }
  } else {
    Logger::error("[connectSession] 创建Session失败");
  }
}

void DesktopViewModel::closeSession() {
  if (m_session) {
    Logger::info("[closeSession] 开始关闭会话");

    tcr_session_set_observer(m_session, nullptr);
    tcr_session_set_video_frame_observer(m_session, nullptr);

    if (m_tcrClient) {
      tcr_client_destroy_session(m_tcrClient, m_session);
    }

    m_session = nullptr;
    m_sessionConnected = false;
    m_dataChannel = nullptr;
  }
}

void DesktopViewModel::createAndInitSession() {
  closeSession();

  if (!m_tcrClient) {
    m_tcrClient = tcr_client_get_instance();
  }
  m_instance = tcr_client_get_android_instance(m_tcrClient);

  TcrSessionConfig config = tcr_session_config_default();

  StreamConfig* streamConfig = StreamConfig::instance();
  config.stream_profile.video_width = streamConfig->mainStreamWidth();
  config.stream_profile.fps = streamConfig->mainStreamFps();
  config.stream_profile.max_bitrate = streamConfig->mainStreamMaxBitrate();
  config.stream_profile.min_bitrate = streamConfig->mainStreamMinBitrate();

  Logger::info(QString("[createAndInitSession] 大流参数 - 宽:%1, 帧率:%2, 码率:%3-%4")
                   .arg(config.stream_profile.video_width)
                   .arg(config.stream_profile.fps)
                   .arg(config.stream_profile.min_bitrate)
                   .arg(config.stream_profile.max_bitrate));

  m_session = tcr_client_create_session(m_tcrClient, &config);
  setSessionObservers();
}

void DesktopViewModel::setSessionObservers() {
  if (m_session) {
    tcr_session_set_observer(m_session, &m_sessionObserver);
    tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
  }
}

// ==================== 数据通道 ====================

void DesktopViewModel::createDataChannel() {
  static TcrDataChannelObserver s_dc_observer = {
      nullptr,
      [](void* user_data, int32_t port) {
        auto* self = static_cast<DesktopViewModel*>(user_data);
        Logger::info(QString("[DesktopDataChannel] connected, label=%1 port=%2").arg(DATA_CHANNEL_LABEL).arg(port));
      },
      [](void* user_data, int32_t port, const TcrErrorCode& code, const char* msg) {
        Logger::error(
            QString("[DesktopDataChannel] error on port %1, code=%2, msg=%3").arg(port).arg(code).arg(msg ? msg : ""));
      },
      [](void* user_data, int32_t port, const uint8_t* data, size_t size) {
        // 云端回传消息（如键鼠操作的回执），目前只打印
        if (data && size > 0) {
          QByteArray rawData(reinterpret_cast<const char*>(data), static_cast<int>(size));
          Logger::debug(QString("[DesktopDataChannel] recv on port %1: %2").arg(port).arg(QString::fromUtf8(rawData)));
        }
      }};
  s_dc_observer.user_data = this;

  m_dataChannel = tcr_session_create_data_channel_with_label(m_session, DATA_CHANNEL_LABEL, &s_dc_observer);

  if (m_dataChannel) {
    Logger::info(QString("[DesktopDataChannel] create success, label=%1").arg(DATA_CHANNEL_LABEL));
  } else {
    Logger::error("[DesktopDataChannel] create failed");
  }
}

void DesktopViewModel::sendJson(const QJsonObject& payload) {
  if (!m_dataChannel) {
    Logger::warning("[DesktopInput] dataChannel 未就绪，丢弃事件");
    return;
  }

  QJsonDocument doc(payload);
  QByteArray data = doc.toJson(QJsonDocument::Compact);

  TcrErrorCode code =
      tcr_data_channel_send(m_dataChannel, reinterpret_cast<const uint8_t*>(data.constData()), data.size());

  // 同时打印，便于调试
  Logger::info(QString("[DesktopInput] → %1 (code=%2)").arg(QString::fromUtf8(data)).arg(code));
}

// ==================== 输入事件槽（来自 InputCaptureItem） ====================

void DesktopViewModel::onKeyEvent(bool pressed, const QString& key, const QString& code, const QStringList& modifiers) {
  QJsonObject payload;
  payload["type"] = pressed ? "keydown" : "keyup";
  payload["timestamp"] = static_cast<qint64>(QDateTime::currentMSecsSinceEpoch());
  payload["key"] = key;
  payload["code"] = code;
  QJsonArray mods;
  for (const QString& m : modifiers) mods << m;
  payload["modifiers"] = mods;
  sendJson(payload);
}

void DesktopViewModel::onMouseEvent(const QString& type, qreal localX, qreal localY, const QString& button,
                                    const QStringList& buttons) {
  QPointF desktop = localToDesktop(localX, localY);

  QJsonObject payload;
  payload["type"] = type;
  payload["timestamp"] = static_cast<qint64>(QDateTime::currentMSecsSinceEpoch());
  payload["x"] = static_cast<int>(desktop.x());
  payload["y"] = static_cast<int>(desktop.y());
  if (!button.isEmpty()) {
    payload["button"] = button;
  }
  QJsonArray btns;
  for (const QString& b : buttons) btns << b;
  payload["buttons"] = btns;
  sendJson(payload);
}

void DesktopViewModel::onWheelEvent(qreal localX, qreal localY, int deltaX, int deltaY, const QStringList& buttons) {
  QPointF desktop = localToDesktop(localX, localY);

  QJsonObject payload;
  payload["type"] = "mousewheel";
  payload["timestamp"] = static_cast<qint64>(QDateTime::currentMSecsSinceEpoch());
  payload["x"] = static_cast<int>(desktop.x());
  payload["y"] = static_cast<int>(desktop.y());
  payload["deltaX"] = deltaX;
  payload["deltaY"] = deltaY;
  QJsonArray btns;
  for (const QString& b : buttons) btns << b;
  payload["buttons"] = btns;
  sendJson(payload);
}

void DesktopViewModel::onFocusLost() {
  // 焦点丢失时发 reset，避免云端按键卡住
  sendReset();
}

void DesktopViewModel::sendReset() {
  QJsonObject payload;
  payload["type"] = "reset";
  payload["timestamp"] = static_cast<qint64>(QDateTime::currentMSecsSinceEpoch());
  sendJson(payload);
}

// ==================== 坐标变换 ====================

QPointF DesktopViewModel::localToDesktop(qreal localX, qreal localY) const {
  // 本地像素 → 云端桌面像素
  // 云端桌面分辨率来自 REMOTE_DESKTOP_INFO；本地渲染区域尺寸来自 InputCaptureItem 的 width/height
  // 若任一未就绪，回退到原坐标（不阻塞输入，但坐标可能不准）
  if (m_desktopWidth <= 0 || m_desktopHeight <= 0 || !m_inputCaptureItem) {
    return QPointF(localX, localY);
  }

  qreal w = m_inputCaptureItem->width();
  qreal h = m_inputCaptureItem->height();
  if (w <= 0 || h <= 0) {
    return QPointF(localX, localY);
  }

  qreal scaleX = static_cast<qreal>(m_desktopWidth) / w;
  qreal scaleY = static_cast<qreal>(m_desktopHeight) / h;
  return QPointF(localX * scaleX, localY * scaleY);
}

// ==================== SDK 事件回调 ====================

void DesktopViewModel::SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData) {
  DesktopViewModel* self = static_cast<DesktopViewModel*>(user_data);
  QString eventDataCopy = eventData ? QString::fromUtf8(eventData) : QString();

  if (event == TCR_SESSION_EVENT_STATE_CONNECTED) {
    self->handleSessionConnected();
  } else if (event == TCR_SESSION_EVENT_STATE_CLOSED) {
    self->handleSessionClosed(eventDataCopy);
  } else if (event == TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO) {
    self->handleRemoteDesktopInfo(eventDataCopy);
  }
  // 云桌面场景不处理 SCREEN_CONFIG_CHANGE / CAMERA_STATUS 等
}

void DesktopViewModel::handleSessionConnected() {
  char requestIdBuffer[REQUEST_ID_BUFFER_SIZE] = {0};
  if (tcr_session_get_request_id(m_session, requestIdBuffer, sizeof(requestIdBuffer))) {
    Logger::info(QString("[handleSessionConnected] 云桌面会话连接成功, RequestId: %1").arg(requestIdBuffer));
  } else {
    Logger::info("[handleSessionConnected] 云桌面会话连接成功");
  }

  m_sessionConnected = true;
  createDataChannel();
}

void DesktopViewModel::handleSessionClosed(const QString& eventData) {
  m_sessionConnected = false;
  Logger::error("[handleSessionClosed] 云桌面会话断开: " + eventData);
}

void DesktopViewModel::handleRemoteDesktopInfo(const QString& eventData) {
  Logger::info("[handleRemoteDesktopInfo] 云端桌面信息: " + eventData);

  QJsonDocument doc = QJsonDocument::fromJson(eventData.toUtf8());
  if (!doc.isObject()) {
    Logger::warning("[handleRemoteDesktopInfo] JSON 解析失败");
    return;
  }

  QJsonObject obj = doc.object();
  int newWidth = obj.value("screen_width").toInt();
  int newHeight = obj.value("screen_height").toInt();

  if (newWidth > 0 && newHeight > 0) {
    m_desktopWidth = newWidth;
    m_desktopHeight = newHeight;
    Logger::info(QString("[handleRemoteDesktopInfo] 云端桌面分辨率: %1x%2").arg(m_desktopWidth).arg(m_desktopHeight));
    emit desktopInfoChanged();
  } else {
    Logger::warning(QString("[handleRemoteDesktopInfo] 分辨率无效: %1x%2").arg(newWidth).arg(newHeight));
  }
}

// ==================== SDK 视频帧回调 ====================

void DesktopViewModel::VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
  DesktopViewModel* self = static_cast<DesktopViewModel*>(user_data);
  if (!self || !frame_handle) return;

  if (self->m_isDestroying.load(std::memory_order_acquire)) {
    return;
  }

  const TcrVideoFrameBuffer* frame_buffer = tcr_video_frame_get_buffer(frame_handle);
  if (!frame_buffer) {
    return;
  }

  tcr_video_frame_add_ref(frame_handle);

  if (frame_buffer->type != TCR_VIDEO_BUFFER_TYPE_I420) {
    Logger::warning(QString("[DesktopVideoFrame] 未知的帧类型: %1").arg(frame_buffer->type));
    tcr_video_frame_release(frame_handle);
    return;
  }

  const TcrI420Buffer& i420 = frame_buffer->buffer.i420;
  VideoFrameDataPtr frameDataPtr(new VideoFrameData(frame_handle, i420.data_y, i420.data_u, i420.data_v, i420.stride_y,
                                                    i420.stride_u, i420.stride_v, i420.width, i420.height,
                                                    frame_buffer->timestamp_us));

  if (self->m_isDestroying.load(std::memory_order_acquire) || !self->m_videoRenderItem) {
    tcr_video_frame_release(frame_handle);
    return;
  }

  emit self->newVideoFrame(frameDataPtr);
}
