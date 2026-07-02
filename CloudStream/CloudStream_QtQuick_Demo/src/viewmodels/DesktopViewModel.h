#pragma once

#include <atomic>
#include <QObject>
#include <QPointer>
#include <QStringList>

#include "core/video/Frame.h"
#include "core/video/VideoRenderPaintedItem.h"
#include "tcr_c_api.h"

class InputCaptureItem;

// 云桌面会话 ViewModel，与 StreamingViewModel 平级。
// 对应 InstanceTokenWindow 选「云桌面」后的场景：单画面渲染 + 键鼠输入。
//
// 【为何需要独立的 DesktopViewModel】
// 云手机和云桌面的串流/音视频/数据通道底层是同一份 TcrSdk，差异在控制输入路径：
//   - 云手机：SDK 内置的触摸/按键接口（tcr_session_touchscreen_touch 等）面向 Android 实例设计
//   - 云桌面：云端为 Windows 系统，上述接口不适用，必须由客户端自行捕获键鼠事件、
//     归一化后通过自定义数据通道（tcr_data_channel_send）发送
// 因此 DesktopViewModel 去掉 StreamingViewModel 的触摸/按键/设备控制接口，
// 新增 InputCaptureItem → JSON → 数据通道这条输入路径。
//
// 职责（StreamingViewModel 的子集 + 自定义数据通道输入）：
//   1. connectSession / closeSession —— 同云手机，走 TcrSdk 完整流程
//   2. setVideoRenderItem —— 建立 newVideoFrame → VideoRenderPaintedItem::setFrame 连接
//      （不调用 item.setStreamingViewModel，避免云手机触摸转发逻辑介入）
//   3. setInputCaptureItem —— 连接 InputCaptureItem 的键鼠/滚轮/焦点信号到本类槽
//   4. 订阅 TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO，缓存云端桌面分辨率
//      desktopWidth/desktopHeight 暴露为 Q_PROPERTY，供 QML 做坐标映射参考
//   5. createDataChannel —— tcr_session_create_data_channel
//   6. 归一化键鼠事件 → JSON（按 docs/desktop_input_protocol.md）→ tcr_data_channel_send + Logger 打印
//   7. sendReset —— 焦点丢失/窗口隐藏时发 {"type":"reset"}
class DesktopViewModel : public QObject {
  Q_OBJECT
  Q_PROPERTY(int desktopWidth READ desktopWidth NOTIFY desktopInfoChanged)
  Q_PROPERTY(int desktopHeight READ desktopHeight NOTIFY desktopInfoChanged)

 public:
  explicit DesktopViewModel(QObject* parent = nullptr);
  ~DesktopViewModel() override;

  // 设置渲染组件：连接 newVideoFrame → VideoRenderPaintedItem::setFrame
  void setVideoRenderItem(VideoRenderPaintedItem* item);
  // QML 调用入口（QObject 重载，内部 cast）
  Q_INVOKABLE void setVideoRenderItem(QObject* item);

  // 设置输入捕获组件：建立键鼠/滚轮/焦点信号连接
  void setInputCaptureItem(InputCaptureItem* item);
  Q_INVOKABLE void setInputCaptureItem(QObject* item);

  // TcrSdk 客户端全局初始化（用 token + accessInfo 配置 TcrConfig 并 tcr_client_init）
  // 必须在 connectSession 之前调用一次，否则 createSession 会失败（错误: please init first）
  // 与 MultiStreamViewModel::initialize 等价——StreamingViewModel 自身不调 tcr_client_init，
  // 它依赖 MainWindow 启动时调 multiInstanceViewModel.initialize 完成全局初始化；
  // 云桌面路径不经过 MainWindow，所以 DesktopViewModel 必须自己承担这一步。
  Q_INVOKABLE void initialize(const QStringList& instanceIds, const QString& accessInfo, const QString& token);

  // 连接云桌面实例（单实例：取 instanceIds[0]）
  Q_INVOKABLE void connectSession(const QVariantList& instanceIds, bool isGroupControl);

  // 关闭会话
  Q_INVOKABLE void closeSession();

  // 焦点/窗口状态变化时 QML 调用，发 reset 事件
  Q_INVOKABLE void sendReset();

  // 云端桌面分辨率（来自 REMOTE_DESKTOP_INFO 事件）
  int desktopWidth() const { return m_desktopWidth; }
  int desktopHeight() const { return m_desktopHeight; }

 signals:
  void newVideoFrame(VideoFrameDataPtr frame);
  void desktopInfoChanged();

 private slots:
  // InputCaptureItem 信号槽
  void onKeyEvent(bool pressed, const QString& key, const QString& code, const QStringList& modifiers);
  void onMouseEvent(const QString& type, qreal localX, qreal localY, const QString& button, const QStringList& buttons);
  void onWheelEvent(qreal localX, qreal localY, int deltaX, int deltaY, const QStringList& buttons);
  void onFocusLost();

 private:
  // 会话管理
  void createAndInitSession();
  void setSessionObservers();
  bool isSessionReady() const { return m_session && m_sessionConnected; }

  // 数据通道
  void createDataChannel();
  // 序列化 JSON 并发送到数据通道；打印日志
  void sendJson(const QJsonObject& payload);

  // 事件处理
  void handleSessionConnected();
  void handleSessionClosed(const QString& eventData);
  void handleRemoteDesktopInfo(const QString& eventData);

  // 坐标变换：本地像素 → 云端桌面像素
  // localX/localY 相对 InputCaptureItem，需要 renderAreaWidth/Height 做缩放
  QPointF localToDesktop(qreal localX, qreal localY) const;

  // SDK 回调（静态，C 风格）
  static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData);
  static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle);

  // ==================== 成员变量 ====================
  QPointer<VideoRenderPaintedItem> m_videoRenderItem;
  QPointer<InputCaptureItem> m_inputCaptureItem;
  std::atomic<bool> m_isDestroying{false};

  TcrClientHandle m_tcrClient = nullptr;
  TcrSessionHandle m_session = nullptr;
  TcrAndroidInstance m_instance = nullptr;
  TcrDataChannelHandle m_dataChannel = nullptr;

  TcrSessionObserver m_sessionObserver = {};
  TcrVideoFrameObserver m_videoFrameObserver = {};

  bool m_sessionConnected = false;

  // 云端桌面分辨率
  int m_desktopWidth = 0;
  int m_desktopHeight = 0;
};
