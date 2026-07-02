#include "core/input/InputCaptureItem.h"

#include <QDateTime>
#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

// ==================== 构造与析构 ====================

InputCaptureItem::InputCaptureItem(QQuickItem* parent) : QQuickItem(parent) {
  // 接收键盘焦点：键鼠捕获必须先拿到焦点
  setFlag(ItemAcceptsInputMethod, true);
  setFlag(ItemIsFocusScope, true);
  setAcceptedMouseButtons(Qt::LeftButton | Qt::RightButton | Qt::MiddleButton | Qt::BackButton | Qt::ForwardButton);
  setAcceptHoverEvents(false);  // 我们要的是真实鼠标按钮事件，hover 不发
  // 透明背景：渲染层（VideoRenderPaintedItem）在下层照常绘制
}

InputCaptureItem::~InputCaptureItem() = default;

// 渲染节点：永远返回空，本 Item 不绘制任何内容（纯事件捕获层）
QSGNode* InputCaptureItem::updatePaintNode(QSGNode* node, UpdatePaintNodeData*) { return nullptr; }

// ==================== 焦点变化 ====================

void InputCaptureItem::itemChange(ItemChange change, const ItemChangeData& data) {
  QQuickItem::itemChange(change, data);
  if (change == ItemActiveFocusHasChanged) {
    if (!hasActiveFocus()) {
      // 焦点丢失 → 通知 ViewModel 发 reset
      emit focusLost();
    }
  }
}

// ==================== 键盘事件 ====================

void InputCaptureItem::keyPressEvent(QKeyEvent* event) {
  if (event->isAutoRepeat()) {
    return;  // 忽略自动重复，只发首次按下
  }
  QString key = qtKeyToW3cKey(event->key());
  QString code = qtKeyToW3cCode(event->key(), event->modifiers());
  QStringList mods = currentModifiers(event->modifiers());
  emit keyEvent(true, key, code, mods);
  event->accept();
}

void InputCaptureItem::keyReleaseEvent(QKeyEvent* event) {
  if (event->isAutoRepeat()) {
    return;
  }
  QString key = qtKeyToW3cKey(event->key());
  QString code = qtKeyToW3cCode(event->key(), event->modifiers());
  QStringList mods = currentModifiers(event->modifiers());
  emit keyEvent(false, key, code, mods);
  event->accept();
}

// ==================== 鼠标事件 ====================

void InputCaptureItem::mousePressEvent(QMouseEvent* event) {
  QString button = qtMouseButtonToString(event->button());
  QStringList buttons = currentMouseButtons(event->buttons() | event->button());
  emit mouseInput("mousedown", event->position().x(), event->position().y(), button, buttons);
  event->accept();
}

void InputCaptureItem::mouseReleaseEvent(QMouseEvent* event) {
  QString button = qtMouseButtonToString(event->button());
  // QMouseEvent::buttons() 在 release 时已经不含本次 button
  QStringList buttons = currentMouseButtons(event->buttons());
  emit mouseInput("mouseup", event->position().x(), event->position().y(), button, buttons);
  event->accept();
}

void InputCaptureItem::mouseMoveEvent(QMouseEvent* event) {
  // 节流到 ~60fps：上次发出到现在不足阈值则丢弃
  qint64 now = QDateTime::currentMSecsSinceEpoch();
  if (m_lastMouseMoveMs > 0 && (now - m_lastMouseMoveMs) < kMouseMoveThrottleMs) {
    event->accept();
    return;
  }
  m_lastMouseMoveMs = now;

  // mousemove 没有"本次变化"的 button，传空字符串
  QStringList buttons = currentMouseButtons(event->buttons());
  emit mouseInput("mousemove", event->position().x(), event->position().y(), QString(), buttons);
  event->accept();
}

void InputCaptureItem::wheelEvent(QWheelEvent* event) {
  // angleDelta().y()/120 = 行数（向上为正，向下为负）
  int deltaY = event->angleDelta().y() / 120;
  int deltaX = event->angleDelta().x() / 120;
  QStringList buttons = currentMouseButtons(event->buttons());
  QPointF pos = event->position();
  emit wheelInput(pos.x(), pos.y(), deltaX, deltaY, buttons);
  event->accept();
}

// ==================== Qt → w3c 映射 ====================

// 返回小写逻辑键名。字母一律小写；修饰键/功能键用语义名。
QString InputCaptureItem::qtKeyToW3cKey(int qtKey) {
  // 字母 A-Z
  if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
    return QChar(Qt::Key_A - 'A' + 'a' + (qtKey - Qt::Key_A));
  }
  // 数字 0-9
  if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
    return QChar('0' + (qtKey - Qt::Key_0));
  }
  // 功能键 F1-F24
  if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24) {
    return QString("f%1").arg(qtKey - Qt::Key_F1 + 1);
  }
  // 方向键
  switch (qtKey) {
    case Qt::Key_Up:
      return "arrowup";
    case Qt::Key_Down:
      return "arrowdown";
    case Qt::Key_Left:
      return "arrowleft";
    case Qt::Key_Right:
      return "arrowright";
    case Qt::Key_Space:
      return "space";
    case Qt::Key_Return:
    case Qt::Key_Enter:
      return "enter";
    case Qt::Key_Backspace:
      return "backspace";
    case Qt::Key_Tab:
      return "tab";
    case Qt::Key_Escape:
      return "escape";
    case Qt::Key_Home:
      return "home";
    case Qt::Key_End:
      return "end";
    case Qt::Key_PageUp:
      return "pageup";
    case Qt::Key_PageDown:
      return "pagedown";
    case Qt::Key_Insert:
      return "insert";
    case Qt::Key_Delete:
      return "delete";
    case Qt::Key_Shift:
      return "shift";
    case Qt::Key_Control:
      return "control";
    case Qt::Key_Alt:
      return "alt";
    case Qt::Key_Meta:
      return "meta";
    default:
      break;
  }
  // 符号键：直接转 char（小写）
  if (qtKey <= 0xFFFF && qtKey >= 0x20 && QChar(qtKey).isPrint()) {
    QChar c(qtKey);
    return QString(c.toLower());
  }
  // 未识别：返回 "unidentified"
  return "unidentified";
}

// 返回 w3c KeyboardEvent.code 物理键名。简化版——按主键猜物理位置，
// 修饰键区分左右目前不实现（只返回通用名），后续若需要可扩展。
QString InputCaptureItem::qtKeyToW3cCode(int qtKey, Qt::KeyboardModifiers mods) {
  if (qtKey >= Qt::Key_A && qtKey <= Qt::Key_Z) {
    return QString("Key%1").arg(QChar('A' + (qtKey - Qt::Key_A)));
  }
  if (qtKey >= Qt::Key_0 && qtKey <= Qt::Key_9) {
    return QString("Digit%1").arg(QChar('0' + (qtKey - Qt::Key_0)));
  }
  if (qtKey >= Qt::Key_F1 && qtKey <= Qt::Key_F24) {
    return QString("F%1").arg(qtKey - Qt::Key_F1 + 1);
  }
  switch (qtKey) {
    case Qt::Key_Up:
      return "ArrowUp";
    case Qt::Key_Down:
      return "ArrowDown";
    case Qt::Key_Left:
      return "ArrowLeft";
    case Qt::Key_Right:
      return "ArrowRight";
    case Qt::Key_Space:
      return "Space";
    case Qt::Key_Return:
    case Qt::Key_Enter:
      return "Enter";
    case Qt::Key_Backspace:
      return "Backspace";
    case Qt::Key_Tab:
      return "Tab";
    case Qt::Key_Escape:
      return "Escape";
    case Qt::Key_Home:
      return "Home";
    case Qt::Key_End:
      return "End";
    case Qt::Key_PageUp:
      return "PageUp";
    case Qt::Key_PageDown:
      return "PageDown";
    case Qt::Key_Insert:
      return "Insert";
    case Qt::Key_Delete:
      return "Delete";
    case Qt::Key_Shift:
      return "ShiftLeft";
    case Qt::Key_Control:
      return "ControlLeft";
    case Qt::Key_Alt:
      return "AltLeft";
    case Qt::Key_Meta:
      return "MetaLeft";
    default:
      break;
  }
  return "Unidentified";
}

// Qt 鼠标键 → 协议字符串
QString InputCaptureItem::qtMouseButtonToString(Qt::MouseButton button) {
  switch (button) {
    case Qt::LeftButton:
      return "left";
    case Qt::RightButton:
      return "right";
    case Qt::MiddleButton:
      return "middle";
    case Qt::BackButton:
      return "x1";
    case Qt::ForwardButton:
      return "x2";
    default:
      return "";
  }
}

// 当前按下的全部鼠标键 → 字符串列表
QStringList InputCaptureItem::currentMouseButtons(Qt::MouseButtons buttons) {
  QStringList list;
  if (buttons & Qt::LeftButton) list << "left";
  if (buttons & Qt::RightButton) list << "right";
  if (buttons & Qt::MiddleButton) list << "middle";
  if (buttons & Qt::BackButton) list << "x1";
  if (buttons & Qt::ForwardButton) list << "x2";
  return list;
}

// 当前按下的修饰键 → 字符串列表
QStringList InputCaptureItem::currentModifiers(Qt::KeyboardModifiers mods) {
  QStringList list;
  if (mods & Qt::ShiftModifier) list << "shift";
  if (mods & Qt::ControlModifier) list << "control";
  if (mods & Qt::AltModifier) list << "alt";
  if (mods & Qt::MetaModifier) list << "meta";
  return list;
}
