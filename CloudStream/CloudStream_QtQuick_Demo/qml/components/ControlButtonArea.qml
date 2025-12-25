import QtQuick 2.15
import QtQuick.Controls 2.15

// 控制按钮区域组件
Item {
    id: root
    
    // 对外暴露的属性
    property var streamingViewModel: null
    property bool isLandscape: false
    property bool showStatsOverlay: false
    
    // 统计数据显示状态变化信号
    signal showStatsChanged(bool showStats)
    
    // 按钮数据模型
    ListModel {
        id: buttonModel
        
        ListElement { text: "主页(Home)"; action: "home"; group: 0 }
        ListElement { text: "任务栏(Menu)"; action: "menu"; group: 0 }
        ListElement { text: "返回(Back)"; action: "back"; group: 0 }
        
        ListElement { text: "音量加"; action: "volumeUp"; group: 1 }
        ListElement { text: "音量减"; action: "volumeDown"; group: 1 }
        
        ListElement { text: "暂停视频推流"; action: "pauseVideo"; group: 2 }
        ListElement { text: "恢复视频推流"; action: "resumeVideo"; group: 2 }
        
        ListElement { text: "暂停音频推流"; action: "pauseAudio"; group: 3 }
        ListElement { text: "恢复音频推流"; action: "resumeAudio"; group: 3 }
        
        ListElement { text: "开启摄像头"; action: "enableCamera"; group: 4 }
        ListElement { text: "关闭摄像头"; action: "disableCamera"; group: 4 }
        
        ListElement { text: "开启麦克风"; action: "enableMicrophone"; group: 5 }
        ListElement { text: "关闭麦克风"; action: "disableMicrophone"; group: 5 }
        
        ListElement { text: "设置视频流参数"; action: "videoSettings"; group: 6 }
        
        ListElement { text: "查看摄像头设备列表"; action: "cameraDeviceList"; group: 7 }
    }
    
    // 处理按钮点击事件
    function handleButtonClick(action) {
        if (!streamingViewModel) {
            console.warn("streamingViewModel is null")
            return
        }
        
        switch(action) {
            case "home":
                streamingViewModel.onHomeClicked()
                break
            case "menu":
                streamingViewModel.onMenuClicked()
                break
            case "back":
                streamingViewModel.onBackClicked()
                break
            case "volumeUp":
                streamingViewModel.onVolumUp()
                break
            case "volumeDown":
                streamingViewModel.onVolumDown()
                break
            case "pauseVideo":
                streamingViewModel.onPauseVideoStreamClicked()
                break
            case "resumeVideo":
                streamingViewModel.onResumeVideoStreamClicked()
                break
            case "pauseAudio":
                streamingViewModel.onPauseAudioStreamClicked()
                break
            case "resumeAudio":
                streamingViewModel.onResumeAudioStreamClicked()
                break
            case "enableCamera":
                streamingViewModel.onEnableCameraClicked()
                break
            case "disableCamera":
                streamingViewModel.onDisableCameraClicked()
                break
            case "enableMicrophone":
                streamingViewModel.onEnableMicrophoneClicked()
                break
            case "disableMicrophone":
                streamingViewModel.onDisableMicrophoneClicked()
                break
            case "videoSettings":
                streamingViewModel.onVideoStreamSettingsClicked()
                break
            case "cameraDeviceList":
                var devices = streamingViewModel.getCameraDeviceList()
                root.parent.parent.parent.cameraDeviceDialog.deviceList = devices
                root.parent.parent.parent.cameraDeviceDialog.open()
                break
        }
    }
    
    // 统计数据显示控制复选框
    CheckBox {
        id: statsCheckBox
        text: "显示统计数据"
        checked: root.showStatsOverlay
        
        anchors {
            top: parent.top
            left: parent.left
            right: parent.right
            margins: 10
        }
        
        height: 40
        
        onCheckedChanged: {
            root.showStatsOverlay = checked
            root.showStatsChanged(checked)
        }
    }
    
    ScrollView {
        anchors {
            top: statsCheckBox.bottom
            left: parent.left
            right: parent.right
            bottom: parent.bottom
            topMargin: 5
        }
        
        // 根据横竖屏使用不同的布局
        Loader {
            width: parent.width
            sourceComponent: isLandscape ? flowLayout : columnLayout
        }
    }
    
    // 竖屏布局：垂直排列
    Component {
        id: columnLayout
        
        Column {
            spacing: 10
            width: parent.width
            
            Repeater {
                model: buttonModel
                
                Column {
                    width: parent.width
                    spacing: 10
                    
                    // 在不同组之间添加分隔线
                    Rectangle {
                        width: parent.width
                        height: 1
                        color: "#cccccc"
                        visible: index > 0 && model.group !== buttonModel.get(index - 1).group
                    }
                    
                    Button {
                        width: parent.width
                        text: model.text
                        onClicked: root.handleButtonClick(model.action)
                    }
                }
            }
        }
    }
    
    // 横屏布局：流式排列
    Component {
        id: flowLayout
        
        Flow {
            spacing: 10
            width: parent.width
            padding: 10
            
            Repeater {
                model: buttonModel
                
                Button {
                    text: model.text
                    onClicked: root.handleButtonClick(model.action)
                }
            }
        }
    }
}
