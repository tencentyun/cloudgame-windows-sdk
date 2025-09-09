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

    // 接收从 InstanceAccessWindow 传进来的实例ID列表、accessInfo、token
    property var instanceIds: []
    property var accessInfo: null
    property string token: ""

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
        instanceManager.initialize(instanceIds, accessInfo, token);
        // mainWindow.visibility = Window.Maximized;
    }

    // 顶部按钮区域
    RowLayout {
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

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
    }


    // 卡片区域
    GridView {
        id: gridView
        ScrollBar.vertical: ScrollBar {
            policy: ScrollBar.AlwaysOn
            onPressedChanged: {
                if (!pressed) {
                    var visibleIds = gridView.updateVisibleInstances();
                    instanceManager.setDownloadInstances(visibleIds);
                    instanceManager.resumeImageDownload();
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
        cellWidth: (parent.width - 20) / 20
        cellHeight: cellWidth * 240 / 135
        model: instanceManager.instances
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
            instanceManager.pauseImageDownload();
        }
        onMovementEnded: {
            isScrolling = false;
            var visibleIds = updateVisibleInstances();
            instanceManager.setDownloadInstances(visibleIds);
            instanceManager.resumeImageDownload();
        }
        delegate: Rectangle {
            width: gridView.cellWidth - 10
            height: gridView.cellHeight - 10
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
                    target: instanceManager
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
            }
        }
        }
    }

    Dialogs {
        id: dialogs
        anchors.centerIn: parent
    }
}