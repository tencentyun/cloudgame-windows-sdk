import QtQuick 2.15
import QtQuick.Controls 2.15
import ".." as App

// 麦克风设备选择对话框
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
        microphoneDeviceListModel.clear()
        for (var i = 0; i < deviceList.length; i++) {
            microphoneDeviceListModel.append({deviceId: deviceList[i]})
        }
    }

    Rectangle {
        anchors.fill: parent
        color: App.Theme.cardBg
        border.color: App.Theme.border
        border.width: 1
        radius: 4

        Column {
            anchors.fill: parent
            anchors.margins: 10
            spacing: 10

            Rectangle {
                width: parent.width
                height: 40
                color: App.Theme.headerBg
                radius: 4

                Text {
                    anchors.centerIn: parent
                    text: "可用的麦克风设备"
                    font.pixelSize: 16
                    font.bold: true
                    color: App.Theme.textPrimary
                }
            }

            ListView {
                width: parent.width
                height: parent.height - 90
                clip: true

                model: ListModel {
                    id: microphoneDeviceListModel
                }

                delegate: Rectangle {
                    id: micDelegate
                    width: ListView.view.width
                    height: 40
                    color: index % 2 === 0 ? App.Theme.listEvenBg : App.Theme.listOddBg

                    property bool isHovered: false

                    Row {
                        anchors.fill: parent
                        anchors.margins: 8
                        spacing: 10

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: "设备 " + (index + 1) + ":"
                            font.bold: true
                            font.pixelSize: 13
                            color: App.Theme.textPrimary
                        }

                        Text {
                            anchors.verticalCenter: parent.verticalCenter
                            text: deviceId
                            elide: Text.ElideRight
                            width: parent.width - 80
                            font.pixelSize: 12
                            color: App.Theme.textPrimary
                        }
                    }

                    MouseArea {
                        anchors.fill: parent
                        hoverEnabled: true
                        onEntered: { micDelegate.isHovered = true; parent.color = App.Theme.listHoverBg }
                        onExited: { micDelegate.isHovered = false; parent.color = index % 2 === 0 ? App.Theme.listEvenBg : App.Theme.listOddBg }
                        onClicked: {
                            console.log("选中麦克风设备: " + deviceId)
                            root.deviceSelected(deviceId)
                            root.close()
                        }
                    }
                }

                Text {
                    anchors.centerIn: parent
                    text: "未检测到麦克风设备"
                    visible: microphoneDeviceListModel.count === 0
                    color: App.Theme.textHint
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
