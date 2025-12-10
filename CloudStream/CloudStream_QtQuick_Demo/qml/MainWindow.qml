import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import CustomComponents 1.0

Window {
    id: multiInstanceWindow
    visible: true
    title: "多实例视频渲染窗口"
    width: 1660
    height: 879

    // ============================================
    // 属性定义区域
    // ============================================
    
    // 基础配置属性
    property var instanceIds: []                    // 传入的实例ID列表
    property var accessInfo: null                   // 访问信息
    property string token: ""                       // 认证令牌
    readonly property int instancesPerSession: 100   // 硬编码的每个session的实例数量
    
    // 实例管理属性
    property var checkedInstanceIds: []             // 选中的实例ID列表
    property var instanceConfigs: []                // 格式: [{instanceId: "xxx", instanceIndex: 0, sessionIndex: 0}, ...]
    
    // 窗口管理属性
    property var syncWindow: null                   // 同步窗口（单例）
    property var syncViewModel: null                // 同步窗口的视图模型（单例）
    property var singleInstanceWindows: ({})        // instanceId -> window 映射（单实例窗口）
    
    // 全选状态
    property bool isAllSelected: false              // 是否全选状态
    
    // 视图模型
    property MultiStreamViewModel multiInstanceViewModel: MultiStreamViewModel {}
    
    // 监听会话关闭事件
    Connections {
        target: multiInstanceViewModel
        function onSessionClosed(sessionIndex, instanceIds, reason) {
            // 弹出对话框
            sessionClosedDialog.sessionIndex = sessionIndex;
            sessionClosedDialog.instanceIds = instanceIds;
            sessionClosedDialog.reason = reason;
            sessionClosedDialog.open();
        }
    }

    // ============================================
    // 工具函数区域
    // ============================================
    
    /**
     * 计算session配置
     * @param instanceIdList 实例ID列表
     * @return session配置数组，每个元素是一个实例ID数组
     */
    function calculateSessionConfigs(instanceIdList) {
        var sessionConfigs = [];
        for (var i = 0; i < instanceIdList.length; i += instancesPerSession) {
            var sessionInstances = [];
            for (var j = i; j < Math.min(i + instancesPerSession, instanceIdList.length); j++) {
                sessionInstances.push(instanceIdList[j]);
            }
            sessionConfigs.push(sessionInstances);
        }
        console.log("需要创建的session配置:", JSON.stringify(sessionConfigs));
        return sessionConfigs;
    }

    /**
     * 计算实例配置
     * @param sessionConfigs session配置数组
     * @return 实例配置数组，包含instanceId、instanceIndex、sessionIndex
     */
    function calculateInstanceConfigs(sessionConfigs) {
        var configs = [];
        for (var i = 0; i < sessionConfigs.length; i++) {
            var sessionInstances = sessionConfigs[i];
            for (var j = 0; j < sessionInstances.length; j++) {
                configs.push({
                    instanceId: sessionInstances[j],
                    instanceIndex: j,
                    sessionIndex: i
                });
            }
        }
        return configs;
    }
    
    /**
     * 创建同步窗口（单例模式）
     * @param targetInstanceIds 目标实例ID列表
     */
    function createSyncWindow(targetInstanceIds) {
        if (!targetInstanceIds || targetInstanceIds.length === 0) {
            console.error("目标实例ID列表为空");
            return;
        }
        
        // 如果同步窗口已存在，直接显示并更新实例列表
        if (syncWindow) {
            syncWindow.show();
            syncWindow.raise && syncWindow.raise();
            syncWindow.requestActivate && syncWindow.requestActivate();
            
            // 更新视图模型的实例列表
            if (syncViewModel) {
                syncViewModel.updateCheckedInstanceIds(targetInstanceIds);
            }
            return;
        }
        
        // 创建新的同步视图模型
        syncViewModel = Qt.createQmlObject(
            'import CustomComponents 1.0; StreamingViewModel {}', 
            multiInstanceWindow
        );
        syncViewModel.connectSession(targetInstanceIds, true);
        
        // 创建窗口
        var window = createWindow(syncViewModel);
        if (!window) return;
        
        syncWindow = window;
        
        // 绑定窗口关闭事件
        syncWindow.closing.connect(function() {
            console.log("同步窗口关闭");
            
            // 1. 关闭会话，释放TcrSdk资源
            if (syncViewModel && syncViewModel.closeSession) {
                syncViewModel.closeSession();
            }
            
            // 2. 销毁视图模型
            if (syncViewModel) {
                syncViewModel.destroy();
                syncViewModel = null;
            }
            
            // 3. 销毁窗口
            if (syncWindow) {
                syncWindow.destroy();
                syncWindow = null;
            }
        });
    }
    
    /**
     * 创建单实例窗口
     * @param instanceId 实例ID
     */
    function createSingleInstanceWindow(instanceId) {
        if (!instanceId) {
            console.error("实例ID为空");
            return;
        }
        
        // 如果窗口已存在，直接显示
        if (singleInstanceWindows[instanceId]) {
            var existingWindow = singleInstanceWindows[instanceId];
            existingWindow.show();
            existingWindow.raise && existingWindow.raise();
            existingWindow.requestActivate && existingWindow.requestActivate();
            return;
        }
        
        // 创建新的视图模型
        var viewModel = Qt.createQmlObject(
            'import CustomComponents 1.0; StreamingViewModel {}', 
            multiInstanceWindow
        );
        viewModel.connectSession([instanceId], false);
        
        // 创建窗口
        var window = createWindow(viewModel);
        if (!window) return;
        
        singleInstanceWindows[instanceId] = {
            window: window,
            viewModel: viewModel
        };
        
        // 绑定窗口关闭事件
        window.closing.connect(function() {
            console.log("单实例窗口关闭:", instanceId);
            
            // 获取窗口信息
            var windowInfo = singleInstanceWindows[instanceId];
            if (windowInfo) {
                // 1. 关闭会话，释放TcrSdk资源
                if (windowInfo.viewModel && windowInfo.viewModel.closeSession) {
                    windowInfo.viewModel.closeSession();
                }
                
                // 2. 销毁视图模型
                if (windowInfo.viewModel) {
                    windowInfo.viewModel.destroy();
                }
                
                // 3. 销毁窗口
                if (windowInfo.window) {
                    windowInfo.window.destroy();
                }
            }
            
            // 4. 清除引用
            delete singleInstanceWindows[instanceId];
        });
    }
    
    /**
     * 创建窗口的通用方法
     * @param viewModel 视图模型
     * @return 创建的窗口对象，失败返回null
     */
    function createWindow(viewModel) {
        var component = Qt.createComponent("StreamingWindow.qml");
        if (component.status !== Component.Ready) {
            console.error("创建流媒体窗口失败:", component.errorString());
            return null;
        }
        
        var window = component.createObject(multiInstanceWindow, {
            streamingViewModel: viewModel
        });
        window.show();
        return window;
    }
    
    /**
     * 更新选中的实例ID列表
     * @param instanceId 实例ID
     * @param isChecked 是否选中
     * @return 更新后的选中列表，如果操作被阻止则返回null
     */
    function updateCheckedInstances(instanceId, isChecked) {
        var newCheckedIds = checkedInstanceIds.slice(); // 创建副本
        
        if (isChecked) {
            // 添加到选中列表
            if (newCheckedIds.indexOf(instanceId) === -1) {
                newCheckedIds.push(instanceId);
            }
        } else {
            // 从选中列表移除
            var idx = newCheckedIds.indexOf(instanceId);
            if (idx !== -1) {
                newCheckedIds.splice(idx, 1);
            }
        }
        
        return newCheckedIds;
    }
    
    /**
     * @brief 同步更新同步窗口的选中实例列表
     * @param previousCheckedIds 之前选中的实例ID列表（可选）
     */
    function syncToSyncWindow(previousCheckedIds) {
        if (syncViewModel && syncViewModel.updateCheckedInstanceIds) {
            // 计算新增的实例列表
            var addedIds = [];
            if (previousCheckedIds) {
                for (var i = 0; i < checkedInstanceIds.length; i++) {
                    var id = checkedInstanceIds[i];
                    if (previousCheckedIds.indexOf(id) === -1) {
                        addedIds.push(id);
                    }
                }
            }
            
            syncViewModel.updateCheckedInstanceIds(checkedInstanceIds, addedIds);
        }
    }
    
    /**
     * 切换全选状态
     */
    function toggleSelectAll() {
        var previousCheckedIds = checkedInstanceIds.slice();
        
        if (isAllSelected) {
            // 取消全选
            checkedInstanceIds = [];
            isAllSelected = false;
        } else {
            // 全选
            checkedInstanceIds = instanceIds.slice();
            isAllSelected = true;
        }
        
        // 同步到同步窗口
        syncToSyncWindow(previousCheckedIds);
    }

    // ============================================
    // 生命周期事件处理
    // ============================================
    
    onVisibleChanged: {
        console.log("MultiInstanceWindow visible changed:", visible)
    }

    onClosing: {
        console.log("MultiInstanceWindow closing")
        multiInstanceViewModel.closeAllSessions();
    }

    Component.onDestruction: {
        console.log("MultiInstanceWindow destroyed")
    }

    Component.onCompleted: {
        console.log("MultiInstanceWindow completed, instanceIds:", instanceIds)
        if (instanceIds && instanceIds.length > 0) {
            multiInstanceViewModel.initialize(instanceIds, accessInfo, token);
            
            // 计算并连接多个实例
            var sessionConfigs = calculateSessionConfigs(instanceIds);
            instanceConfigs = calculateInstanceConfigs(sessionConfigs);
            multiInstanceViewModel.connectMultipleInstances(sessionConfigs);
        }
    }

    // ============================================
    // UI组件区域
    // ============================================
    
    // 顶部控制栏
    RowLayout {
        id: topControls
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        Text {
            text: "多实例视频渲染 - 总计: " + instanceIds.length + " 个实例 (每会话" + instancesPerSession + "个)"
            font.pixelSize: 16
            font.bold: true
        }
        
        Button {
            text: isAllSelected ? "取消全选" : "全选"
            onClicked: {
                toggleSelectAll();
            }
        }

        Button {
            text: "同步操作"
            enabled: checkedInstanceIds.length > 0
            onClicked: {
                if (checkedInstanceIds.length > 0) {
                    createSyncWindow(checkedInstanceIds);
                }
            }
        }

        Button {
            text: "暂停音视频流(部分)"
            enabled: instanceIds.length >= 2
            onClicked: {
                // 获取前两个实例ID
                var targetInstanceIds = instanceIds.slice(0, 2);
                console.log("暂停音视频流，实例:", targetInstanceIds);
                
                // 调用C++接口暂停流媒体
                // 注意：需要在MultiStreamViewModel中添加对应的Q_INVOKABLE方法
                if (multiInstanceViewModel.pauseStreaming) {
                    multiInstanceViewModel.pauseStreaming(targetInstanceIds);
                }
            }
        }

        Button {
            text: "恢复音视频流(部分)"
            enabled: instanceIds.length >= 2
            onClicked: {
                // 获取前两个实例ID
                var targetInstanceIds = instanceIds.slice(0, 2);
                console.log("恢复音视频流，实例:", targetInstanceIds);
                
                // 调用C++接口恢复流媒体
                // 注意：需要在MultiStreamViewModel中添加对应的Q_INVOKABLE方法
                if (multiInstanceViewModel.resumeStreaming) {
                    multiInstanceViewModel.resumeStreaming(targetInstanceIds);
                }
            }
        }

        Item { Layout.fillWidth: true } // 弹性空间

        Button {
            text: "重新连接"
            onClicked: {
                if (instanceIds && instanceIds.length > 0) {
                    var sessionConfigs = calculateSessionConfigs(instanceIds);
                    multiInstanceViewModel.connectMultipleInstances(sessionConfigs);
                }
            }
        }

        Button {
            text: "关闭所有会话"
            onClicked: {
                multiInstanceViewModel.closeAllSessions();
            }
        }
    }

    // 视频渲染网格区域
    GridView {
        id: videoGridView
        anchors.top: topControls.bottom
        anchors.topMargin: 10
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        clip: true
        
        cellWidth: (parent.width - 20) / 30
        cellHeight: cellWidth * 240 / 135
        
        model: instanceConfigs
        
        delegate: Rectangle {
            id: videoCell
            width: videoGridView.cellWidth - 10
            height: videoGridView.cellHeight - 10
            border.color: "gray"
            border.width: 1
            
            // 实例信息属性
            property string instanceId: modelData.instanceId
            property int instanceIndex: modelData.instanceIndex
            property int sessionIndex: modelData.sessionIndex
            property string uniqueKey: instanceId + "_" + instanceIndex
            // 直接绑定到 connectedInstanceIds 列表，避免信号丢失
            property bool isConnected: multiInstanceViewModel.connectedInstanceIds.indexOf(instanceId) !== -1
            
            // 点击事件处理
            MouseArea {
                anchors.fill: parent
                onClicked: {
                    createSingleInstanceWindow(videoCell.instanceId);
                }
            }
            
            // 视频渲染项
            VideoRenderItem {
                id: videoRenderItem
                anchors.fill: parent
                objectName: "videoRenderItem_" + videoCell.uniqueKey
                
                Component.onCompleted: {
                    console.log("Registering VideoRenderItem for instance:", 
                               videoCell.instanceId, "index:", videoCell.instanceIndex)
                    multiInstanceViewModel.registerVideoRenderItem(
                        videoCell.instanceId, 
                        videoCell.instanceIndex, 
                        videoRenderItem
                    )
                }
            }
            
            // 选择复选框
            CheckBox { 
                id: instanceCheckBox
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 2
                checked: checkedInstanceIds.indexOf(videoCell.instanceId) !== -1
                
                onCheckedChanged: {
                    // 保存旧的选中列表
                    var previousCheckedIds = checkedInstanceIds.slice();
                    
                    var newCheckedIds = updateCheckedInstances(videoCell.instanceId, checked);
                    
                    // 如果操作被阻止，恢复checkbox状态
                    if (newCheckedIds === null) {
                        checked = true;
                        return;
                    }
                    
                    // 更新选中列表并同步到同步窗口
                    checkedInstanceIds = newCheckedIds;
                    syncToSyncWindow(previousCheckedIds);
                }
            }

            // 实例信息底部栏
            Rectangle {
                anchors.bottom: parent.bottom
                width: parent.width
                height: 30
                color: "#80000000"
                
                Column {
                    anchors.fill: parent
                    anchors.margins: 3
                    
                    Text {
                        text: videoCell.instanceId
                        color: "white"
                        font.pixelSize: 9
                        elide: Text.ElideRight
                        width: parent.width
                    }
                    
                    Text {
                        text: videoCell.isConnected ? "已连接" : "未连接"
                        color: videoCell.isConnected ? "lightgreen" : "red"
                        font.pixelSize: 8
                    }
                }
            }
        }
    }
    
    // 底部状态栏
    Rectangle {
        anchors.bottom: parent.bottom
        width: parent.width
        height: 25
        color: "#f0f0f0"
        border.color: "#cccccc"
        border.width: 1
        
        RowLayout {
            anchors.fill: parent
            anchors.margins: 5
            
            Text {
                text: "连接状态: " + multiInstanceViewModel.connectedInstanceIds.length + "/" + instanceIds.length
                font.pixelSize: 12
            }
            
            Text {
                text: "已选中: " + checkedInstanceIds.length + " 个实例"
                font.pixelSize: 12
                color: checkedInstanceIds.length > 0 ? "#007bff" : "#666666"
            }
            
            Item { Layout.fillWidth: true }
        }
    }
    
    // 会话关闭对话框
    Dialog {
        id: sessionClosedDialog
        title: "会话已关闭"
        modal: true
        anchors.centerIn: parent
        width: 400
        
        property int sessionIndex: -1
        property var instanceIds: []
        property string reason: ""
        
        contentItem: Column {
            spacing: 15
            padding: 20
            
            Text {
                text: "会话 " + sessionClosedDialog.sessionIndex + " 已断开连接"
                font.pixelSize: 16
                font.bold: true
                color: "#d32f2f"
            }
            
            Text {
                text: "受影响的实例 (" + sessionClosedDialog.instanceIds.length + " 个):"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: parent.width - 40
            }
            
            Rectangle {
                width: parent.width - 40
                height: Math.min(instanceListText.contentHeight + 20, 150)
                color: "#f5f5f5"
                border.color: "#cccccc"
                border.width: 1
                radius: 4
                
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    
                    Text {
                        id: instanceListText
                        text: sessionClosedDialog.instanceIds.join("\n")
                        font.pixelSize: 12
                        color: "#666666"
                    }
                }
            }
            
            Text {
                text: "关闭原因:"
                font.pixelSize: 14
                wrapMode: Text.WordWrap
                width: parent.width - 40
            }
            
            Rectangle {
                width: parent.width - 40
                height: Math.min(reasonText.contentHeight + 20, 100)
                color: "#fff3e0"
                border.color: "#ffb74d"
                border.width: 1
                radius: 4
                
                ScrollView {
                    anchors.fill: parent
                    anchors.margins: 10
                    clip: true
                    
                    Text {
                        id: reasonText
                        text: sessionClosedDialog.reason || "未知原因"
                        font.pixelSize: 12
                        color: "#e65100"
                        wrapMode: Text.WordWrap
                        width: parent.width
                    }
                }
            }
        }
        
        footer: DialogButtonBox {
            Button {
                text: "确定"
                DialogButtonBox.buttonRole: DialogButtonBox.AcceptRole
                onClicked: {
                    sessionClosedDialog.close();
                }
            }
            
            Button {
                text: "重新连接"
                DialogButtonBox.buttonRole: DialogButtonBox.ActionRole
                onClicked: {
                    sessionClosedDialog.close();
                    // 重新连接所有实例
                    if (instanceIds && instanceIds.length > 0) {
                        var sessionConfigs = calculateSessionConfigs(instanceIds);
                        multiInstanceViewModel.connectMultipleInstances(sessionConfigs);
                    }
                }
            }
        }
    }
}
