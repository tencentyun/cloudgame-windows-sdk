import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Dialogs
import CustomComponents 1.0
import "components" as Components

ApplicationWindow {
    id: streamingWindow
    title: "串流窗口"
    width: 800
    height: 1024
    visible: true

    property StreamingViewModel streamingViewModel: StreamingViewModel {}
    
    // 横竖屏状态属性，true为横屏，false为竖屏
    property bool isLandscape: false
    
    // 新增：统计数据显示状态
    property bool showStatsOverlay: false
    
    // 使用 Connections 显式连接信号
    Connections {
        target: streamingViewModel
        
        function onScreenOrientationChanged(landscape) {
            console.log("收到屏幕方向变化信号: isLandscape=" + landscape)
            isLandscape = landscape
            if (isLandscape) {
                streamingWindow.width = 1024
                streamingWindow.height = 800
            } else {
                streamingWindow.width = 800
                streamingWindow.height = 1024
            }
        }
    }

    onClosing: {
        streamingViewModel.closeSession();
    }

    // 使用Loader动态加载不同的布局
    Loader {
        anchors.fill: parent
        sourceComponent: isLandscape ? landscapeLayout : portraitLayout
    }

    // 竖屏布局
    Component {
        id: portraitLayout
        
        Row {
            anchors.fill: parent

            // 视频渲染区 - 使用新的VideoRenderArea组件
            Components.VideoRenderArea {
                width: 576
                height: 1024
                streamingViewModel: streamingWindow.streamingViewModel
                isLandscape: false
                showStats: streamingWindow.showStatsOverlay
            }

            // 按钮区 - 使用统一的ControlButtonArea组件
            Components.ControlButtonArea {
                width: parent.width - 576
                height: parent.height
                streamingViewModel: streamingWindow.streamingViewModel
                isLandscape: false
                showStatsOverlay: streamingWindow.showStatsOverlay
                onShowStatsChanged: streamingWindow.showStatsOverlay = showStats
            }
        }
    }

    // 横屏布局
    Component {
        id: landscapeLayout
        
        Column {
            anchors.fill: parent

            // 视频渲染区 - 使用新的VideoRenderArea组件
            Components.VideoRenderArea {
                width: 1024
                height: 576
                streamingViewModel: streamingWindow.streamingViewModel
                isLandscape: true
                showStats: streamingWindow.showStatsOverlay
            }

            // 按钮区 - 使用统一的ControlButtonArea组件
            Components.ControlButtonArea {
                width: parent.width
                height: parent.height - 576
                streamingViewModel: streamingWindow.streamingViewModel
                isLandscape: true
                showStatsOverlay: streamingWindow.showStatsOverlay
                onShowStatsChanged: streamingWindow.showStatsOverlay = showStats
            }
        }
    }
    
    // 摄像头设备列表对话框
    Components.CameraDeviceDialog {
        id: cameraDeviceDialog
        x: (parent.width - width) / 2
        y: (parent.height - height) / 2
        
        onDeviceSelected: function(deviceId) {
            console.log("选中摄像头设备: " + deviceId)
            streamingViewModel.enableCameraWithDevice(deviceId)
        }
    }
}
