import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Dialogs 6.3

Dialog {
    id: streamSettingsDialog
    title: "串流参数设置"
    modal: true
    
    property var streamConfig: null
    
    width: 500
    height: 500
    
    contentItem: ColumnLayout {
        spacing: 20
        // 小流参数设置
        GroupBox {
            title: "小流串流参数"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: 10
                columnSpacing: 10
                
                Label {
                    text: "分辨率宽度:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: subWidthSpinBox
                    from: 144
                    to: 1920
                    stepSize: 144
                    value: streamConfig ? streamConfig.subStreamWidth : 288
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " px"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "帧率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: subFpsSpinBox
                    from: 1
                    to: 60
                    value: streamConfig ? streamConfig.subStreamFps : 1
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " fps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "最小码率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: subMinBitrateSpinBox
                    from: 10
                    to: 10000
                    stepSize: 10
                    value: streamConfig ? streamConfig.subStreamMinBitrate : 10
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " kbps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "最大码率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: subMaxBitrateSpinBox
                    from: 10
                    to: 10000
                    stepSize: 10
                    value: streamConfig ? streamConfig.subStreamMaxBitrate : 100
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " kbps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
            }
        }
        
        // 大流参数设置
        GroupBox {
            title: "大流串流参数"
            Layout.fillWidth: true
            
            GridLayout {
                anchors.fill: parent
                columns: 2
                rowSpacing: 10
                columnSpacing: 10
                
                Label {
                    text: "分辨率宽度:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: mainWidthSpinBox
                    from: 144
                    to: 1920
                    stepSize: 144
                    value: streamConfig ? streamConfig.mainStreamWidth : 288
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " px"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "帧率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: mainFpsSpinBox
                    from: 1
                    to: 60
                    value: streamConfig ? streamConfig.mainStreamFps : 1
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " fps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "最小码率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: mainMinBitrateSpinBox
                    from: 10
                    to: 10000
                    stepSize: 10
                    value: streamConfig ? streamConfig.mainStreamMinBitrate : 10
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " kbps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
                
                Label {
                    text: "最大码率:"
                    font.pixelSize: 14
                }
                
                SpinBox {
                    id: mainMaxBitrateSpinBox
                    from: 10
                    to: 10000
                    stepSize: 10
                    value: streamConfig ? streamConfig.mainStreamMaxBitrate : 100
                    editable: true
                    Layout.fillWidth: true
                    
                    textFromValue: function(value) {
                        return value + " kbps"
                    }
                    
                    valueFromText: function(text) {
                        return parseInt(text)
                    }
                }
            }
        }
    }
    
    footer: DialogButtonBox {
        Button {
            text: "确定"
            DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
            
            background: Rectangle {
                color: parent.hovered ? "#1565C0" : "#1976D2"
                radius: 4
            }
            
            contentItem: Text {
                text: parent.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
            }
        }
        
        Button {
            text: "取消"
            DialogButtonBox.buttonRole: DialogButtonBox.RejectRole
            
            background: Rectangle {
                color: parent.hovered ? "#E0E0E0" : "#F5F5F5"
                radius: 4
                border.color: "#BDBDBD"
                border.width: 1
            }
            
            contentItem: Text {
                text: parent.text
                color: "#424242"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font.pixelSize: 14
            }
        }
    }
    
    onAccepted: {
        if (streamConfig) {
            // 保存大流参数
            streamConfig.mainStreamWidth = mainWidthSpinBox.value
            streamConfig.mainStreamFps = mainFpsSpinBox.value
            streamConfig.mainStreamMinBitrate = mainMinBitrateSpinBox.value
            streamConfig.mainStreamMaxBitrate = mainMaxBitrateSpinBox.value
            
            // 保存小流参数
            streamConfig.subStreamWidth = subWidthSpinBox.value
            streamConfig.subStreamFps = subFpsSpinBox.value
            streamConfig.subStreamMinBitrate = subMinBitrateSpinBox.value
            streamConfig.subStreamMaxBitrate = subMaxBitrateSpinBox.value
            
            console.log("串流参数已保存")
        }
    }
    
    onOpened: {
        // 对话框打开时，从StreamConfig加载当前值
        if (streamConfig) {
            mainWidthSpinBox.value = streamConfig.mainStreamWidth
            mainFpsSpinBox.value = streamConfig.mainStreamFps
            mainMinBitrateSpinBox.value = streamConfig.mainStreamMinBitrate
            mainMaxBitrateSpinBox.value = streamConfig.mainStreamMaxBitrate
            
            subWidthSpinBox.value = streamConfig.subStreamWidth
            subFpsSpinBox.value = streamConfig.subStreamFps
            subMinBitrateSpinBox.value = streamConfig.subStreamMinBitrate
            subMaxBitrateSpinBox.value = streamConfig.subStreamMaxBitrate
        }
    }
}
