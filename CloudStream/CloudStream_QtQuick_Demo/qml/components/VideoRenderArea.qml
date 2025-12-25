import QtQuick 2.15
import QtQuick.Controls 2.15
import CustomComponents 1.0
import "../components" as Components

// 视频渲染区域组件
Item {
    id: root
    
    // 对外暴露的属性
    property var streamingViewModel: null
    property bool isLandscape: false
    property bool showStats: false 

    // 内部使用的objectName，用于区分横竖屏
    readonly property string videoObjectName: isLandscape ? "videoRenderItem_landscape" : "videoRenderItem_portrait"
    
    // loading遮罩
    Rectangle {
        id: loadingMask
        anchors.fill: parent
        color: "#80000000"
        visible: !videoRenderItem.hasFrame
        
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
    VideoRenderPaintedItem {
        id: videoRenderItem
        objectName: root.videoObjectName
        anchors.fill: parent
        
        // 横屏时添加黑色背景
        Rectangle {
            anchors.fill: parent
            color: "black"
            z: -1
            visible: root.isLandscape
        }
        
        Component.onCompleted: {
            console.log("VideoRenderArea创建完成: " + objectName)
            if (root.streamingViewModel) {
                root.streamingViewModel.setVideoRenderItem(videoRenderItem)
                // 在 C++ 层建立 VideoRenderPaintedItem 到 StreamingViewModel 的直接连接
                videoRenderItem.setStreamingViewModel(root.streamingViewModel)
            }
        }
        
        Component.onDestruction: {
            console.log("VideoRenderArea销毁: " + objectName)
        }
        
        // 触摸事件处理
        MouseArea {
            anchors.fill: parent
            hoverEnabled: true
            
            function sendMouseEvent(mouse, eventType) {
                var relativeX = mouse.x
                var relativeY = mouse.y
                var timestamp = Date.now()
                videoRenderItem.handleTouchEvent(
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
    Components.StatsOverlay {
        id: statsOverlay
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        clientStats: ""
        visible: root.showStats  // 根据showStats属性控制显示
        
        // 监听统计数据更新
        Connections {
            target: root.streamingViewModel
            function onClientStatsChanged() {
                if (!root.streamingViewModel) {
                    return;
                }
                
                // 获取当前实例的统计数据
                var newStats = root.streamingViewModel.getInstanceStats();
                if (newStats && newStats.length > 0) {
                    statsOverlay.clientStats = newStats;
                }
            }
        }
        
        // 初始化时获取一次数据
        Component.onCompleted: {
            if (root.streamingViewModel) {
                var initialStats = root.streamingViewModel.getInstanceStats();
                if (initialStats && initialStats.length > 0) {
                    statsOverlay.clientStats = initialStats;
                }
            }
        }
    }
}
