import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 6.3

Window {
    id: instanceAccessWindow
    visible: true
    title: "实例访问配置"
    width: 500
    height: 400
    minimumWidth: 400
    minimumHeight: 350
    
    // 主布局
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.85
        
        // 标题
        Label {
            text: "获取实例访问令牌（演示用）"
            font.pixelSize: 24
            font.bold: true
            Layout.alignment: Qt.AlignHCenter
            color: "#1976D2"
        }
        
        // 实例ID输入区域
        GroupBox {
            title: "实例ID列表"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                TextArea {
                    id: instanceIdsField
                    placeholderText: "请输入实例ID，多个实例用逗号分隔\n例如：ACP250819OHEI9BD,ACP250819OHEI9B2,ACP250819OHEI9B3"
                    text: "cai-1300056159-fe2d0h0xjuf,cai-1300056159-fe2dd7xiyhz,cai-1300056159-fe2dmpyqy76"
                    Layout.fillWidth: true
                    Layout.preferredHeight: 100
                    wrapMode: Text.Wrap
                    font.pixelSize: 14
                }
                
                Label {
                    text: "提示：多个实例ID请用英文逗号分隔"
                    font.pixelSize: 12
                    color: "#666"
                }
            }
        }
        
        // IP地址输入区域
        GroupBox {
            title: "用户IP地址（可选）"
            Layout.fillWidth: true
            
            ColumnLayout {
                anchors.fill: parent
                spacing: 10
                
                TextField {
                    id: userIpField
                    placeholderText: "例如：192.168.1.100（可选）"
                    text: ""
                    Layout.fillWidth: true
                    font.pixelSize: 14
                    inputMethodHints: Qt.ImhDigitsOnly
                }
                
                Label {
                    text: "提示：留空将使用默认IP地址"
                    font.pixelSize: 12
                    color: "#666"
                }
            }
        }
        
        // 确认按钮
        Button {
            id: confirmButton
            text: "创建访问令牌"
            Layout.fillWidth: true
            Layout.preferredHeight: 40
            enabled: !instanceAccessViewModel.isBusy && instanceIdsField.text.trim().length > 0
            
            font.pixelSize: 16
            font.bold: true
            
            background: Rectangle {
                color: confirmButton.enabled ? "#1976D2" : "#BDBDBD"
                radius: 5
            }
            
            contentItem: Text {
                text: confirmButton.text
                color: "white"
                horizontalAlignment: Text.AlignHCenter
                verticalAlignment: Text.AlignVCenter
                font: confirmButton.font
            }
            
            onClicked: {
                instanceAccessViewModel.createAccessToken(
                    instanceIdsField.text.trim(), 
                    userIpField.text.trim()
                )
            }
        }
        
        // 加载指示器
        BusyIndicator {
            visible: instanceAccessViewModel.isBusy
            running: visible
            Layout.alignment: Qt.AlignHCenter
        }
    }
    
    // 错误提示对话框
    Dialog {
        id: errorDialog
        title: "创建失败"
        standardButtons: Dialog.Ok
        
        property string errorMessage: ""
        
        Label {
            text: errorDialog.errorMessage
            font.pixelSize: 16
            wrapMode: Text.Wrap
            width: 300
        }
    }
    
    property var mainWindowRef: null   // 主窗口引用

    // 处理ViewModel信号
    Connections {
        target: instanceAccessViewModel
        
        function onAccessTokenCreated(accessInfo, token) {
            if (!mainWindowRef) {
                var component = Qt.createComponent("MainWindow.qml");
                if (component.status === Component.Ready) {
                    mainWindowRef = component.createObject(null, {
                        "instanceIds": instanceIdsField.text.trim().split(","),
                        "accessInfo": accessInfo,
                        "token": token
                    });
                    if (mainWindowRef) {
                        mainWindowRef.visible = true;
                        instanceAccessWindow.hide();   // 隐藏
                    } else {
                        console.error("创建主窗口对象失败");
                    }
                } else {
                    console.error("创建主窗口失败:", component.errorString());
                }
            } else {
                // 已存在主窗口，直接显示
                mainWindowRef.instanceIds = instanceIdsField.text.trim().split(",");
                mainWindowRef.accessInfo   = accessInfo;
                mainWindowRef.token        = token;
                mainWindowRef.visible      = true;
                instanceAccessWindow.hide();
            }
        }
        
        function onErrorOccurred(errorMessage) {
            console.error("创建失败:", errorMessage)
            errorDialog.errorMessage = errorMessage
            errorDialog.open()
        }
    }
}
