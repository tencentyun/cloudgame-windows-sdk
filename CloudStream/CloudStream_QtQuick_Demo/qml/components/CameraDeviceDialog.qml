import QtQuick 2.15
import QtQuick.Controls 2.15

// 摄像头设备选择对话框
Popup {
    id: root
    width: 400
    height: 350
    modal: true
    focus: true
    closePolicy: Popup.CloseOnEscape | Popup.CloseOnPressOutside
    
    // 对外暴露的属性和方法
    property var deviceList: []
    signal deviceSelected(string deviceId)
    
    // 当设备列表变化时更新内部model
    onDeviceListChanged: {
        cameraDeviceListModel.clear()
        for (var i = 0; i < deviceList.length; i++) {
            cameraDeviceListModel.append({deviceId: deviceList[i]})
        }
    }
    
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
                            root.deviceSelected(deviceId)
                            root.close()
                        }
                    }
                }
                
                Text {
                    anchors.centerIn: parent
                    text: "未检测到摄像头设备"
                    visible: cameraDeviceListModel.count === 0
                    color: "#999999"
                    font.pixelSize: 14
                }
                
                ScrollBar.vertical: ScrollBar {
                    policy: ScrollBar.AsNeeded
                }
            }
            
            Button {
                width: parent.width
                height: 35
                text: "关闭"
                onClicked: root.close()
            }
        }
    }
}
