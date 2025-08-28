import QtQuick 2.15
import QtQuick.Controls 2.15
import CustomComponents 1.0

ApplicationWindow {
    id: streamingWindow
    title: "串流窗口"
    width: 800
    height: 1024
    visible: true

    property StreamingViewModel streamingViewModel: StreamingViewModel {}

    onClosing: {
        streamingViewModel.closeSession();
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
            }
        }
    }
}