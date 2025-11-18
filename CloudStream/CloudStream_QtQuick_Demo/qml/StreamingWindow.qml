import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
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
            
            // 统计数据蒙层
            Rectangle {
                id: statsOverlay
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: statsText.contentHeight + 20
                color: "#80000000" // 半透明黑
                visible: streamingViewModel.clientStats !== ""
                
                Text {
                    id: statsText
                    anchors.fill: parent
                    anchors.margins: 10
                    color: "white"
                    font.pixelSize: 12
                    wrapMode: Text.WordWrap
                    text: {
                        if (streamingViewModel.clientStats === "") return ""
                        
                        try {
                            var stats = JSON.parse(streamingViewModel.clientStats)
                            var bitrateKbps = ((stats.bitrate || 0) / 1000).toFixed(2)
                            return "FPS: " + (stats.fps || 0) + 
                                   " | 码率: " + bitrateKbps + " kbps" +
                                   " | RTT: " + (stats.rtt || 0) + "ms" +
                                   "\n分辨率: " + (stats.frame_recv_res || "N/A") +
                                   " | NACK: " + (stats.nack_count || 0) +
                                   "\n视频包 - 接收: " + (stats.video_packet_recv || 0) + 
                                   " | 发送: " + (stats.video_packet_sent || 0) + 
                                   " | 丢包: " + (stats.video_packet_lost || 0) +
                                   "\n音频包 - 接收: " + (stats.audio_packet_recv || 0) + 
                                   " | 发送: " + (stats.audio_packet_sent || 0) +
                                   " | 丢包: " + (stats.audio_packet_lost || 0) +
                                   "\n视频帧 - 接收: " + (stats.frame_recv || 0) + 
                                   " | 解码: " + (stats.frame_decode || 0) +
                                   " | 编码: " + (stats.frame_encoded || 0) + 
                                   " | 发送: " + (stats.frame_sent || 0) + 
                                   " | 丢帧: " + (stats.frame_drop || 0) +
                                   "\nRequest ID: " + (stats.request_id || "N/A") +
                                   "\nInstance IDs: " + (stats.instance_ids || "N/A") +
                                   "\nCodec: " + (stats.codec || "N/A")
                        } catch (e) {
                            return "统计数据解析错误"
                        }
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
                Button { text: "任务栏(Menu)"; onClicked: streamingViewModel.onMenuClicked() }
                Button { text: "返回(Back)"; onClicked: streamingViewModel.onBackClicked() }

                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "音量加"; onClicked: streamingViewModel.onVolumUp() }
                Button { text: "音量减"; onClicked: streamingViewModel.onVolumDown() }

                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "暂停视频推流"; onClicked: streamingViewModel.onPauseVideoStreamClicked() }
                Button { text: "恢复视频推流"; onClicked: streamingViewModel.onResumeVideoStreamClicked() }

                Button { text: "暂停音频推流"; onClicked: streamingViewModel.onPauseAudioStreamClicked() }
                Button { text: "恢复音频推流"; onClicked: streamingViewModel.onResumeAudioStreamClicked() }

                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "开启摄像头"; onClicked: streamingViewModel.onEnableCameraClicked() }
                Button { text: "关闭摄像头"; onClicked: streamingViewModel.onDisableCameraClicked() }
                
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "开启麦克风"; onClicked: streamingViewModel.onEnableMicrophoneClicked() }
                Button { text: "关闭麦克风"; onClicked: streamingViewModel.onDisableMicrophoneClicked() }
                
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }

                Button { text: "设置视频流参数"; onClicked: streamingViewModel.onVideoStreamSettingsClicked() }
                
                Rectangle { width: parent.width; height: 1; color: "#cccccc" }
                
                Button { 
                    text: "查看摄像头设备列表"
                    onClicked: {
                        var devices = streamingViewModel.getCameraDeviceList()
                        cameraDeviceListModel.clear()
                        for (var i = 0; i < devices.length; i++) {
                            cameraDeviceListModel.append({deviceId: devices[i]})
                        }
                        cameraDeviceDialog.open()
                    }
                }

            }
        }
    }
    
    // 摄像头设备列表对话框
    Popup {
        id: cameraDeviceDialog
        width: 400
        height: 350
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        modal: true
        focus: true
        closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
        
        // 对话框背景和内容
        Rectangle {
            anchors.fill: parent
            color: "white"
            border.color: "#cccccc"
            border.width: 1
            radius: 4
            
            Column {
                anchors.fill: parent
                anchors.margins: 10
                spacing: 10
                
                // 标题栏
                Rectangle {
                    width: parent.width
                    height: 40
                    color: "#f5f5f5"
                    radius: 4
                    
                    Text {
                        anchors.centerIn: parent
                        text: "可用的摄像头设备"
                        font.pixelSize: 16
                        font.bold: true
                    }
                }
                
                // 设备列表
                ListView {
                    width: parent.width
                    height: parent.height - 90
                    clip: true
                    
                    model: ListModel {
                        id: cameraDeviceListModel
                    }
                    
                    delegate: Rectangle {
                        width: ListView.view.width
                        height: 40
                        color: index % 2 === 0 ? "#f9f9f9" : "white"
                        
                        Row {
                            anchors.fill: parent
                            anchors.margins: 8
                            spacing: 10
                            
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: "设备 " + (index + 1) + ":"
                                font.bold: true
                                font.pixelSize: 13
                            }
                            
                            Text {
                                anchors.verticalCenter: parent.verticalCenter
                                text: deviceId
                                elide: Text.ElideRight
                                width: parent.width - 80
                                font.pixelSize: 12
                            }
                        }
                        
                        MouseArea {
                            anchors.fill: parent
                            hoverEnabled: true
                            onEntered: parent.color = "#e8f4fd"
                            onExited: parent.color = index % 2 === 0 ? "#f9f9f9" : "white"
                            onClicked: {
                                console.log("选中摄像头设备: " + deviceId)
                                // 调用ViewModel方法启用选中的摄像头设备
                                streamingViewModel.enableCameraWithDevice(deviceId)
                                // 关闭对话框
                                cameraDeviceDialog.close()
                            }
                        }
                    }
                    
                    // 空列表提示
                    Text {
                        anchors.centerIn: parent
                        text: "未检测到摄像头设备"
                        visible: cameraDeviceListModel.count === 0
                        color: "#999999"
                        font.pixelSize: 14
                    }
                    
                    // 滚动条
                    ScrollBar.vertical: ScrollBar {
                        policy: ScrollBar.AsNeeded
                    }
                }
                
                // 关闭按钮
                Button {
                    width: parent.width
                    height: 35
                    text: "关闭"
                    onClicked: cameraDeviceDialog.close()
                }
            }
        }
    }
}
