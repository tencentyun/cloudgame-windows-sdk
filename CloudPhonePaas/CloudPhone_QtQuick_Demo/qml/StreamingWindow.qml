import QtQuick 2.15
import QtQuick.Controls 2.15
import CustomComponents 1.0

ApplicationWindow {
    id: streamingWindow
    property StreamingViewModel streamingViewModel

    title: "串流窗口"
    width: 800
    height: 1024
    visible: false

    onClosing: {
        if (streamingViewModel) {
            streamingViewModel.closeSession();
        }
    }

    Row {
        anchors.fill: parent

        // 视频渲染区
        Item {
            id: videoArea
            width: 576
            height: 1024

            // loading遮罩
            Rectangle {
                id: loadingMask
                anchors.fill: parent
                color: "#80000000" // 半透明黑
                visible: !videoRenderItem.hasFrame

                // loading动画
                BusyIndicator {
                    anchors.centerIn: parent
                    running: visible
                }
                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.top: parent.verticalCenter
                    anchors.topMargin: 40
                    text: "正在加载视频..."
                    color: "white"
                }
            }

            // 自定义渲染控件
            VideoRenderItem {
                id: videoRenderItem
                objectName: "videoRenderItem"
                Component.onCompleted: {
                    streamingViewModel.setVideoRenderItem(videoRenderItem)
                }
                anchors.fill: parent

                // 添加鼠标区域
                MouseArea {
                    anchors.fill: parent
                    hoverEnabled: true
                    // 0: press, 1: move, 2: release
                    function sendMouseEvent(mouse, eventType) {
                        var relativeX = mouse.x / videoRenderItem.width * videoRenderItem.width
                        var relativeY = mouse.y / videoRenderItem.height * videoRenderItem.height
                        var timestamp = Date.now()
                        streamingViewModel.handleMouseEvent(
                            relativeX, relativeY,
                            videoRenderItem.width, videoRenderItem.height,
                            eventType, timestamp
                        )
                    }

                    onPressed: {
                        sendMouseEvent(mouse, 0)
                    }
                    onPositionChanged: {
                        sendMouseEvent(mouse, 1)
                    }
                    onReleased: {
                        sendMouseEvent(mouse, 2)
                    }
                }
            }
        }

        // 按钮区
        ScrollView {
            width: parent.width - 80
            height: parent.height

            Column {
                spacing: 10
                width: parent.width

                Button { text: "主页(Home)"; onClicked: streamingViewModel.onHomeClicked() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "任务栏(Menu)"; onClicked: streamingViewModel.onMenuClicked() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "返回(Back)"; onClicked: streamingViewModel.onBackClicked() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "音量加"; onClicked: streamingViewModel.onVolumUp() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "音量减"; onClicked: streamingViewModel.onVolumDown() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "暂停推流"; onClicked: streamingViewModel.onPauseStreamClicked() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "恢复推流"; onClicked: streamingViewModel.onResumeStreamClicked() }
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Row {
                    spacing: 6
                    Label { text: "码率Min:" }
                    TextField {
                        id: minBitrateField
                        text: "1000"
                        validator: IntValidator { bottom: 1; top: 10000 }
                        width: 80
                    }
                }
                Row {
                    spacing: 6
                    Label { text: "码率Max:" }
                    TextField {
                        id: maxBitrateField
                        text: "3000"
                        validator: IntValidator { bottom: 1; top: 10000 }
                        width: 80
                    }
                }

                Row {
                    spacing: 6
                    Label { text: "帧率:" }
                    TextField {
                        id: framerateField
                        text: "30"
                        validator: IntValidator { bottom: 1; top: 60 }
                        width: 60
                    }
                }

                Button {
                    text: "改码率&帧率"
                    onClicked: {
                        streamingViewModel.onChangeBitrateClicked(
                            parseInt(framerateField.text),
                            parseInt(minBitrateField.text),
                            parseInt(maxBitrateField.text),
                        )
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Row {
                    spacing: 6
                    Button {
                        text: "粘贴"
                        onClicked: {
                            streamingViewModel.onClickPaste(inputField.text)
                        }
                    }

                    TextField {
                        id: inputField
                        text: "被粘贴的文本"
                        validator: IntValidator { bottom: 1; top: 60 }
                        width: 60
                    }
                }

                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                // ========== 显示当前的输入法 ==========
                Row {
                    spacing: 10
                    ButtonGroup { id: imeTypeGroup }
                    RadioButton {
                        text: "本地输入法"
                        checked: streamingViewModel.imeType === "local"
                        enabled: false
                        ButtonGroup.group: imeTypeGroup
                    }
                    RadioButton {
                        text: "云端输入法"
                        checked: streamingViewModel.imeType === "cloud"
                        enabled: false
                        ButtonGroup.group: imeTypeGroup
                    }
                }

                // ========== 切换输入法 ==========
                Row {
                    spacing: 10
                    Button {
                        text: "本地输入法"
                        onClicked: streamingViewModel.setLocalIME()
                    }
                    Button {
                        text: "云端输入法"
                        onClicked: streamingViewModel.setCloudIME()
                    }
                }
            }
        }
    }

    // ========== 输入弹窗 ==========
    Rectangle {
        id: inputPopup
        anchors.centerIn: parent
        width: 300
        height: 80
        color: "#ffffff"
        border.color: "#888"
        radius: 8
        visible: false
        z: 999

        TextField {
            id: popupInputField
            objectName: "popupInputField"
            anchors.centerIn: parent
            width: parent.width - 40
            focus: inputPopup.visible
            onAccepted: {
                streamingViewModel.onInputTextFromQml(text)
                text = ""
                Qt.callLater(function() { inputPopup.visible = false })
            }
        }
    }

    // 监听C++信号，弹出输入框
    Connections {
        target: streamingViewModel
        function onRequestShowInputBox() {
            if (!inputPopup.visible) {
                inputPopup.visible = true
                popupInputField.forceActiveFocus()
            }
        }
    }
}