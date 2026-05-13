import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 6.3
import CustomComponents 1.0
import "." as App

ApplicationWindow {
    id: instanceAccessWindow
    visible: true
    title: "实例访问配置"
    width: 520
    height: 620
    minimumWidth: 440
    minimumHeight: 500
    color: App.Theme.windowBg

    // 适配深色模式: palette 会自动传播给所有子 Controls (Button, Label, ComboBox, GroupBox, TextField 等)
    palette {
        window: App.Theme.windowBg
        windowText: App.Theme.textPrimary
        base: App.Theme.paletteBase
        alternateBase: App.Theme.paletteAlternateBase
        button: App.Theme.paletteButton
        buttonText: App.Theme.paletteButtonText
        mid: App.Theme.paletteMid
        light: App.Theme.paletteLight
        midlight: App.Theme.paletteMidlight
        dark: App.Theme.paletteDark
        text: App.Theme.textPrimary
        highlight: App.Theme.paletteHighlight
        highlightedText: App.Theme.paletteHighlightedText
        toolTipBase: App.Theme.paletteToolTipBase
        toolTipText: App.Theme.paletteToolTipText
        placeholderText: App.Theme.textHint
    }

    // 串流参数设置对话框
    StreamSettingsDialog {
        id: streamSettingsDialog
        streamConfig: StreamConfig
    }

    // 可滚动区域，防止窗口缩小时内容被遮挡
    ScrollView {
        anchors.fill: parent
        anchors.margins: 20
        contentWidth: availableWidth
        clip: true

        // 主布局
        ColumnLayout {
            width: parent.width
            spacing: 16

            // 标题和设置按钮行
            RowLayout {
                Layout.fillWidth: true
                spacing: 10

                Label {
                    text: "获取实例访问令牌（演示用）"
                    font.pixelSize: 22
                    font.bold: true
                    Layout.alignment: Qt.AlignLeft
                    color: "#1976D2"
                }

                Item {
                    Layout.fillWidth: true
                }

                Button {
                    text: "⚙ 串流参数设置"
                    font.pixelSize: 13
                    Layout.alignment: Qt.AlignRight

                    background: Rectangle {
                        color: parent.hovered ? App.Theme.btnHoverBg : App.Theme.btnBg
                        radius: 5
                        border.color: App.Theme.primary
                        border.width: 1
                    }

                    contentItem: Text {
                        text: parent.text
                        color: App.Theme.primary
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                        font: parent.font
                    }

                    onClicked: {
                        streamSettingsDialog.open()
                    }
                }
            }

            // 配置文件选择区域
            GroupBox {
                title: "选择配置文件"
                Layout.fillWidth: true

                RowLayout {
                    anchors.fill: parent
                    spacing: 10

                    Label {
                        text: "配置："
                        font.pixelSize: 14
                        Layout.alignment: Qt.AlignVCenter
                    }

                    ComboBox {
                        id: configSelector
                        Layout.fillWidth: true
                        font.pixelSize: 14
                        model: AppConfig.configNames

                        onActivated: {
                            AppConfig.switchConfig(currentText)
                        }

                        // 初始化时加载第一个配置文件的数据
                        // onActivated 只在用户交互时触发，启动时需手动触发一次
                        Component.onCompleted: {
                            if (model && model.length > 0) {
                                AppConfig.switchConfig(currentText)
                            }
                        }
                    }
                }
            }

            // 实例ID输入区域
            GroupBox {
                title: "实例ID列表"
                Layout.fillWidth: true

                ColumnLayout {
                    anchors.fill: parent
                    spacing: 6

                    TextArea {
                        id: instanceIdsField
                        placeholderText: "请输入实例ID，多个实例用逗号分隔\n例如：ACP250819OHEI9BD,ACP250819OHEI9B2"
                        text: AppConfig.instanceIds

                        Layout.fillWidth: true
                        Layout.preferredHeight: 120
                        wrapMode: Text.Wrap
                        font.pixelSize: 13

                        background: Rectangle {
                            color: App.Theme.inputBg
                            border.color: instanceIdsField.activeFocus ? App.Theme.primary : App.Theme.border
                            border.width: 1
                            radius: 3
                        }

                        // 切换配置后强制刷新文本（TextArea 用户编辑会断开属性绑定）
                        Connections {
                            target: AppConfig
                            function onInstanceIdsChanged() {
                                instanceIdsField.text = AppConfig.instanceIds
                            }
                        }
                    }

                    Label {
                        text: "多个实例ID用英文逗号分隔"
                        font.pixelSize: 12
                        color: App.Theme.textHint
                    }
                }
            }

            // IP地址输入区域
            GroupBox {
                title: "用户IP地址（可选）"
                Layout.fillWidth: true

                TextField {
                    id: userIpField
                    anchors.fill: parent
                    placeholderText: "例如：192.168.1.100（留空使用默认）"
                    text: ""
                    font.pixelSize: 14
                    inputMethodHints: Qt.ImhDigitsOnly
                }
            }

            // 确认按钮
            Button {
                id: confirmButton
                text: "创建访问令牌"
                Layout.fillWidth: true
                Layout.preferredHeight: 44
                enabled: !instanceAccessViewModel.isBusy && instanceIdsField.text.trim().length > 0

                font.pixelSize: 16
                font.bold: true

                background: Rectangle {
                    color: confirmButton.enabled ? App.Theme.primary : App.Theme.btnDisabledBg
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
        } // ColumnLayout
    } // ScrollView

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
                        instanceAccessWindow.hide();
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
