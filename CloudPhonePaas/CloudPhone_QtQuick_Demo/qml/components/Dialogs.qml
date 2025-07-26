import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: dialogsRoot

    property int dialogWidth: 600
    property int dialogHeight: 400

    // 通用提示框
    Dialog {
        id: genericTipDialog
        property string tipTitle: ""
        property string tipMessage: ""
        property var buttons: Dialog.Ok
        
        title: tipTitle
        standardButtons: buttons
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        anchors.centerIn: parent
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    text: genericTipDialog.tipMessage
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                    background: null
                }
            }
        }
    }
    property alias genericTipDialog: genericTipDialog

    // 批量操作弹窗alias
    property alias gpsDialog: gpsDialog
    property alias resolutionDialog: resolutionDialog
    property alias pasteDialog: pasteDialog
    property alias sendClipboardDialog: sendClipboardDialog
    property alias modifySensorDialog: modifySensorDialog
    property alias shakeDialog: shakeDialog
    property alias blowDialog: blowDialog
    property alias messageDialog: messageDialog
    property alias modifyInstancePropertiesDialog: modifyInstancePropertiesDialog
    property alias modifyKeepFrontAppStatusDialog: modifyKeepFrontAppStatusDialog
    property alias uninstallAppDialog: uninstallAppDialog
    property alias startAppDialog: startAppDialog
    property alias stopAppDialog: stopAppDialog
    property alias clearAppDataDialog: clearAppDataDialog
    property alias enableAppDialog: enableAppDialog
    property alias disableAppDialog: disableAppDialog
    property alias startCameraMediaPlayDialog: startCameraMediaPlayDialog
    property alias displayCameraImageDialog: displayCameraImageDialog
    property alias addKeepAliveListDialog: addKeepAliveListDialog
    property alias removeKeepAliveListDialog: removeKeepAliveListDialog
    property alias setKeepAliveListDialog: setKeepAliveListDialog
    property alias describeInstancePropertiesDialog: describeInstancePropertiesDialog
    property alias describeKeepFrontAppStatusDialog: describeKeepFrontAppStatusDialog
    property alias stopCameraMediaPlayDialog: stopCameraMediaPlayDialog
    property alias describeCameraMediaPlayStatusDialog: describeCameraMediaPlayStatusDialog
    property alias describeKeepAliveListDialog: describeKeepAliveListDialog
    property alias clearKeepAliveListDialog: clearKeepAliveListDialog
    property alias listUserAppsDialog: listUserAppsDialog
    property alias muteDialog: muteDialog
    property alias mediaSearchDialog: mediaSearchDialog
    property alias rebootDialog: rebootDialog
    property alias listAllAppsDialog: listAllAppsDialog
    property alias moveAppBackgroundDialog: moveAppBackgroundDialog
    property alias addAppInstallBlackListDialog: addAppInstallBlackListDialog
    property alias removeAppInstallBlackListDialog: removeAppInstallBlackListDialog
    property alias setAppInstallBlackListDialog: setAppInstallBlackListDialog
    property alias describeAppInstallBlackListDialog: describeAppInstallBlackListDialog
    property alias clearAppInstallBlackListDialog: clearAppInstallBlackListDialog

    // 信号定义
    signal gpsAccepted(string longitude, string latitude)
    signal resolutionAccepted(int width, int height, int dpi)
    signal pasteAccepted(string text)
    signal sendClipboardAccepted(string text)
    signal modifySensorAccepted(string sensorType, var values, int accuracy)
    signal shakeAccepted()
    signal blowAccepted()
    signal messageAccepted(string packageName, string msg)
    signal modifyInstancePropertiesAccepted(string json)
    signal modifyKeepFrontAppStatusAccepted(string packageName, bool enable, int restartInterval)
    signal uninstallAppAccepted(string packageName)
    signal startAppAccepted(string packageName, string activityName)
    signal stopAppAccepted(string packageName)
    signal clearAppDataAccepted(string packageName)
    signal enableAppAccepted(string packageName)
    signal disableAppAccepted(string packageName)
    signal startCameraMediaPlayAccepted(string filePath, int loops)
    signal displayCameraImageAccepted(string filePath)
    signal addKeepAliveListAccepted(string appList)
    signal removeKeepAliveListAccepted(string appList)
    signal setKeepAliveListAccepted(string appList)
    signal describeInstancePropertiesAccepted()
    signal describeKeepFrontAppStatusAccepted()
    signal stopCameraMediaPlayAccepted()
    signal describeCameraMediaPlayStatusAccepted()
    signal describeKeepAliveListAccepted()
    signal clearKeepAliveListAccepted()
    signal listUserAppsAccepted()
    signal muteAccepted(bool mute)
    signal mediaSearchAccepted(string keyword)
    signal rebootAccepted()
    signal listAllAppsAccepted()
    signal moveAppBackgroundAccepted()
    signal addAppInstallBlackListAccepted(string appList)
    signal removeAppInstallBlackListAccepted(string appList)
    signal setAppInstallBlackListAccepted(string appList)
    signal describeAppInstallBlackListAccepted()
    signal clearAppInstallBlackListAccepted()

    // 1. GPS
    Dialog {
        id: gpsDialog
        title: "设置GPS"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "Longitude"; Layout.preferredWidth: 80 }
                TextField { 
                    id: longitudeField; 
                    placeholderText: "Longitude"; 
                    text: "121.4737"  // 上海经度默认值
                }
            }
            RowLayout {
                Label { text: "Latitude"; Layout.preferredWidth: 80 }
                TextField { 
                    id: latitudeField; 
                    placeholderText: "Latitude"; 
                    text: "31.2304"   // 上海纬度默认值
                }
            }
        }
        onAccepted: dialogsRoot.gpsAccepted(longitudeField.text, latitudeField.text)
    }

    // 2. 分辨率
    Dialog {
        id: resolutionDialog
        title: "设置分辨率"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "Width"; Layout.preferredWidth: 80 }
                TextField { 
                    id: widthField; 
                    placeholderText: "Width"; 
                    text: "720"; 
                    validator: IntValidator { bottom: 1 } 
                }
            }
            RowLayout {
                Label { text: "Height"; Layout.preferredWidth: 80 }
                TextField { 
                    id: heightField; 
                    placeholderText: "Height"; 
                    text: "1280"; 
                    validator: IntValidator { bottom: 1 } 
                }
            }
            RowLayout {
                Label { text: "DPI"; Layout.preferredWidth: 80 }
                TextField { 
                    id: dpiField; 
                    placeholderText: "DPI"; 
                    text: "1"; 
                    validator: IntValidator { bottom: 1 } 
                }
            }
        }
        onAccepted: dialogsRoot.resolutionAccepted(parseInt(widthField.text), parseInt(heightField.text), parseInt(dpiField.text))
    }

    // 3. 粘贴文本
    Dialog {
        id: pasteDialog
        title: "粘贴文本"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "请输入文本"; Layout.preferredWidth: 80 }
                TextField { 
                    id: textField; 
                    placeholderText: "请输入文本";
                    text: "示例文本" 
                }
            }
        }
        onAccepted: dialogsRoot.pasteAccepted(textField.text)
    }

    // 4. 发送剪贴板
    Dialog {
        id: sendClipboardDialog
        title: "发送剪贴板"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "请输入剪贴板内容" }
            TextField { 
                id: clipboardTextField; 
                placeholderText: "请输入剪贴板内容";
                text: "示例内容" 
            }
        }
        onAccepted: dialogsRoot.sendClipboardAccepted(clipboardTextField.text)
    }

    Dialog {
        id: modifySensorDialog
        title: "修改传感器数据"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight + 100
        
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            
            // 传感器类型选择
            GroupBox {
                title: "传感器类型"
                Layout.fillWidth: true
                
                ComboBox {
                    id: sensorTypeCombo
                    model: ["accelerometer", "gyroscope", "significant_motion", 
                            "step_counter", "step_detector", "pressure", "proximity"]
                    width: parent.width
                }
            }
            
            // 传感器数值输入
            GroupBox {
                title: "传感器数值"
                Layout.fillWidth: true
                
                GridLayout {
                    columns: 3
                    width: parent.width
                    
                    Label { text: "X:" }
                    TextField { 
                        id: sensorValueX
                        text: "0.2"
                        placeholderText: "X值"
                        validator: DoubleValidator { decimals: 6 }
                    }
                    Item { Layout.fillWidth: true }
                    
                    Label { text: "Y:" }
                    TextField { 
                        id: sensorValueY
                        text: "0.3"
                        placeholderText: "Y值"
                        validator: DoubleValidator { decimals: 6 }
                    }
                    Item { Layout.fillWidth: true }
                    
                    Label { text: "Z:" }
                    TextField { 
                        id: sensorValueZ
                        text: "0.4"
                        placeholderText: "Z值"
                        validator: DoubleValidator { decimals: 6 }
                    }
                    Item { Layout.fillWidth: true }
                }
            }
            
            // 精度设置
            GroupBox {
                title: "精度设置"
                Layout.fillWidth: true
                
                TextField { 
                    id: sensorAccuracy
                    placeholderText: "精度 (目前仅支持3)"
                    text: "3"
                    validator: IntValidator { bottom: 3; top: 3 }
                    enabled: false // 目前仅支持3，所以禁用
                }
            }
        }
        
        onAccepted: {
            var values = [];
            if (sensorValueX.text !== "") values.push(parseFloat(sensorValueX.text));
            if (sensorValueY.text !== "") values.push(parseFloat(sensorValueY.text));
            if (sensorValueZ.text !== "") values.push(parseFloat(sensorValueZ.text));
            
            dialogsRoot.modifySensorAccepted(
                sensorTypeCombo.currentText,
                values,
                parseInt(sensorAccuracy.text)
            );
        }
    } 

    // 5. 摇一摇
    Dialog {
        id: shakeDialog
        title: "摇一摇"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认要摇一摇吗？" }
        }
        onAccepted: dialogsRoot.shakeAccepted()
    }

    // 6. 吹气
    Dialog {
        id: blowDialog
        title: "吹气"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认要吹气吗？" }
        }
        onAccepted: dialogsRoot.blowAccepted()
    }

    // 7. 透传消息
    Dialog {
        id: messageDialog
        title: "透传消息"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "PackageName"; Layout.preferredWidth: 80 }
                TextField { 
                    id: packageNameField; 
                    placeholderText: "PackageName";
                    text: "com.example.app"
                }
            }
            RowLayout {
                Label { text: "Msg"; Layout.preferredWidth: 80 }
                TextField { 
                    id: msgField; 
                    placeholderText: "Msg";
                    text: "Hello from CloudPhone"
                }
            }
        }
        onAccepted: dialogsRoot.messageAccepted(packageNameField.text, msgField.text)
    }

    // 8. 修改实例属性
    Dialog {
        id: modifyInstancePropertiesDialog
        title: "修改实例属性"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth + 100
        height: dialogsRoot.dialogHeight + 400
        x: (dialogsRoot.width - width) / 2
        y: (dialogsRoot.height - height) / 2
        ScrollView {
            anchors.fill: parent
            anchors.margins: 20
            ColumnLayout {
                width: parent.width
                spacing: 10
                
                // 设备信息
                GroupBox {
                    title: "设备信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "品牌(Brand):" }
                        TextField { 
                            id: deviceBrand
                            placeholderText: "例如: Samsung"
                            text: "Samsung"
                        }
                        
                        Label { text: "型号(Model):" }
                        TextField { 
                            id: deviceModel
                            placeholderText: "例如: Galaxy S23"
                            text: "Galaxy S23"
                        }
                    }
                }
                
                // 代理信息
                GroupBox {
                    title: "代理信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "启用代理:" }
                        CheckBox { 
                            id: proxyEnabled
                            checked: true
                        }
                        
                        Label { text: "协议(Protocol):" }
                        TextField { 
                            id: proxyProtocol
                            placeholderText: "例如: socks5"
                            text: "socks5"
                        }
                        
                        Label { text: "主机(Host):" }
                        TextField { 
                            id: proxyHost
                            placeholderText: "例如: proxy.example.com"
                            text: "proxy.example.com"
                        }
                        
                        Label { text: "端口(Port):" }
                        TextField { 
                            id: proxyPort
                            placeholderText: "例如: 1080"
                            text: "1080"
                            validator: IntValidator { bottom: 1 }
                        }
                        
                        Label { text: "用户名(User):" }
                        TextField { 
                            id: proxyUser
                            placeholderText: "例如: username"
                            text: "username"
                        }
                        
                        Label { text: "密码(Password):" }
                        TextField { 
                            id: proxyPassword
                            placeholderText: "例如: password123"
                            text: "password123"
                            echoMode: TextInput.Password
                        }
                    }
                }
                
                // GPS信息
                GroupBox {
                    title: "GPS信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "经度(Longitude):" }
                        TextField { 
                            id: gpsLongitude
                            placeholderText: "例如: 121.4737"
                            text: "121.4737"
                        }
                        
                        Label { text: "纬度(Latitude):" }
                        TextField { 
                            id: gpsLatitude
                            placeholderText: "例如: 31.2304"
                            text: "31.2304"
                        }
                    }
                }
                
                // SIM卡信息
                GroupBox {
                    title: "SIM卡信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "状态(State):" }
                        TextField { 
                            id: simState
                            placeholderText: "例如: 1"
                            text: "1"
                            validator: IntValidator { bottom: 0 }
                        }
                        
                        Label { text: "电话号码(PhoneNumber):" }
                        TextField { 
                            id: simPhoneNumber
                            placeholderText: "例如: 13800138000"
                            text: "13800138000"
                        }
                        
                        Label { text: "IMSI:" }
                        TextField { 
                            id: simIMSI
                            placeholderText: "例如: 460001234567890"
                            text: "460001234567890"
                        }
                        
                        Label { text: "ICCID:" }
                        TextField { 
                            id: simICCID
                            placeholderText: "例如: 89860123456789012345"
                            text: "89860123456789012345"
                        }
                    }
                }
                
                // 时区信息
                GroupBox {
                    title: "时区信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "时区(Timezone):" }
                        TextField { 
                            id: localeTimezone
                            placeholderText: "例如: Asia/Shanghai"
                            text: "Asia/Shanghai"
                        }
                    }
                }
                
                // 语言信息
                GroupBox {
                    title: "语言信息"
                    Layout.fillWidth: true
                    GridLayout {
                        columns: 2
                        width: parent.width
                        
                        Label { text: "语言(Language):" }
                        TextField { 
                            id: languageLanguage
                            placeholderText: "例如: zh"
                            text: "zh"
                        }
                        
                        Label { text: "国家(Country):" }
                        TextField { 
                            id: languageCountry
                            placeholderText: "例如: CN"
                            text: "CN"
                        }
                    }
                }
                
                // 额外属性
                GroupBox {
                    title: "额外属性"
                    Layout.fillWidth: true
                    ColumnLayout {
                        width: parent.width
                        spacing: 5
                        
                        Repeater {
                            id: extraPropertiesRepeater
                            model: ListModel {
                                ListElement { key: "exampleKey1"; value: "exampleValue1" }
                                ListElement { key: "exampleKey2"; value: "exampleValue2" }
                            }
                            
                            RowLayout {
                                width: parent.width
                                spacing: 5
                                
                                TextField {
                                    text: model.key
                                    placeholderText: "键名"
                                    Layout.fillWidth: true
                                    onTextChanged: model.key = text
                                }
                                
                                TextField {
                                    text: model.value
                                    placeholderText: "键值"
                                    Layout.fillWidth: true
                                    onTextChanged: model.value = text
                                }
                                
                                Button {
                                    text: "删除"
                                    onClicked: extraPropertiesRepeater.model.remove(index)
                                }
                            }
                        }
                        
                        Button {
                            text: "添加新属性"
                            onClicked: extraPropertiesRepeater.model.append({key: "", value: ""})
                        }
                    }
                }
            }
        }
        
        onAccepted: {
            // 构建JSON对象
            var json = {
                "RequestID": "request_" + Date.now(),
                "DeviceInfo": {
                    "Brand": deviceBrand.text,
                    "Model": deviceModel.text
                },
                "ProxyInfo": {
                    "Enabled": proxyEnabled.checked,
                    "Protocol": proxyProtocol.text,
                    "Host": proxyHost.text,
                    "Port": parseInt(proxyPort.text),
                    "User": proxyUser.text,
                    "Password": proxyPassword.text
                },
                "GPSInfo": {
                    "Longitude": parseFloat(gpsLongitude.text),
                    "Latitude": parseFloat(gpsLatitude.text)
                },
                "SIMInfo": {
                    "State": parseInt(simState.text),
                    "PhoneNumber": simPhoneNumber.text,
                    "IMSI": simIMSI.text,
                    "ICCID": simICCID.text
                },
                "LocaleInfo": {
                    "Timezone": localeTimezone.text
                },
                "LanguageInfo": {
                    "Language": languageLanguage.text,
                    "Country": languageCountry.text
                },
                "ExtraProperties": []
            };
            
            // 添加额外属性
            for (var i = 0; i < extraPropertiesRepeater.count; i++) {
                var item = extraPropertiesRepeater.model.get(i);
                if (item.key && item.value) {
                    json.ExtraProperties.push({
                        "Key": item.key,
                        "Value": item.value
                    });
                }
            }
            
            // 发送信号
            dialogsRoot.modifyInstancePropertiesAccepted(JSON.stringify(json));
        }
    }

    // 9. 修改保活应用状态
    Dialog {
        id: modifyKeepFrontAppStatusDialog
        title: "修改保活应用状态"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight + 30
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "PackageName"; Layout.preferredWidth: 80 }
                TextField { 
                    id: keepFrontPackageName; 
                    placeholderText: "PackageName"; 
                    text: "com.example.app" 
                }
            }
            CheckBox { 
                id: keepFrontEnable; 
                text: "Enable"; 
                checked: true 
            }
            RowLayout {
                Label { text: "RestartIntervalSeconds"; Layout.preferredWidth: 80 }
                TextField { 
                    id: keepFrontRestartInterval; 
                    placeholderText: "RestartIntervalSeconds"; 
                    text: "3"; 
                    validator: IntValidator { bottom: 0 } 
                }
            }
        }
        onAccepted: dialogsRoot.modifyKeepFrontAppStatusAccepted(
            keepFrontPackageName.text, keepFrontEnable.checked, parseInt(keepFrontRestartInterval.text)
        )
    }

    // 10. 卸载应用
    Dialog {
        id: uninstallAppDialog
        title: "卸载应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: uninstallPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
        }
        onAccepted: dialogsRoot.uninstallAppAccepted(uninstallPackageName.text)
    }

    // 11. 启动应用
    Dialog {
        id: startAppDialog
        title: "启动应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight + 30
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: startAppPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
            Label { text: "ActivityName" }
            TextField { 
                id: startAppActivityName; 
                placeholderText: "ActivityName";
                text: "com.example.app.MainActivity" 
            }
        }
        onAccepted: dialogsRoot.startAppAccepted(startAppPackageName.text, startAppActivityName.text)
    }

    // 12. 停止应用
    Dialog {
        id: stopAppDialog
        title: "停止应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: stopAppPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
        }
        onAccepted: dialogsRoot.stopAppAccepted(stopAppPackageName.text)
    }

    // 13. 清除应用数据
    Dialog {
        id: clearAppDataDialog
        title: "清除应用数据"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: clearAppDataPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
        }
        onAccepted: dialogsRoot.clearAppDataAccepted(clearAppDataPackageName.text)
    }

    // 14. 启用应用
    Dialog {
        id: enableAppDialog
        title: "启用应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: enableAppPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
        }
        onAccepted: dialogsRoot.enableAppAccepted(enableAppPackageName.text)
    }

    // 15. 禁用应用
    Dialog {
        id: disableAppDialog
        title: "禁用应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "PackageName" }
            TextField { 
                id: disableAppPackageName; 
                placeholderText: "PackageName";
                text: "com.example.app" 
            }
        }
        onAccepted: dialogsRoot.disableAppAccepted(disableAppPackageName.text)
    }

    // 16. 摄像头媒体播放
    Dialog {
        id: startCameraMediaPlayDialog
        title: "摄像头媒体播放"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight + 30
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "FilePath"; Layout.preferredWidth: 80 }
                TextField { 
                    id: cameraMediaFilePath; 
                    placeholderText: "FilePath"; 
                    text: "/sdcard/video.mp4" 
                }
            }
            RowLayout {
                Label { text: "Loops"; Layout.preferredWidth: 80 }
                TextField { 
                    id: cameraMediaLoops; 
                    placeholderText: "Loops"; 
                    text: "1"; 
                    validator: IntValidator { bottom: 1 } 
                }
            }
        }
        onAccepted: dialogsRoot.startCameraMediaPlayAccepted(cameraMediaFilePath.text, parseInt(cameraMediaLoops.text))
    }

    // 17. 显示摄像头图片
    Dialog {
        id: displayCameraImageDialog
        title: "显示摄像头图片"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "FilePath" }
            TextField { 
                id: cameraImageFilePath; 
                placeholderText: "FilePath";
                text: "/sdcard/image.jpg" 
            }
        }
        onAccepted: dialogsRoot.displayCameraImageAccepted(cameraImageFilePath.text)
    }

    // 18. 添加保活列表
    Dialog {
        id: addKeepAliveListDialog
        title: "添加保活列表"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: addKeepAliveAppList; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
            }
        }
        onAccepted: dialogsRoot.addKeepAliveListAccepted(addKeepAliveAppList.text)
    }

    // 19. 移除保活列表
    Dialog {
        id: removeKeepAliveListDialog
        title: "移除保活列表"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: removeKeepAliveAppList; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
            }
        }
        onAccepted: dialogsRoot.removeKeepAliveListAccepted(removeKeepAliveAppList.text)
    }

    // 20. 设置保活列表
    Dialog {
        id: setKeepAliveListDialog
        title: "设置保活列表"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: setKeepAliveAppList; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            var appList = setKeepAliveAppList.text.split(',').filter(app => app.trim() !== "");
            dialogsRoot.setKeepAliveListAccepted(appList);
        }
    }

    // 21. 查询实例属性
    Dialog {
        id: describeInstancePropertiesDialog
        title: "查询实例属性"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认查询选中实例的属性？" }
        }
        onAccepted: dialogsRoot.describeInstancePropertiesAccepted()
    }

    // 22. 查询保活应用状态
    Dialog {
        id: describeKeepFrontAppStatusDialog
        title: "查询保活应用状态"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认查询选中实例的保活应用状态？" }
        }
        onAccepted: dialogsRoot.describeKeepFrontAppStatusAccepted()
    }

    // 23. 停止摄像头播放
    Dialog {
        id: stopCameraMediaPlayDialog
        title: "停止摄像头播放"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认停止摄像头播放？" }
        }
        onAccepted: dialogsRoot.stopCameraMediaPlayAccepted()
    }

    // 24. 查询摄像头播放状态
    Dialog {
        id: describeCameraMediaPlayStatusDialog
        title: "查询摄像头播放状态"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认查询摄像头播放状态？" }
        }
        onAccepted: dialogsRoot.describeCameraMediaPlayStatusAccepted()
    }

    // 25. 查询保活列表
    Dialog {
        id: describeKeepAliveListDialog
        title: "查询保活列表"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认查询保活列表？" }
        }
        onAccepted: dialogsRoot.describeKeepAliveListAccepted()
    }

    // 26. 清除保活列表
    Dialog {
        id: clearKeepAliveListDialog
        title: "清除保活列表"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认清除保活列表？" }
        }
        onAccepted: dialogsRoot.clearKeepAliveListAccepted()
    }

    // 27. 列出用户应用
    Dialog {
        id: listUserAppsDialog
        title: "列出用户应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认列出用户应用？" }
        }
        onAccepted: dialogsRoot.listUserAppsAccepted()
    }

    // 28. 静音
    Dialog {
        id: muteDialog
        title: "静音"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            CheckBox { 
                id: muteCheck; 
                text: "静音"; 
                checked: false 
            }
        }
        onAccepted: dialogsRoot.muteAccepted(muteCheck.checked)
    }

    // 29. 媒体库搜索
    Dialog {
        id: mediaSearchDialog
        title: "媒体库搜索"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            RowLayout {
                Label { text: "Keyword"; Layout.preferredWidth: 80 }
                TextField { 
                    id: mediaSearchKeyword; 
                    placeholderText: "Keyword"; 
                    text: "example" 
                }
            }
        }
        onAccepted: dialogsRoot.mediaSearchAccepted(mediaSearchKeyword.text)
    }

    // 30. 重启实例
    Dialog {
        id: rebootDialog
        title: "重启实例"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认重启选中实例？" }
        }
        onAccepted: dialogsRoot.rebootAccepted()
    }

    // 31. 列出所有应用
    Dialog {
        id: listAllAppsDialog
        title: "列出所有应用"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认列出所有应用？" }
        }
        onAccepted: dialogsRoot.listAllAppsAccepted()
    }

    // 32. 应用移至后台
    Dialog {
        id: moveAppBackgroundDialog
        title: "应用移至后台"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认将应用移至后台？" }
        }
        onAccepted: dialogsRoot.moveAppBackgroundAccepted()
    }

    // 33. 添加安装黑名单
    Dialog {
        id: addAppInstallBlackListDialog
        title: "添加安装黑名单"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: addAppInstallBlackListField; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            var appList = addAppInstallBlackListField.text.split(',').filter(app => app.trim() !== "");
            dialogsRoot.addAppInstallBlackListAccepted(appList);
        }
    }

    // 34. 移除安装黑名单
    Dialog {
        id: removeAppInstallBlackListDialog
        title: "移除安装黑名单"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: removeAppInstallBlackListField; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            var appList = removeAppInstallBlackListField.text.split(',').filter(app => app.trim() !== "");
            dialogsRoot.removeAppInstallBlackListAccepted(appList);
        }
    }

    // 35. 设置安装黑名单
    Dialog {
        id: setAppInstallBlackListDialog
        title: "设置安装黑名单"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "AppList(逗号分隔)" }
            TextField { 
                id: setAppInstallBlackListField; 
                placeholderText: "AppList(逗号分隔)";
                text: "com.wechat,com.alipay,com.dingtalk" 
                Layout.fillWidth: true
            }
        }
        onAccepted: {
            var appList = setAppInstallBlackListField.text.split(',').filter(app => app.trim() !== "");
            dialogsRoot.setAppInstallBlackListAccepted(appList);
        }
    }

    // 36. 查询安装黑名单
    Dialog {
        id: describeAppInstallBlackListDialog
        title: "查询安装黑名单"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认查询安装黑名单？" }
        }
        onAccepted: dialogsRoot.describeAppInstallBlackListAccepted()
    }

    // 37. 清空安装黑名单
    Dialog {
        id: clearAppInstallBlackListDialog
        title: "清空安装黑名单"
        standardButtons: Dialog.Ok | Dialog.Cancel
        width: dialogsRoot.dialogWidth
        height: 120
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            Label { text: "确认清空安装黑名单？" }
        }
        onAccepted: dialogsRoot.clearAppInstallBlackListAccepted()
    }
}