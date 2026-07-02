#pragma once

#include <QObject>
#include <QPointer>
#include <QQuickItem>

class QKeyEvent;
class QMouseEvent;
class QWheelEvent;

// 通用键鼠捕获 QQuickItem：覆盖父级区域，把 Qt 键鼠/滚轮事件归一化为
// w3c 风格字段（key/code 小写、button 字符串、modifiers 数组、wheel deltaY 行数），
// 通过信号转发给上层 ViewModel（如 DesktopViewModel）做坐标变换与序列化。
//
// 存在原因：云桌面场景下云端为 Windows 系统，TcrSdk 内置的触摸/按键接口仅适用于
// 云手机（Android），因此客户端必须自行捕获并序列化键鼠事件，通过自定义数据通道发送。
// 当前仅用于云桌面场景；设计上不耦合任何 ViewModel 类型。
// mousemove 事件节流到 ~60fps，避免淹没数据通道。
class InputCaptureItem : public QQuickItem {
  Q_OBJECT

 public:
  explicit InputCaptureItem(QQuickItem* parent = nullptr);
  ~InputCaptureItem() override;

 signals:
  // 键盘按下/抬起。key 为小写逻辑键名（如 "a"/"shift"/"arrowleft"），
  // code 为 w3c KeyboardEvent.code 物理键名（如 "KeyA"/"ShiftLeft"），
  // modifiers 当前按下的修饰键列表（"shift"/"control"/"alt"/"meta"）。
  void keyEvent(bool pressed, const QString& key, const QString& code, const QStringList& modifiers);

  // 鼠标按下/释放/移动。type 取值 "mousedown"/"mouseup"/"mousemove"。
  // localX/localY 为相对本 Item 的本地像素坐标（未做云端映射）。
  // button 为本次状态变化的按键（"left"/"right"/"middle"/"x1"/"x2"），
  // buttons 为事件后当前按下的全部按键。
  void mouseInput(const QString& type, qreal localX, qreal localY, const QString& button, const QStringList& buttons);

  // 滚轮。deltaX/deltaY 为行数（上负下正，左负右正）。
  void wheelInput(qreal localX, qreal localY, int deltaX, int deltaY, const QStringList& buttons);

  // 焦点丢失（窗口失活/隐藏/最小化时触发），ViewModel 收到后应发 reset。
  void focusLost();

 protected:
  void keyPressEvent(QKeyEvent* event) override;
  void keyReleaseEvent(QKeyEvent* event) override;
  void mousePressEvent(QMouseEvent* event) override;
  void mouseReleaseEvent(QMouseEvent* event) override;
  void mouseMoveEvent(QMouseEvent* event) override;
  void wheelEvent(QWheelEvent* event) override;
  void itemChange(ItemChange change, const ItemChangeData& data) override;
  QSGNode* updatePaintNode(QSGNode* node, UpdatePaintNodeData* data) override;

 private:
  // Qt 按键 → w3c key/code 字符串
  static QString qtKeyToW3cKey(int qtKey);
  static QString qtKeyToW3cCode(int qtKey, Qt::KeyboardModifiers mods);
  static QString qtMouseButtonToString(Qt::MouseButton button);
  static QStringList currentMouseButtons(Qt::MouseButtons buttons);
  static QStringList currentModifiers(Qt::KeyboardModifiers mods);

  // mousemove 节流：上次发出 mousemove 的时间戳（ms），<=0 表示未发过
  qint64 m_lastMouseMoveMs = 0;
  static constexpr int kMouseMoveThrottleMs = 16;  // ~60fps
};
