import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 6.3
import CustomComponents 1.0

Window {
    id: instanceAccessWindow
    visible: true
    title: "实例访问配置"
    width: 500
    height: 400
    minimumWidth: 400
    minimumHeight: 600
    
    // 串流参数设置对话框
    StreamSettingsDialog {
        id: streamSettingsDialog
        streamConfig: StreamConfig
    }
    
    // 主布局
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.85
        
        // 标题和设置按钮行
        RowLayout {
            Layout.fillWidth: true
            spacing: 10
            
            Label {
                text: "获取实例访问令牌（演示用）"
                font.pixelSize: 24
                font.bold: true
                Layout.alignment: Qt.AlignLeft
                color: "#1976D2"
            }
            
            Item {
                Layout.fillWidth: true
            }
            
            Button {
                text: "⚙ 串流参数设置"
                font.pixelSize: 14
                Layout.alignment: Qt.AlignRight
                
                background: Rectangle {
                    color: parent.hovered ? "#E3F2FD" : "#F5F5F5"
                    radius: 5
                    border.color: "#1976D2"
                    border.width: 1
                }
                
                contentItem: Text {
                    text: parent.text
                    color: "#1976D2"
                    horizontalAlignment: Text.AlignHCenter
                    verticalAlignment: Text.AlignVCenter
                    font: parent.font
                }
                
                onClicked: {
                    streamSettingsDialog.open()
                }
            }
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
                    text: "cai-1300056159-fe2dhk6w65e,cai-1300056159-fe2dc3vyloo,cai-1300056159-fe2drzlzonn,cai-1300056159-fe2d8vzcram,cai-1300056159-fe2dvysc6xv,cai-1300056159-fe2darvvbm0,cai-1300056159-fe2dnbou172,cai-1300056159-fe2d6755rra,cai-1300056159-fe2dnbhiopm,cai-1300056159-fe2dzftqng9,cai-1300056159-fe2dqorri26,cai-1300056159-fe2dvfqlv0o,cai-1300056159-fe2d55g0zs0,cai-1300056159-fe2dmaxw2qd,cai-1300056159-fe2d35c8vy8,cai-1300056159-fe2ddy48o98,cai-1300056159-fe2d2477h6b,cai-1300056159-fe2doo24r0o,cai-1300056159-fe2dd436cea,cai-1300056159-fe2dtmsfxpd,cai-1300056159-fe2d7r3ikrc,cai-1300056159-fe2dex0nqob,cai-1300056159-fe2dmv8445x,cai-1300056159-fe2dvww9yq3,cai-1300056159-fe2d4zheref,cai-1300056159-fe2ddr3iqdi,cai-1300056159-fe2doe2coqd,cai-1300056159-fe2dsiqw1pq,cai-1300056159-fe2dqz6c21o,cai-1300056159-fe2d7rcs6ve,cai-1300056159-fe2d857whoo,cai-1300056159-fe2dzvalpij,cai-1300056159-fe2dawrt3j8,cai-1300056159-fe2dvncit4e,cai-1300056159-fe2d1hye73t,cai-1300056159-fe2dd6xmyyu,cai-1300056159-fe2de07sz1j,cai-1300056159-fe2d6n9xsi6,cai-1300056159-fe2dn2e976c,cai-1300056159-fe2dst0ae97,cai-1300056159-fe2ddejd97m,cai-1300056159-fe2dxfw4mzg,cai-1300056159-fe2dk20qw0x,cai-1300056159-fe2dw3njfig,cai-1300056159-fe2dkjzlhhy,cai-1300056159-fe2d66w6fqy,cai-1300056159-fe2do6asuch,cai-1300056159-fe2djfp0k8p,cai-1300056159-fe2dg5ez1tk,cai-1300056159-fe2daijil83,cai-1300056159-fe2d16pp9ko,cai-1300056159-fe2d6kypkp4,cai-1300056159-fe2do99x8gc,cai-1300056159-fe2dryoxgom,cai-1300056159-fe2dvzci435,cai-1300056159-fe2dp80hjgg,cai-1300056159-fe2d3atuigt,cai-1300056159-fe2dwrrshwb,cai-1300056159-fe2d0uh450h,cai-1300056159-fe2dpryyl8n,cai-1300056159-fe2d32sdzdv,cai-1300056159-fe2d5svv6rb,cai-1300056159-fe2djb7ra9m,cai-1300056159-fe2dcb2uao0,cai-1300056159-fe2dfescl4a,cai-1300056159-fe2dz2ktt43,cai-1300056159-fe2dl1gwrji,cai-1300056159-fe2da1oa7vu,cai-1300056159-fe2deydaw1b,cai-1300056159-fe2dip4uc2n,cai-1300056159-fe2dhk92ltr,cai-1300056159-fe2d1s75ruu,cai-1300056159-fe2dtqa2lrd,cai-1300056159-fe2d4pn0mrq,cai-1300056159-fe2disna3xj,cai-1300056159-fe2ddir3rzd,cai-1300056159-fe2dfn5zi2s,cai-1300056159-fe2dsh08u44,cai-1300056159-fe2deoe46ex,cai-1300056159-fe2dad4mz0x,cai-1300056159-fe2dfsakrhw,cai-1300056159-fe2d0tqzoaz,cai-1300056159-fe2d7knfrly,cai-1300056159-fe2dwzq3807,cai-1300056159-fe2dzrsdxzv,cai-1300056159-fe2dy3v6lc5,cai-1300056159-fe2dyyenfwu,cai-1300056159-fe2d98xfmc1,cai-1300056159-fe2d1gqm6cw,cai-1300056159-fe2d0mkq1k8,cai-1300056159-fe2d1fgff8p,cai-1300056159-fe2d85k2otj,cai-1300056159-fe2dmio64zz,cai-1300056159-fe2di1es4vx,cai-1300056159-fe2ddkg3qmu,cai-1300056159-fe2dqmwz1t9,cai-1300056159-fe2d0e0zc6a,cai-1300056159-fe2dit3w30j"

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
