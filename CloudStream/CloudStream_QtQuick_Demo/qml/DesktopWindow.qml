import QtQuick 2.15
import QtQuick.Controls 2.15
import CustomComponents 1.0

// 云桌面窗口，与 StreamingWindow 平级。
// 不复用 StreamingWindow 的原因：云端为 Windows 系统，SDK 内置的触摸/按键接口
// （tcr_session_touchscreen_touch 等）仅适用于云手机（Android），云桌面场景的键鼠事件
// 需由 InputCaptureItem 捕获后通过自定义数据通道发送，因此需要独立的 ViewModel 和 UI 布局。
// 仅渲染单个实例（instanceIds[0]）+ 键鼠捕获转发，不含 StreamingWindow 的业务按钮控件。
// 窗口 1280x720 / 16:9 PC 比例，F11 切全屏。
ApplicationWindow {
    id: desktopWindow
    width: 1280
    height: 720
    title: "云桌面 - " + (instanceIds.length > 0 ? instanceIds[0] : "")
    visible: true
    color: "black"

    // 由 InstanceTokenWindow 创建窗口时注入
    property var instanceIds: []
    property var accessInfo: ({})
    property string token: ""
    property DesktopViewModel desktopViewModel: DesktopViewModel {}

    // 渲染层（z:0）：裸 VideoRenderPaintedItem，不嵌 VideoRenderArea
    // （VideoRenderArea 内部 MouseArea 会拦截云手机触摸事件，与云桌面键鼠语义冲突）
    VideoRenderPaintedItem {
        id: videoRenderItem
        objectName: "desktopVideoRenderItem"
        anchors.fill: parent
        z: 0
    }

    // 输入捕获层（z:1）：透明背景，覆盖在渲染层之上拦截所有键鼠事件
    InputCaptureItem {
        id: inputCaptureItem
        anchors.fill: parent
        z: 1
    }

    // F11 切换全屏
    Shortcut {
        sequence: "F11"
        onActivated: {
            desktopWindow.visibility = (desktopWindow.visibility === Window.FullScreen
                                       ? Window.Windowed
                                       : Window.FullScreen)
        }
    }

    Component.onCompleted: {
        desktopViewModel.setVideoRenderItem(videoRenderItem)
        desktopViewModel.setInputCaptureItem(inputCaptureItem)
        // 让输入捕获层拿到键盘焦点
        inputCaptureItem.forceActiveFocus()
        // 云桌面只取第一个实例
        if (instanceIds.length > 0) {
            desktopViewModel.connectSession([instanceIds[0]], false)
        }
    }

    // 窗口失活/隐藏 → 发 reset，避免云端按键卡住
    onActiveChanged: {
        if (!active) {
            desktopViewModel.sendReset()
        }
    }

    onVisibilityChanged: {
        if (visibility === Window.Hidden || visibility === Window.Minimized) {
            desktopViewModel.sendReset()
        }
    }
}
