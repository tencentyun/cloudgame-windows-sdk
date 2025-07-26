import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import CustomComponents 1.0

Window {
    id: mainWindow
    visible: true
    title: "主窗口"
    width: 1660
    height: 879

    property var checkedInstanceIds: []
    property var streamingWindowMap: ({}) // AndroidInstanceId -> window
    property StreamingViewModel streamingViewModel: StreamingViewModel {}
    property var groupStreamingViewModels: []

    onVisibleChanged: {
        console.log("MainWindow visible changed:", visible)
    }
    onClosing: {
        console.log("MainWindow closing")
    }
    Component.onDestruction: {
        console.log("MainWindow destroyed")
    }

    Component.onCompleted: {
        var c = Qt.createComponent("StreamingWindow.qml");
        console.log("status:", c.status, c.errorString());
        streamingViewModel.setTcrOperator(batchTaskOperator);
    }

    // 顶部按钮区域
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        Button {
            text: "刷新列表"
            onClicked: {
                androidInstanceModel.refreshInstances();
            }
        }

        Button {
            text: "同步操作"
            onClicked: {
                if (mainWindow.checkedInstanceIds.length > 1) {
                    var instanceId = mainWindow.checkedInstanceIds[0];
                    if (mainWindow.streamingWindowMap[instanceId]) {
                        var win = mainWindow.streamingWindowMap[instanceId];
                        win.show();
                        win.raise && win.raise();
                        win.requestActivate && win.requestActivate();
                        return;
                    }
                    var vm = Qt.createQmlObject('import CustomComponents 1.0; StreamingViewModel {}', mainWindow);
                    vm.connectGroupSession(mainWindow.checkedInstanceIds);
                    if (mainWindow.groupStreamingViewModels.indexOf(vm) === -1)
                        mainWindow.groupStreamingViewModels.push(vm);

                    var component = Qt.createComponent("StreamingWindow.qml");
                    if (component.status === Component.Ready) {
                        var win = component.createObject(mainWindow, {
                            streamingViewModel: vm
                        });
                        win.show();
                        mainWindow.streamingWindowMap[instanceId] = win;
                        win.closing.connect(function() {
                            mainWindow.streamingWindowMap[instanceId] = undefined;
                            var idx = mainWindow.groupStreamingViewModels.indexOf(vm);
                            if (idx !== -1)
                                mainWindow.groupStreamingViewModels.splice(idx, 1);
                        });
                    } else {
                        console.error("创建流媒体窗口失败:", component.errorString());
                    }
                }
            }
        }

        // 批量操作菜单
        Button {
            text: "批量操作"
            onClicked: batchMenu.popup()
            Menu {
                id: batchMenu
                // 常用操作
                MenuItem { text: "设置GPS"; onTriggered: dialogs.gpsDialog.open() }
                MenuItem { text: "设置分辨率"; onTriggered: dialogs.resolutionDialog.open() }
                MenuItem { text: "粘贴文本"; onTriggered: dialogs.pasteDialog.open() }
                MenuItem { text: "发送剪贴板"; onTriggered: dialogs.sendClipboardDialog.open() }
                MenuItem { text: "修改传感器数据"; onTriggered: dialogs.modifySensorDialog.open() }
                MenuItem { text: "摇一摇"; onTriggered: dialogs.shakeDialog.open() }
                MenuItem { text: "吹气"; onTriggered: dialogs.blowDialog.open() }
                MenuItem { text: "透传消息"; onTriggered: dialogs.messageDialog.open() }
                MenuSeparator {}
                // 设备属性
                MenuItem { text: "修改实例属性"; onTriggered: dialogs.modifyInstancePropertiesDialog.open() }
                MenuItem { text: "修改保活应用状态"; onTriggered: dialogs.modifyKeepFrontAppStatusDialog.open() }
                MenuSeparator {}
                // 应用相关
                MenuItem { text: "卸载应用"; onTriggered: dialogs.uninstallAppDialog.open() }
                MenuItem { text: "启动应用"; onTriggered: dialogs.startAppDialog.open() }
                MenuItem { text: "停止应用"; onTriggered: dialogs.stopAppDialog.open() }
                MenuItem { text: "清除应用数据"; onTriggered: dialogs.clearAppDataDialog.open() }
                MenuItem { text: "启用应用"; onTriggered: dialogs.enableAppDialog.open() }
                MenuItem { text: "禁用应用"; onTriggered: dialogs.disableAppDialog.open() }
                MenuSeparator {}
                // 摄像头
                MenuItem { text: "摄像头媒体播放"; onTriggered: dialogs.startCameraMediaPlayDialog.open() }
                MenuItem { text: "显示摄像头图片"; onTriggered: dialogs.displayCameraImageDialog.open() }
                MenuSeparator {}
                // 保活列表
                MenuItem { text: "添加保活列表"; onTriggered: dialogs.addKeepAliveListDialog.open() }
                MenuItem { text: "移除保活列表"; onTriggered: dialogs.removeKeepAliveListDialog.open() }
                MenuItem { text: "设置保活列表"; onTriggered: dialogs.setKeepAliveListDialog.open() }
                MenuSeparator {}
                // 查询/控制
                MenuItem { text: "查询实例属性"; onTriggered: dialogs.describeInstancePropertiesDialog.open() }
                MenuItem { text: "查询保活应用状态"; onTriggered: dialogs.describeKeepFrontAppStatusDialog.open() }
                MenuItem { text: "停止摄像头播放"; onTriggered: dialogs.stopCameraMediaPlayDialog.open() }
                MenuItem { text: "查询摄像头播放状态"; onTriggered: dialogs.describeCameraMediaPlayStatusDialog.open() }
                MenuItem { text: "查询保活列表"; onTriggered: dialogs.describeKeepAliveListDialog.open() }
                MenuItem { text: "清除保活列表"; onTriggered: dialogs.clearKeepAliveListDialog.open() }
                MenuItem { text: "列出用户应用"; onTriggered: dialogs.listUserAppsDialog.open() }
                MenuSeparator {}
                // 其它
                MenuItem { text: "静音"; onTriggered: dialogs.muteDialog.open() }
                MenuItem { text: "媒体库搜索"; onTriggered: dialogs.mediaSearchDialog.open() }
                MenuItem { text: "重启实例"; onTriggered: dialogs.rebootDialog.open() }
                MenuItem { text: "列出所有应用"; onTriggered: dialogs.listAllAppsDialog.open() }
                MenuItem { text: "应用移至后台"; onTriggered: dialogs.moveAppBackgroundDialog.open() }
                MenuSeparator {}
                // 安装黑名单
                MenuItem { text: "添加安装黑名单"; onTriggered: dialogs.addAppInstallBlackListDialog.open() }
                MenuItem { text: "移除安装黑名单"; onTriggered: dialogs.removeAppInstallBlackListDialog.open() }
                MenuItem { text: "设置安装黑名单"; onTriggered: dialogs.setAppInstallBlackListDialog.open() }
                MenuItem { text: "查询安装黑名单"; onTriggered: dialogs.describeAppInstallBlackListDialog.open() }
                MenuItem { text: "清空安装黑名单"; onTriggered: dialogs.clearAppInstallBlackListDialog.open() }
            }
        }

        Item { Layout.fillWidth: true }
    }

    // 卡片区域
    GridView {
        id: gridView
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            onPressedChanged: {
                if (!pressed) {
                    var visibleIds = gridView.updateVisibleInstances();
                    androidInstanceModel.setDownloadInstances(visibleIds);
                    androidInstanceModel.resumeImageDownload();
                }
            }
        }
        anchors.top: parent.top
        anchors.topMargin: 50
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        clip: true
        cellWidth: 320
        cellHeight: 569
        model: androidInstanceModel.instances
        property bool isScrolling: false
        property var visibleInstanceIds: []
        function updateVisibleInstances() {
            var newVisibleInstances = [];
            var columns = Math.floor(width / cellWidth);
            if (columns < 1) columns = 1;
            var rows = Math.floor(height / cellHeight);
            if (rows < 1) rows = 1;
            var firstVisibleIndex = Math.floor(contentY / cellHeight) * columns;
            if (firstVisibleIndex < 0) firstVisibleIndex = 0;
            var visibleRowCount = Math.ceil(height / cellHeight) + 1;
            var lastVisibleIndex = firstVisibleIndex + visibleRowCount * columns - 1;
            if (lastVisibleIndex >= model.count) lastVisibleIndex = model.count - 1;
            for (var i = firstVisibleIndex; i <= lastVisibleIndex; i++) {
                if (model[i]) {
                    newVisibleInstances.push(model[i].AndroidInstanceId);
                }
            }
            visibleInstanceIds = newVisibleInstances;
            return newVisibleInstances;
        }
        onMovementStarted: {
            isScrolling = true;
            androidInstanceModel.pauseImageDownload();
        }
        onMovementEnded: {
            isScrolling = false;
            var visibleIds = updateVisibleInstances();
            androidInstanceModel.setDownloadInstances(visibleIds);
            androidInstanceModel.resumeImageDownload();
        }
        delegate: Rectangle {
            width: 310
            height: 559
            border.color: "gray"
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    var instanceId = modelData.AndroidInstanceId;
                    if (mainWindow.streamingWindowMap[instanceId]) {
                        var win = mainWindow.streamingWindowMap[instanceId];
                        win.show();
                        win.raise && win.raise();
                        win.requestActivate && win.requestActivate();
                        return;
                    }
                    var vm = Qt.createQmlObject('import CustomComponents 1.0; StreamingViewModel {}', mainWindow);
                    vm.connectSession(instanceId);
                    var component = Qt.createComponent("StreamingWindow.qml");
                    if (component.status === Component.Ready) {
                        var win = component.createObject(mainWindow, {
                            streamingViewModel: vm
                        });
                        win.show();
                        mainWindow.streamingWindowMap[instanceId] = win;
                        win.closing.connect(function() {
                            mainWindow.streamingWindowMap[instanceId] = undefined;
                        });
                    } else {
                        console.error("创建流媒体窗口失败:", component.errorString());
                    }
                }
            }
            Image {
                id: instanceImage
                anchors.fill: parent
                source: "image://instance/" + modelData.AndroidInstanceId
                fillMode: Image.PreserveAspectCrop
                Connections {
                    target: androidInstanceModel
                    function onImageUpdated(instanceId) {
                        if (instanceId === modelData.AndroidInstanceId) {
                            var oldSource = instanceImage.source;
                            instanceImage.source = "";
                            instanceImage.source = oldSource;
                        }
                    }
                }
            }
        CheckBox { 
            anchors.right: parent.right
            anchors.top: parent.top
            checked: mainWindow.checkedInstanceIds.indexOf(modelData.AndroidInstanceId) !== -1
            onCheckedChanged: {
                if (checked) {
                    if (mainWindow.checkedInstanceIds.indexOf(modelData.AndroidInstanceId) === -1)
                        mainWindow.checkedInstanceIds.push(modelData.AndroidInstanceId);
                } else {
                    if (mainWindow.groupStreamingViewModels.length > 0
                        && mainWindow.checkedInstanceIds.length === 2
                        && mainWindow.checkedInstanceIds.indexOf(modelData.AndroidInstanceId) !== -1) {
                        checked = true;
                        dialogs.genericTipDialog.close();
                        dialogs.genericTipDialog.tipTitle = "操作提示"
                        dialogs.genericTipDialog.tipMessage = "正在进行同步操作, 无法取消"
                        dialogs.genericTipDialog.open();
                        return;
                    }
                    var idx = mainWindow.checkedInstanceIds.indexOf(modelData.AndroidInstanceId);
                    if (idx !== -1)
                        mainWindow.checkedInstanceIds.splice(idx, 1);
                }
                for (var i = 0; i < mainWindow.groupStreamingViewModels.length; ++i) {
                    var vm = mainWindow.groupStreamingViewModels[i];
                    if (vm) {
                        vm.updateCheckedInstanceIds(mainWindow.checkedInstanceIds);
                    }
                }
            }
        }
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 50
                color: "#80000000"
                Column {
                    anchors.fill: parent
                    anchors.margins: 5
                    Text {
                        text: modelData.AndroidInstanceId
                        color: "white"
                        elide: Text.ElideRight
                    }
                    Text {
                        text: (function() {
                            const stateMap = {
                                "NORMAL": '运行中',
                                "STARTING": '开机中',
                                "STOPPING": '关机中',
                                "UPGRADING": '服务更新中',
                                "REBOOTING": '重启中',
                                "RESETING": '重置中',
                                "STOPPED": '已关机',
                                "IMAGE_CREATING": '镜像制作中',
                                "INITIALIZING": '初始化中',
                                "BACKING_UP": '备份中',
                                "RESTORING": '还原中'
                            };
                            return stateMap[modelData.State] || modelData.State;
                        })()
                        color: "white"
                    }
                    Text {
                        text: modelData.AndroidInstanceRegion
                        color: "white"
                        elide: Text.ElideRight
                    }
                }
            }
        }
    }

    // 所有批量操作弹窗
    Dialogs {
        id: dialogs
        anchors.centerIn: parent

        // 处理各类对话框的信号
        onGpsAccepted: (longitude, latitude) => batchTaskOperatorModel.handleDialogSignal("gps", mainWindow.checkedInstanceIds, { longitude, latitude })
        onResolutionAccepted: (width, height, dpi) => batchTaskOperatorModel.handleDialogSignal("resolution", mainWindow.checkedInstanceIds, { width, height, dpi })
        onPasteAccepted: (text) => batchTaskOperatorModel.handleDialogSignal("paste", mainWindow.checkedInstanceIds, { text })
        onSendClipboardAccepted: (text) => batchTaskOperatorModel.handleDialogSignal("sendClipboard", mainWindow.checkedInstanceIds, { text })
        onModifySensorAccepted: (sensorType, values, accuracy) => batchTaskOperatorModel.handleDialogSignal("modifySensor", mainWindow.checkedInstanceIds, { sensorType, values, accuracy }) 
        onShakeAccepted: () => batchTaskOperatorModel.handleDialogSignal("shake", mainWindow.checkedInstanceIds, {})
        onBlowAccepted: () => batchTaskOperatorModel.handleDialogSignal("blow", mainWindow.checkedInstanceIds, {})
        onMessageAccepted: (packageName, msg) => batchTaskOperatorModel.handleDialogSignal("message", mainWindow.checkedInstanceIds, { packageName, msg })
        onModifyInstancePropertiesAccepted: (json) => batchTaskOperatorModel.handleDialogSignal("modifyInstanceProperties", mainWindow.checkedInstanceIds, { json })
        onModifyKeepFrontAppStatusAccepted: (packageName, enable, restartInterval) => batchTaskOperatorModel.handleDialogSignal("modifyKeepFrontAppStatus", mainWindow.checkedInstanceIds, { packageName, enable, restartInterval })
        onUninstallAppAccepted: (packageName) => batchTaskOperatorModel.handleDialogSignal("unInstallByPackageName", mainWindow.checkedInstanceIds, { packageName })
        onStartAppAccepted: (packageName, activityName) => batchTaskOperatorModel.handleDialogSignal("startApp", mainWindow.checkedInstanceIds, { packageName, activityName })
        onStopAppAccepted: (packageName) => batchTaskOperatorModel.handleDialogSignal("stopApp", mainWindow.checkedInstanceIds, { packageName })
        onClearAppDataAccepted: (packageName) => batchTaskOperatorModel.handleDialogSignal("clearAppData", mainWindow.checkedInstanceIds, { packageName })
        onEnableAppAccepted: (packageName) => batchTaskOperatorModel.handleDialogSignal("enableApp", mainWindow.checkedInstanceIds, { packageName })
        onDisableAppAccepted: (packageName) => batchTaskOperatorModel.handleDialogSignal("disableApp", mainWindow.checkedInstanceIds, { packageName })
        onStartCameraMediaPlayAccepted: (filePath, loops) => batchTaskOperatorModel.handleDialogSignal("startCameraMediaPlay", mainWindow.checkedInstanceIds, { filePath, loops })
        onDisplayCameraImageAccepted: (filePath) => batchTaskOperatorModel.handleDialogSignal("displayCameraImage", mainWindow.checkedInstanceIds, { filePath })
        onAddKeepAliveListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("addKeepAliveList", mainWindow.checkedInstanceIds, { appList })
        onRemoveKeepAliveListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("removeKeepAliveList", mainWindow.checkedInstanceIds, { appList })
        onSetKeepAliveListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("setKeepAliveList", mainWindow.checkedInstanceIds, { appList })
        onDescribeInstancePropertiesAccepted: () => batchTaskOperatorModel.handleDialogSignal("describeInstanceProperties", mainWindow.checkedInstanceIds, {})
        onDescribeKeepFrontAppStatusAccepted: () => batchTaskOperatorModel.handleDialogSignal("describeKeepFrontAppStatus", mainWindow.checkedInstanceIds, {})
        onStopCameraMediaPlayAccepted: () => batchTaskOperatorModel.handleDialogSignal("stopCameraMediaPlay", mainWindow.checkedInstanceIds, {})
        onDescribeCameraMediaPlayStatusAccepted: () => batchTaskOperatorModel.handleDialogSignal("describeCameraMediaPlayStatus", mainWindow.checkedInstanceIds, {})
        onDescribeKeepAliveListAccepted: () => batchTaskOperatorModel.handleDialogSignal("describeKeepAliveList", mainWindow.checkedInstanceIds, {})
        onClearKeepAliveListAccepted: () => batchTaskOperatorModel.handleDialogSignal("clearKeepAliveList", mainWindow.checkedInstanceIds, {})
        onListUserAppsAccepted: () => batchTaskOperatorModel.handleDialogSignal("listUserApps", mainWindow.checkedInstanceIds, {})
        onMuteAccepted: (mute) => batchTaskOperatorModel.handleDialogSignal("mute", mainWindow.checkedInstanceIds, { mute })
        onMediaSearchAccepted: (keyword) => batchTaskOperatorModel.handleDialogSignal("mediaSearch", mainWindow.checkedInstanceIds, { keyword })
        onRebootAccepted: () => batchTaskOperatorModel.handleDialogSignal("reboot", mainWindow.checkedInstanceIds, {})
        onListAllAppsAccepted: () => batchTaskOperatorModel.handleDialogSignal("listAllApps", mainWindow.checkedInstanceIds, {})
        onMoveAppBackgroundAccepted: () => batchTaskOperatorModel.handleDialogSignal("moveAppBackground", mainWindow.checkedInstanceIds, {})
        onAddAppInstallBlackListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("addAppInstallBlackList", mainWindow.checkedInstanceIds, { appList })
        onRemoveAppInstallBlackListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("removeAppInstallBlackList", mainWindow.checkedInstanceIds, { appList })
        onSetAppInstallBlackListAccepted: (appList) => batchTaskOperatorModel.handleDialogSignal("setAppInstallBlackList", mainWindow.checkedInstanceIds, { appList })
        onDescribeAppInstallBlackListAccepted: () => batchTaskOperatorModel.handleDialogSignal("describeAppInstallBlackList", mainWindow.checkedInstanceIds, {})
        onClearAppInstallBlackListAccepted: () => batchTaskOperatorModel.handleDialogSignal("clearAppInstallBlackList", mainWindow.checkedInstanceIds, {})
    }
    
    Connections {
        target: batchTaskOperatorModel
        function onShowDialog(title, message) {
            dialogs.genericTipDialog.tipTitle = title
            dialogs.genericTipDialog.tipMessage = message
            dialogs.genericTipDialog.open()
        }
    }
}