import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import QtQuick.Dialogs 6.3
Window {
    id: loginWindow
    visible: true
    title: "登录窗口"
    width: 400
    height: 300

    // 登录表单
    ColumnLayout {
        anchors.centerIn: parent
        spacing: 20
        width: parent.width * 0.8

        // 账号输入框
        TextField {
            id: userIdField
            placeholderText: "请输入账号"
            text: ""
            Layout.fillWidth: true
            selectByMouse: true
        }

        // 密码输入框
        TextField {
            id: passwordField
            placeholderText: "请输入密码"
            text: ""
            Layout.fillWidth: true
            echoMode: TextInput.Password
            selectByMouse: true
        }

        // 登录按钮
        Button {
            id: loginButton
            text: "登录"
            Layout.fillWidth: true
            enabled: !loginViewModel.isBusy  // 禁用按钮当登录进行中
            onClicked: {
                // 调用LoginViewModel的登录方法
                loginViewModel.login(userIdField.text, passwordField.text)
            }
        }
    }

    // 错误提示对话框组件
    Dialog {
        id: errorDialog
        title: "登录失败"
        standardButtons: Dialog.Ok
        width: 300  // 显式设置宽度，避免依赖隐式宽度
        
        // 对话框内容
        Label {
            id: errorDialogLabel
            text: "登录失败"
            font.pixelSize: 16
            width: parent.width  // 显式设置宽度
            wrapMode: Text.Wrap  // 允许文本换行
        }
    }

    property var mainWindowRef: null  // 主窗口引用

    // 处理登录结果
    Connections {
        target: loginViewModel
        
        // 登录成功处理
        function onLoginSuccess(userType) {
            console.log("登录成功，用户类型:", userType)
            
            loginWindow.close();
            if (!mainWindowRef) {
                mainWindowRef = Qt.createComponent("MainWindow.qml").createObject(Qt.application);
                mainWindowRef.visible = true;
            }
        }
        
        // 登录失败处理
        function onAuthFailed(error) {
            console.error("登录失败:", error)
            // 设置错误信息并打开对话框
            errorDialogLabel.text = "登录失败: " + error
            errorDialog.open()
        }
    }
}
