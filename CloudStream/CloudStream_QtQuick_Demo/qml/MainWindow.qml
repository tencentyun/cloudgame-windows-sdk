import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import QtQuick.Window 2.15
import CustomComponents 1.0
import "components" as Components

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
    
    // 视图大小配置
    property int viewSize: 1                        // 视图大小: 0=小(30列), 1=中(10列), 2=大(5列)
    property var viewSizeColumns: [20, 10, 5]       // 对应每种视图的列数
    
    // 监听视图大小变化，触发可见性检测
    onViewSizeChanged: {
        // 延迟执行，等待GridView完成布局更新
        Qt.callLater(function() {
            videoGridView.checkVisibleItems();
            
            // 重新连接多实例，以使用新的 visibleInstancesPerPage
            if (instanceIds && instanceIds.length > 0) {
                var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
                console.log("[onViewSizeChanged] 视图大小变化，重新连接，visibleInstancesPerPage:", visibleInstancesPerPage);
                multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
            }
        });
    }
    
    // 统计数据显示开关
    property bool showStatsOverlay: false           // 是否显示统计数据蒙层
    
    // 实例管理属性
    property var checkedInstanceIds: []             // 选中的实例ID列表
    property var instanceConfigs: []                // 格式: [{instanceId: "xxx", instanceIndex: 0}, ...]
    
    // 窗口管理属性
    property var syncWindow: null                   // 同步窗口（单例）
    property var syncViewModel: null                // 同步窗口的视图模型（单例）
    property var singleInstanceWindows: ({})        // instanceId -> window 映射（单实例窗口）
    
    // 全选状态
    property bool isAllSelected: false              // 是否全选状态
    property bool isUpdatingCheckboxes: false       // 正在批量更新复选框状态
    
    // 视图模型
    property MultiStreamViewModel multiInstanceViewModel: MultiStreamViewModel {}
    
    // 监听会话连接状态变化
    Connections {
        target: multiInstanceViewModel
        function onSessionClosed(instanceIds, reason) {
            // 弹出对话框
            sessionClosedDialog.instanceIds = instanceIds;
            sessionClosedDialog.reason = reason;
            sessionClosedDialog.open();
        }
        
        function onClientStatsChanged() {
            // 统计数据更新
            // 可以在这里解析 JSON 并更新 UI
            // console.log("统计数据更新:", multiInstanceViewModel.clientStats);
        }
    }

    // ============================================
    // 工具函数区域
    // ============================================
    
    /**
     * 计算实例配置
     * @param instanceIdList 实例ID列表
     * @return 实例配置数组，包含instanceId和instanceIndex
     */
    function calculateInstanceConfigs(instanceIdList) {
        var configs = [];
        for (var i = 0; i < instanceIdList.length; i++) {
            configs.push({
                instanceId: instanceIdList[i],
                instanceIndex: i
            });
        }
        return configs;
    }
    
    /**
     * 计算每页可见的实例数量
     * @return 每页可见的实例数量（GridView可视区域内的格子数）
     */
    function calculateVisibleInstancesPerPage() {
        var cols = viewSizeColumns[viewSize];
        
        // 计算单元格尺寸
        var cellW = (multiInstanceWindow.width - 20) / cols;
        var cellH = cellW * 240 / 135;
        
        // 计算GridView可用高度（减去顶部控制栏和底部状态栏）
        var gridViewHeight = multiInstanceWindow.height - 100;  // 预估顶部和底部占用约100像素
        
        // 计算可见行数（向上取整确保覆盖部分可见的行，再加1行预加载）
        var visibleRows = Math.ceil(gridViewHeight / cellH) + 1;
        
        // 计算每页可见的实例数量
        var visibleInstances = visibleRows * cols;
        
        console.log("[calculateVisibleInstancesPerPage] 列数:", cols, "单元格高度:", cellH, 
                   "可见行数:", visibleRows, "每页可见实例数:", visibleInstances);
        
        return visibleInstances;
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
        
        // 设置更新标志，防止 CheckBox 的 onCheckedChanged 触发
        isUpdatingCheckboxes = true;
        
        if (isAllSelected) {
            // 取消全选
            checkedInstanceIds = [];
            isAllSelected = false;
        } else {
            // 全选
            checkedInstanceIds = instanceIds.slice();
            isAllSelected = true;
        }
        
        // 延迟重置更新标志，让所有 CheckBox 完成状态更新
        Qt.callLater(function() {
            isUpdatingCheckboxes = false;
        });
        
        // 同步到同步窗口
        syncToSyncWindow(previousCheckedIds);
    }

    /**
     * 复制选中的实例ID到剪贴板
     */
    function copyCheckedInstanceIds() {
        if (checkedInstanceIds.length === 0) {
            console.log("没有选中的实例ID");
            return;
        }
        
        // 使用逗号分隔实例ID
        var idsText = checkedInstanceIds.join(",");
        
        // 通过C++接口复制到剪贴板（确保Windows兼容性）
        multiInstanceViewModel.copyToClipboard(idsText);
        
        console.log("已复制实例ID:", idsText);
        
        // 显示提示信息
        copySuccessToast.show();
    }

    // ============================================
    // 生命周期事件处理
    // ============================================
    
    onVisibleChanged: {
        console.log("MultiInstanceWindow visible changed:", visible)
    }

    onClosing: {
        console.log("MultiInstanceWindow closing")
        multiInstanceViewModel.closeSession();
    }

    Component.onDestruction: {
        console.log("MultiInstanceWindow destroyed")
    }

    Component.onCompleted: {
        console.log("MultiInstanceWindow completed, instanceIds:", instanceIds)
        if (instanceIds && instanceIds.length > 0) {
            multiInstanceViewModel.initialize(instanceIds, accessInfo, token);
            
            // 计算实例配置
            instanceConfigs = calculateInstanceConfigs(instanceIds);
            
            // 计算每页可见的实例数量
            var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
            
            // 直接传递所有实例ID和同时串流数量给C++层
            multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
            
            // 延迟执行初始可见性检测，确保 GridView 完成布局
            Qt.callLater(function() {
                videoGridView.checkVisibleItems();
            });
        }
    }

    // ============================================
    // UI组件区域
    // ============================================
    
    // 复制成功提示
    Rectangle {
        id: copySuccessToast
        anchors.centerIn: parent
        width: 200
        height: 50
        color: "#80000000"
        radius: 8
        visible: false
        z: 1000
        
        Text {
            anchors.centerIn: parent
            text: "已复制到剪贴板"
            color: "white"
            font.pixelSize: 14
        }
        
        function show() {
            visible = true;
            hideTimer.restart();
        }
        
        Timer {
            id: hideTimer
            interval: 2000
            onTriggered: {
                copySuccessToast.visible = false;
            }
        }
    }
    
    // 顶部控制栏
    RowLayout {
        id: topControls
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 10
        spacing: 10

        Text {
            text: "多实例视频渲染 - 总计: " + instanceIds.length + " 个实例"
            font.pixelSize: 16
            font.bold: true
        }
        
        // 视图大小选择
        Text {
            text: "视图:"
            font.pixelSize: 14
            verticalAlignment: Text.AlignVCenter
        }
        
        ComboBox {
            id: viewSizeComboBox
            model: ["小", "中", "大"]
            currentIndex: viewSize
            implicitWidth: 80
            onCurrentIndexChanged: {
                viewSize = currentIndex;
                // 当视图大小不是"大"时，自动取消勾选统计数据显示
                if (viewSize !== 2) {
                    showStatsOverlay = false;
                }
            }
        }
        
        CheckBox {
            id: statsOverlayCheckBox
            text: "显示统计数据"
            checked: showStatsOverlay
            enabled: viewSize === 2  // 只有视图大小为"大"时才允许勾选
            onCheckedChanged: {
                showStatsOverlay = checked;
            }
            
            ToolTip.visible: hovered && !enabled
            ToolTip.text: "请先将视图大小切换为\"大\"才能启用统计数据显示"
        }
        
        Button {
            text: isAllSelected ? "取消全选" : "全选"
            onClicked: {
                toggleSelectAll();
            }
        }

        Button {
            text: "复制实例ID"
            enabled: checkedInstanceIds.length > 0
            onClicked: {
                copyCheckedInstanceIds();
            }
            
            ToolTip.visible: hovered
            ToolTip.text: "复制选中的实例ID（逗号分隔）"
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
            text: "暂停音视频流"
            enabled: checkedInstanceIds.length > 0
            onClicked: {
                // 获取选中的实例ID
                console.log("暂停音视频流，实例:", checkedInstanceIds);
                
                // 调用C++接口暂停流媒体
                if (multiInstanceViewModel.pauseStreaming) {
                    multiInstanceViewModel.pauseStreaming(checkedInstanceIds);
                }
            }
        }

        Button {
            text: "恢复音视频流"
            enabled: checkedInstanceIds.length > 0
            onClicked: {
                // 获取选中的实例ID
                console.log("恢复音视频流，实例:", checkedInstanceIds);
                
                // 调用C++接口恢复流媒体
                if (multiInstanceViewModel.resumeStreaming) {
                    multiInstanceViewModel.resumeStreaming(checkedInstanceIds);
                }
            }
        }

        Item { Layout.fillWidth: true } // 弹性空间

        Button {
            text: "重新连接"
            onClicked: {
                if (instanceIds && instanceIds.length > 0) {
                    // 计算每页可见的实例数量
                    var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
                    // 直接传递所有实例ID和同时串流数量
                    multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
                }
            }
        }

        Button {
            text: "关闭会话"
            onClicked: {
                multiInstanceViewModel.closeSession();
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
        
        cellWidth: (parent.width - 20) / viewSizeColumns[viewSize]
        cellHeight: cellWidth * 240 / 135
        
        model: instanceConfigs

        Timer {
            id: scrollStopTimer
            interval: 500
            repeat: false
            onTriggered: {
                videoGridView.checkVisibleItems();
            }
        }

        onMovingChanged: {
            if (!moving) {
                scrollStopTimer.restart();
            }
        }

        function checkVisibleItems() {
            var visibleIds = [];
            var invisibleIds = [];
            
            var cols = viewSizeColumns[viewSize];
            if (cols <= 0) {
                console.warn("[checkVisibleItems] 列数无效:", cols);
                return;
            }
            
            // 检查 GridView 是否已正确初始化
            if (cellHeight <= 0 || height <= 0) {
                console.warn("[checkVisibleItems] GridView 尺寸未初始化 - cellHeight:", cellHeight, "height:", height);
                return;
            }
            
            // 计算可见区域的行范围
            var startRow = Math.floor(contentY / cellHeight);
            var endRow = Math.ceil((contentY + height) / cellHeight);
            
            // 转换为索引范围
            var startIndex = startRow * cols;
            var endIndex = Math.min(endRow * cols, instanceConfigs.length); // 防止越界
            
            console.log("[checkVisibleItems] 检测参数 - contentY:", contentY, "height:", height, 
                       "cellHeight:", cellHeight, "cols:", cols);
            console.log("[checkVisibleItems] 可见行范围:", startRow, "-", endRow, 
                       "索引范围:", startIndex, "-", endIndex);
            
            for (var i = 0; i < instanceConfigs.length; i++) {
                var config = instanceConfigs[i];
                if (i >= startIndex && i < endIndex) {
                    visibleIds.push(config.instanceId);
                } else {
                    invisibleIds.push(config.instanceId);
                }
            }
            
            // 回调到C++并打印
            multiInstanceViewModel.onVisibilityChanged(visibleIds, invisibleIds);
        }
        
        delegate: Rectangle {
            id: videoCell
            width: videoGridView.cellWidth - 10
            height: videoGridView.cellHeight - 10
            border.color: "gray"
            border.width: 1
            color: "transparent"
            
            // 实例信息属性
            property string instanceId: modelData.instanceId
            property int instanceIndex: modelData.instanceIndex
            // 连接状态: 0=未连接, 1=连接中, 2=已连接
            property int connectionState: 0
            property bool isConnected: connectionState === 2
            
            // 初始化时获取状态
            Component.onCompleted: {
                connectionState = multiInstanceViewModel.getInstanceConnectionState(instanceId)
            }
            
            // 监听连接状态变化
            Connections {
                target: multiInstanceViewModel
                function onInstanceConnectionChanged(changedInstanceId, connected) {
                    if (changedInstanceId === videoCell.instanceId) {
                        videoCell.connectionState = multiInstanceViewModel.getInstanceConnectionState(videoCell.instanceId)
                    }
                }
                function onConnectedInstanceIdsChanged() {
                    videoCell.connectionState = multiInstanceViewModel.getInstanceConnectionState(videoCell.instanceId)
                }
            }
            
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
                objectName: "videoRenderItem_" + videoCell.instanceId
                
                // 居中显示
                anchors.centerIn: parent
                
                // 根据视频宽高比动态调整尺寸，保持宽高比
                width: {
                    if (videoWidth <= 0 || videoHeight <= 0) {
                        return parent.width;
                    }
                    var aspectRatio = videoWidth / videoHeight;
                    var parentAspectRatio = parent.width / parent.height;
                    
                    if (aspectRatio > parentAspectRatio) {
                        // 视频更宽，以宽度为准
                        return parent.width;
                    } else {
                        // 视频更高，以高度为准
                        return parent.height * aspectRatio;
                    }
                }
                
                height: {
                    if (videoWidth <= 0 || videoHeight <= 0) {
                        return parent.height;
                    }
                    var aspectRatio = videoWidth / videoHeight;
                    var parentAspectRatio = parent.width / parent.height;
                    
                    if (aspectRatio > parentAspectRatio) {
                        // 视频更宽，以宽度为准
                        return parent.width / aspectRatio;
                    } else {
                        // 视频更高，以高度为准
                        return parent.height;
                    }
                }
                
                Component.onCompleted: {
                    console.log("Registering VideoRenderItem for instance:", 
                               videoCell.instanceId)
                    multiInstanceViewModel.registerVideoRenderItem(
                        videoCell.instanceId,
                        videoRenderItem
                    )
                }
            }
            
            // 统计数据显示蒙层
            Components.StatsOverlay {
                id: statsOverlay
                anchors.top: videoRenderItem.top
                anchors.left: videoRenderItem.left
                anchors.right: videoRenderItem.right
                clientStats: ""
                // 只有在开关打开且有统计数据时才显示
                visible: showStatsOverlay && statsOverlay.clientStats !== ""
                
                // 监听统计数据更新
                Connections {
                    target: multiInstanceViewModel
                    function onClientStatsChanged() {
                        var newStats = multiInstanceViewModel.getInstanceStats(videoCell.instanceId);
                        if (newStats && newStats.length > 0) {
                            statsOverlay.clientStats = newStats;
                        }
                    }
                }
                
                // 初始化时获取一次数据
                Component.onCompleted: {
                    var initialStats = multiInstanceViewModel.getInstanceStats(videoCell.instanceId);
                    if (initialStats && initialStats.length > 0) {
                        statsOverlay.clientStats = initialStats;
                    }
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
                    // 如果正在批量更新，忽略此事件
                    if (isUpdatingCheckboxes) {
                        return;
                    }
                    
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
                        text: {
                            switch(videoCell.connectionState) {
                                case 0: return "未连接";
                                case 1: return "连接中...";
                                case 2: return "已连接";
                                default: return "未知";
                            }
                        }
                        color: {
                            switch(videoCell.connectionState) {
                                case 0: return "red";
                                case 1: return "yellow";
                                case 2: return "lightgreen";
                                default: return "gray";
                            }
                        }
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
            
            Text {
                text: {
                    try {
                        if (multiInstanceViewModel.clientStats) {
                            var stats = JSON.parse(multiInstanceViewModel.clientStats);
                            var fps = stats.fps || 0;
                            var bitrate = stats.bitrate || 0;
                            var rtt = stats.rtt || 0;
                            return "FPS: " + fps + " | 码率: " + (bitrate/1000).toFixed(1) + "Mbps | 延迟: " + rtt + "ms";
                        }
                    } catch(e) {
                        // JSON 解析失败，忽略
                    }
                    return "统计数据: --";
                }
                font.pixelSize: 12
                color: "#666666"
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
        
        property var instanceIds: []
        property string reason: ""
        
        contentItem: Column {
            spacing: 15
            padding: 20
            
            Text {
                text: "会话已断开连接"
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
                height: Math.min(instanceListText.contentHeight + 20, 200)
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
                        // 计算每页可见的实例数量
                        var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
                        multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
                    }
                }
            }
        }
    }
}
