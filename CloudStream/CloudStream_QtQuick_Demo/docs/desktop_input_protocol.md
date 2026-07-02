# 云桌面输入数据通道协议

> 本文件定义 `DesktopViewModel` 通过 TcrSdk 自定义数据通道
> （`tcr_session_create_data_channel_with_label` / `tcr_data_channel_send`）
> 向云端 PC 桌面发送的 JSON 输入事件协议。
>
> **适用范围**：仅 Demo 工程的「云桌面」场景。云手机场景的输入不走本协议
> （云手机通过 `VideoRenderPaintedItem` 的触摸事件转发，不经过自定义数据通道）。
>
> **协议性质**：本协议是 **Demo 级示例**，仅作为「云桌面 + 自定义数据通道」如何对接的参考。
> 实际业务方可以定义自己的协议（消息格式、字段语义、状态机），**只要把消息体通过
> `tcr_data_channel_send` 发到约定好的自定义数据通道 label 上即可**。
> 云端只需在同 label 的通道上接收并按业务方自定义的协议解析，无需与本文件一致。
>
> **背景**：云端 PC 桌面运行 Windows 系统，TcrSdk 内置的触摸/按键控制接口
> （`tcr_session_touchscreen_touch`、`tcr_session_send_keyboard_event` 等）
> 仅面向 Android 云手机设计。因此云桌面场景下，客户端必须自行捕获键鼠事件、
> 定义输入消息格式，并通过 TcrSdk 的自定义数据通道（`tcr_data_channel_send`）发送。
> 本协议即定义这一自定义消息格式。

---

## 1. 设计目标

| 目标 | 说明 |
|---|---|
| 语义自解释 | 一个 JSON 即一个完整输入事件，读 JSON 即知意图 |
| 易扩展 | 用判别式联合（`type` 字段区分事件类型），新增事件类型不破坏旧解析器 |
| 可调试 | 字段命名贴近浏览器/w3c 输入事件习惯，便于理解 |

## 2. 总体结构

每个事件是一个 JSON 对象，**必须**包含 `type` 字段，其余字段按 `type` 取值决定。

```json
{
  "type": "<event_type>",
  "timestamp": 1700000000000,
  "...": "按 type 决定的负载字段"
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `type` | string | 是 | 事件类型，取值见 §3 |
| `timestamp` | number | 否 | 客户端生成的事件时间戳（毫秒，epoch），调试用，云端可忽略 |

`type` 取值集合（共 7 种）：

| `type` | 含义 | 负载字段 |
|---|---|---|
| `mousemove` | 鼠标移动 | `x`, `y`, `buttons` |
| `mousedown` | 鼠标键按下 | `x`, `y`, `button`, `buttons` |
| `mouseup` | 鼠标键释放 | `x`, `y`, `button`, `buttons` |
| `mousewheel` | 滚轮滚动 | `x`, `y`, `deltaX`, `deltaY`, `buttons` |
| `keydown` | 键盘键按下 | `key`, `code`, `modifiers` |
| `keyup` | 键盘键释放 | `key`, `code`, `modifiers` |
| `reset` | 输入状态复位 | （无） |

## 3. 事件类型详解

### 3.1 鼠标坐标约定（重要）

> **所有鼠标事件的 `x` / `y` 字段，单位为「云端桌面的像素坐标」**，
> 原点为云端桌面屏幕左上角，`x` 向右递增，`y` 向下递增。
>
> 客户端在发送前，必须用本地窗口的渲染画面尺寸除以云端桌面分辨率
> （来自 `TCR_SESSION_EVENT_REMOTE_DESKTOP_INFO` 事件，由 `DesktopViewModel`
> 缓存并通过 `desktopWidth` / `desktopHeight` 属性暴露给 QML）做坐标映射：
>
> ```
> cloud_x = local_window_x * (desktopWidth  / renderAreaWidth)
> cloud_y = local_window_y * (desktopHeight / renderAreaHeight)
> ```
>
> 这保证了云端收到的坐标与其自身桌面分辨率一致，无需再做二次换算。

### 3.2 `mousemove`

鼠标在云端桌面上的移动。**不携带按键状态变化**，只表示"光标当前位置"。

```json
{
  "type": "mousemove",
  "timestamp": 1700000000000,
  "x": 540,
  "y": 360,
  "buttons": []
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `x` | number | 是 | 云端桌面横坐标（px） |
| `y` | number | 是 | 云端桌面纵坐标（px） |
| `buttons` | string[] | 否 | 当前按下的鼠标键集合，取值见 §4.1。可省略（视为 `[]`） |

**节流建议**：客户端应节流（如每 16ms / 60fps 上限），避免淹没数据通道。
`InputCaptureItem` 在 C++ 侧做节流，schema 不强制。

### 3.3 `mousedown` / `mouseup`

鼠标键按下 / 释放。`x` / `y` 为事件发生时的光标位置。

```json
{
  "type": "mousedown",
  "timestamp": 1700000000000,
  "x": 540,
  "y": 360,
  "button": "left",
  "buttons": ["left"]
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `x` | number | 是 | 云端桌面横坐标（px） |
| `y` | number | 是 | 云端桌面纵坐标（px） |
| `button` | string | 是 | 本次状态发生变化的按键，取值见 §4.1 |
| `buttons` | string[] | 否 | 事件后当前按下的全部按键集合（含本次） |

> `button` 是"本次变化的那个键"；`buttons` 是"事件后所有按下的键"。
> 二者并存方便云端做幂等处理：只看 `buttons` 也能复原状态。

### 3.4 `mousewheel`

滚轮滚动。`deltaX` / `deltaY` 取浏览器/w3c 习惯：向上滚 `deltaY < 0`，向下滚 `deltaY > 0`，
单位为"行"（典型单次滚动值 ±1 或 ±3，由 `InputCaptureItem` 按 Qt `wheelEvent.angleDelta().y() / 120` 归一）。

```json
{
  "type": "mousewheel",
  "timestamp": 1700000000000,
  "x": 540,
  "y": 360,
  "deltaX": 0,
  "deltaY": -3,
  "buttons": []
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `x` | number | 是 | 滚轮事件发生时光标的云端桌面坐标 |
| `y` | number | 是 | 同上 |
| `deltaX` | number | 否 | 水平滚动量（行），可省略（视为 0） |
| `deltaY` | number | 是 | 垂直滚动量（行），上负下正 |
| `buttons` | string[] | 否 | 事件时按下的鼠标键集合 |

### 3.5 `keydown` / `keyup`

键盘键按下 / 释放。

```json
{
  "type": "keydown",
  "timestamp": 1700000000000,
  "key": "a",
  "code": "KeyA",
  "modifiers": ["shift"]
}
```

| 字段 | 类型 | 必填 | 说明 |
|---|---|---|---|
| `key` | string | 是 | 逻辑键名，**小写**。详见 §4.2 |
| `code` | string | 是 | 物理键位 code，采用 [KeyboardEvent.code](https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_code_values) 命名（如 `KeyA` / `ArrowLeft` / `ShiftLeft`） |
| `modifiers` | string[] | 否 | 事件时按下的修饰键集合，取值见 §4.3。可省略（视为 `[]`） |

> `key` 与 `code` 区别（与 w3c 一致）：
> - `key` 表示"按这个键产生的字符/逻辑名"——按 Shift+a 时 `key="a"`，单独按 a 也是 `key="a"`。
>   字母一律小写；功能键用名称（`enter` / `escape` / `arrowleft` / `f1` ...）。
> - `code` 表示"按的是键盘上哪个物理位置"——`KeyA` 永远是 A 键位置，
>   与大小写状态无关。这让云端能在不同键盘布局下复原输入。

### 3.6 `reset`

输入状态复位。客户端在失去焦点、窗口隐藏、连接断开时发送，
通知云端"清除一切按下状态、复位光标"。无负载字段。

```json
{
  "type": "reset",
  "timestamp": 1700000000000
}
```

## 4. 枚举值定义

### 4.1 鼠标键 `button` / `buttons[]`

| 字符串 | 含义 |
|---|---|
| `"left"` | 主键（通常左键） |
| `"right"` | 次键（通常右键） |
| `"middle"` | 中键（滚轮按下） |
| `"x1"` | 扩展键 1（侧键 back） |
| `"x2"` | 扩展键 2（侧键 forward） |

对应 Qt `Qt::MouseButton`：`LeftButton / RightButton / MiddleButton / BackButton / ForwardButton`。

### 4.2 键盘逻辑键名 `key`（节选）

| 类别 | `key` 取值 | 说明 |
|---|---|---|
| 字母 | `"a"` ~ `"z"` | 一律小写 |
| 数字 | `"0"` ~ `"9"` | 顶部数字行 |
| 符号 | `"-"`, `"="`, `"["`, `"]"`, `"\\"`, `";"`, `"'"`, `` "` "`", `","`, `"."`, `"/"` | 按 US 布局产生 |
| 空格 | `"space"` | |
| 回车 | `"enter"` | |
| 退格 | `"backspace"` | |
| Tab | `"tab"` | |
| Esc | `"escape"` | |
| 方向键 | `"arrowup"`, `"arrowdown"`, `"arrowleft"`, `"arrowright"` | |
| 功能键 | `"f1"` ~ `"f24"` | |
| Home/End/PgUp/PgDn/Insert/Delete | `"home"`, `"end"`, `"pageup"`, `"pagedown"`, `"insert"`, `"delete"` | |
| 修饰键 | `"shift"`, `"control"`, `"alt"`, `"meta"` | 大小写状态无关 |

> 完整列表对齐 [w3c `KeyboardEvent.key`](https://developer.mozilla.org/en-US/docs/Web/API/UI_Events/Keyboard_event_key_values)。
> 未列出者按 w3c 文档小写形式补全。

### 4.3 修饰键 `modifiers[]`

| 字符串 | 对应 Qt | 对应 `code`（按下时） |
|---|---|---|
| `"shift"` | `Qt::ShiftModifier` | `ShiftLeft` / `ShiftRight` |
| `"control"` | `Qt::ControlModifier` | `ControlLeft` / `ControlRight` |
| `"alt"` | `Qt::AltModifier` | `AltLeft` / `AltRight` |
| `"meta"` | `Qt::MetaModifier` | `MetaLeft` / `MetaRight`（macOS Command / Windows Win 键） |

修饰键作为独立的 `keydown`/`keyup` 事件也会发送一次，
**同时**在所有键鼠事件的 `modifiers` / `buttons` 字段里反映当前状态。这样云端可以选择
"只看事件流"或"只看 modifiers 字段"两种方式复原状态。

## 5. 序列化与传输

- **编码**：UTF-8。
- **传输**：调用 `tcr_data_channel_send(channel, reinterpret_cast<const uint8_t*>(json.data()), json.size())`。
  一个事件一个 JSON 字符串，不加分隔符——数据通道本身是消息边界清晰的（SDK 按调用次数发送）。
- **打印**：客户端在发送时同时 `Logger::info` 打印 JSON 文本，便于调试。
  打印格式建议 `[DesktopInput] → <type> <json>`。

## 5.1 数据通道 label 与本协议的关系

Demo 中通过 label 型自定义数据通道承载本协议，关键约定：

| 项 | 取值 | 说明 |
|---|---|---|
| 数据通道 API | `tcr_session_create_data_channel_with_label` | label 型，不走 `prefix+port` 拼接与 udp_trans 握手 |
| label | `"desktop_input"` | 客户端发送、云端接收需在 **同一 label** 的 WebRTC DataChannel 上 |
| 协议字段 | 见 §3 | 本节定义的消息体格式仅在 `desktop_input` label 通道上使用 |

> 为什么用 `with_label` 而不是 `create_data_channel`？
> 云桌面场景下，客户端只需要一个「直发 JSON 消息给云端」的通道，
> 不需要云端再开 UDP 端口做转发。`with_label` 跳过 udp_trans 握手、
> channel 打开后即视为就绪，更贴合"通道即消息总线"的语义。

## 5.2 替换为业务方自定义协议

本文件是 Demo 级的协议示例。**实际业务方可以自由定义自己的协议**——消息体可以是 JSON、
Protobuf、FlatBuffers、自定义二进制结构等，只要满足以下两条即可：

1. **客户端**：用 `tcr_session_create_data_channel_with_label` 创建一个自定义 label 的通道，
   然后用 `tcr_data_channel_send` 把业务消息体发出去。
2. **云端**：在同一 label 的 WebRTC DataChannel 上接收消息，按业务方自定义的协议解析。

换句话说，本节§3 之后定义的事件类型 / 字段 / 枚举值是 **Demo 的选择**，**不是 SDK 的约束**。
业务方在 `desktop_input` 这个 label 上完全可以发完全不同的内容——只要云端也按业务方的协议解析。

### 5.2.1 客户端替换示例：换成 Protobuf 协议

```cpp
// 1) 创建通道，label 保持一致
m_dataChannel = tcr_session_create_data_channel_with_label(m_session, "desktop_input", &s_dc_observer);

// 2) 发送时直接序列化业务 protobuf
MyInputEvent event;
event.set_type(INPUT_KEYDOWN);
event.set_keycode(65);
auto bytes = event.SerializeAsString();
tcr_data_channel_send(m_dataChannel,
                      reinterpret_cast<const uint8_t*>(bytes.data()),
                      bytes.size());
```

### 5.2.2 客户端替换示例：换一条独立通道承载另一种业务

```cpp
// 用另一个 label 的通道承载剪贴板同步业务，协议自定义
m_clipboardChannel = tcr_session_create_data_channel_with_label(
    m_session, "desktop_clipboard", &s_clipboardObserver);
// 按自己的协议（如 CB1、CB2 信令）发消息，云端在 "desktop_clipboard" label 通道上接收
```

> **关键点**：label 是通道间的"路由标识"，不是协议本身。
> 一个 label = 一个独立的数据通道。云端在每个 label 上独立接收、按该 label 约定的协议解析。
> 业务方可以按需开多个 label 通道，每个通道跑自己的协议，**互不影响**。

---

## 6. 生命周期与状态管理

| 客户端事件 | 客户端应发送 | 说明 |
|---|---|---|
| 数据通道 `on_connected` 回调 | —— | 通道就绪，等待用户输入 |
| 鼠标移动 / 按键 / 滚轮 | 对应事件 | 实时发送 |
| `InputCaptureItem` 失去焦点 | `reset` | 防止云端"按键卡住" |
| 窗口隐藏 / 最小化 | `reset` | 同上 |
| 会话断开 / 数据通道关闭 | —— | 客户端停止发送，无需显式 `reset`（云端会话已结束） |

## 7. 示例会话片段

```
[DesktopInput] → mousemove {"type":"mousemove","x":100,"y":100,"buttons":[]}
[DesktopInput] → mousedown  {"type":"mousedown","x":100,"y":100,"button":"left","buttons":["left"]}
[DesktopInput] → mousemove {"type":"mousemove","x":120,"y":100,"buttons":["left"]}
[DesktopInput] → mouseup    {"type":"mouseup","x":120,"y":100,"button":"left","buttons":[]}
[DesktopInput] → keydown    {"type":"keydown","key":"shift","code":"ShiftLeft","modifiers":["shift"]}
[DesktopInput] → keydown    {"type":"keydown","key":"a","code":"KeyA","modifiers":["shift"]}
[DesktopInput] → keyup      {"type":"keyup","key":"a","code":"KeyA","modifiers":["shift"]}
[DesktopInput] → keyup      {"type":"keyup","key":"shift","code":"ShiftLeft","modifiers":[]}
[DesktopInput] → mousewheel {"type":"mousewheel","x":120,"y":100,"deltaY":-3,"buttons":[]}
[DesktopInput] → reset       {"type":"reset"}
```

## 8. 变更记录

| 版本 | 日期 | 变更 |
|---|---|---|
| 1.0.0 | 2026-07-02 | 初版：定义 7 种事件类型、键鼠枚举、坐标系约定 |
| 1.1.0 | 2026-07-02 | 改用 `tcr_session_create_data_channel_with_label`；新增 §5.1 解释 label 与协议的关系、§5.2 给出业务方替换为自定义协议的示例 |
