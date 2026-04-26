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
    property int viewSize: 2                        // 视图大小: 0=超小(40列),1=小(30列), 2=中(10列), 3=大(5列)
    property var viewSizeColumns: [40, 20, 10, 5]       // 对应每种视图的列数
    
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
    
    // 拖拽选择状态
    property bool isDragSelecting: false            // 是否处于拖拽选择模式
    property bool dragSelectTargetState: true       // 拖拽选择的目标状态（true=选中, false=取消）
    property var dragPreCheckedIds: []              // 拖拽开始前的选中快照
    property real dragStartX: 0                     // 拖拽框起始X（屏幕坐标）
    property real dragStartY: 0                     // 拖拽框起始Y（屏幕坐标）
    property real dragEndX: 0                       // 拖拽框结束X（屏幕坐标）
    property real dragEndY: 0                       // 拖拽框结束Y（屏幕坐标）
    
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
        }
    }

    // ============================================
    // 工具函数区域
    // ============================================
    
    /**
     * 计算实例配置
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
     */
    function calculateVisibleInstancesPerPage() {
        var cols = viewSizeColumns[viewSize];
        var cellW = (multiInstanceWindow.width - 20) / cols;
        var cellH = cellW * 240 / 135;
        var gridViewHeight = multiInstanceWindow.height - 100;
        var visibleRows = Math.ceil(gridViewHeight / cellH) + 1;
        var visibleInstances = visibleRows * cols;
        
        console.log("[calculateVisibleInstancesPerPage] 列数:", cols, "单元格高度:", cellH, 
                   "可见行数:", visibleRows, "每页可见实例数:", visibleInstances);
        
        return visibleInstances;
    }
    
    /**
     * 创建同步窗口（单例模式）
     */
    function createSyncWindow(targetInstanceIds) {
        if (!targetInstanceIds || targetInstanceIds.length === 0) {
            console.error("目标实例ID列表为空");
            return;
        }
        
        if (syncWindow) {
            syncWindow.show();
            syncWindow.raise && syncWindow.raise();
            syncWindow.requestActivate && syncWindow.requestActivate();
            if (syncViewModel) {
                syncViewModel.updateCheckedInstanceIds(targetInstanceIds);
            }
            return;
        }
        
        syncViewModel = Qt.createQmlObject(
            'import CustomComponents 1.0; StreamingViewModel {}', 
            multiInstanceWindow
        );
        syncViewModel.connectSession(targetInstanceIds, true);
        
        var window = createWindow(syncViewModel);
        if (!window) return;
        
        syncWindow = window;
        
        syncWindow.closing.connect(function() {
            console.log("同步窗口关闭");
            if (syncViewModel && syncViewModel.closeSession) {
                syncViewModel.closeSession();
            }
            if (syncViewModel) {
                syncViewModel.destroy();
                syncViewModel = null;
            }
            if (syncWindow) {
                syncWindow.destroy();
                syncWindow = null;
            }
        });
    }
    
    /**
     * 创建单实例窗口
     */
    function createSingleInstanceWindow(instanceId) {
        if (!instanceId) {
            console.error("实例ID为空");
            return;
        }
        
        if (singleInstanceWindows[instanceId]) {
            var existingWindow = singleInstanceWindows[instanceId];
            existingWindow.show();
            existingWindow.raise && existingWindow.raise();
            existingWindow.requestActivate && existingWindow.requestActivate();
            return;
        }
        
        var viewModel = Qt.createQmlObject(
            'import CustomComponents 1.0; StreamingViewModel {}', 
            multiInstanceWindow
        );
        viewModel.connectSession([instanceId], false);
        
        var window = createWindow(viewModel);
        if (!window) return;
        
        singleInstanceWindows[instanceId] = {
            window: window,
            viewModel: viewModel
        };
        
        window.closing.connect(function() {
            console.log("单实例窗口关闭:", instanceId);
            var windowInfo = singleInstanceWindows[instanceId];
            if (windowInfo) {
                if (windowInfo.viewModel && windowInfo.viewModel.closeSession) {
                    windowInfo.viewModel.closeSession();
                }
                if (windowInfo.viewModel) {
                    windowInfo.viewModel.destroy();
                }
                if (windowInfo.window) {
                    windowInfo.window.destroy();
                }
            }
            delete singleInstanceWindows[instanceId];
        });
    }
    
    /**
     * 创建窗口的通用方法
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
     * 同步更新同步窗口的选中实例列表
     */
    function syncToSyncWindow(previousCheckedIds) {
        if (syncViewModel && syncViewModel.updateCheckedInstanceIds) {
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
            checkedInstanceIds = [];
            isAllSelected = false;
        } else {
            checkedInstanceIds = instanceIds.slice();
            isAllSelected = true;
        }
        
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
        
        var idsText = checkedInstanceIds.join(",");
        multiInstanceViewModel.copyToClipboard(idsText);
        console.log("已复制实例ID:", idsText);
        copySuccessToast.show();
    }

    /**
     * 根据鼠标在覆盖层上的坐标，计算对应卡片的实例ID
     */
    function getInstanceIdAtPosition(mouseX, mouseY) {
        var cellW = videoGridView.cellWidth;
        var cellH = videoGridView.cellHeight;
        if (cellW <= 0 || cellH <= 0) return "";
        
        var contentX = mouseX;
        var contentY = mouseY + videoGridView.contentY;
        
        var col = Math.floor(contentX / cellW);
        var row = Math.floor(contentY / cellH);
        var cols = viewSizeColumns[viewSize];
        
        if (col < 0 || col >= cols) return "";
        
        var index = row * cols + col;
        if (index < 0 || index >= instanceConfigs.length) return "";
        
        return instanceConfigs[index].instanceId;
    }
    
    /**
     * 判断点击位置是否在卡片右上角的 CheckBox 区域
     */
    function isClickOnCheckBox(mouseX, mouseY) {
        var cellW = videoGridView.cellWidth;
        var cellH = videoGridView.cellHeight;
        if (cellW <= 0 || cellH <= 0) return false;
        
        var contentX = mouseX;
        var contentY = mouseY + videoGridView.contentY;
        
        // 点击在卡片内的局部坐标
        var localX = contentX % cellW;
        var localY = contentY % cellH;
        
        // CheckBox 在卡片右上角，大约 32x32 区域
        var cardW = cellW - 10;  // 卡片实际宽度 (cellWidth - 间距)
        return localX >= (cardW - 34) && localX <= cardW && localY <= 34;
    }
    
    /**
     * 检查指定卡片是否在拖拽选择框内（屏幕坐标比较）
     */
    function isCellInDragBox(cellIndex) {
        var cols = viewSizeColumns[viewSize];
        var cellW = videoGridView.cellWidth;
        var cellH = videoGridView.cellHeight;
        
        var row = Math.floor(cellIndex / cols);
        var col = cellIndex % cols;
        
        // 卡片的屏幕坐标
        var cellLeft = col * cellW;
        var cellTop = row * cellH - videoGridView.contentY;
        var cellRight = cellLeft + cellW;
        var cellBottom = cellTop + cellH;
        
        // 拖拽框的屏幕坐标
        var minX = Math.min(dragStartX, dragEndX);
        var maxX = Math.max(dragStartX, dragEndX);
        var minY = Math.min(dragStartY, dragEndY);
        var maxY = Math.max(dragStartY, dragEndY);
        
        // 矩形相交检测
        return !(cellRight <= minX || cellLeft >= maxX || cellBottom <= minY || cellTop >= maxY);
    }
    
    /**
     * 根据当前拖拽框重新计算选中状态（以快照为基础）
     */
    function applyDragSelectionToBox() {
        var newCheckedIds = dragPreCheckedIds.slice();
        
        for (var i = 0; i < instanceConfigs.length; i++) {
            var instanceId = instanceConfigs[i].instanceId;
            
            if (isCellInDragBox(i)) {
                if (dragSelectTargetState) {
                    if (newCheckedIds.indexOf(instanceId) === -1) {
                        newCheckedIds.push(instanceId);
                    }
                } else {
                    var idx = newCheckedIds.indexOf(instanceId);
                    if (idx !== -1) {
                        newCheckedIds.splice(idx, 1);
                    }
                }
            }
        }
        
        checkedInstanceIds = newCheckedIds;
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
            instanceConfigs = calculateInstanceConfigs(instanceIds);
            var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
            multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
            
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
                if (viewSize !== 2) {
                    showStatsOverlay = false;
                }
            }
        }
        
        CheckBox {
            id: statsOverlayCheckBox
            text: "显示统计数据"
            checked: showStatsOverlay
            enabled: viewSize === 2
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
                console.log("暂停音视频流，实例:", checkedInstanceIds);
                if (multiInstanceViewModel.pauseStreaming) {
                    multiInstanceViewModel.pauseStreaming(checkedInstanceIds);
                }
            }
        }

        Button {
            text: "恢复音视频流"
            enabled: checkedInstanceIds.length > 0
            onClicked: {
                console.log("恢复音视频流，实例:", checkedInstanceIds);
                if (multiInstanceViewModel.resumeStreaming) {
                    multiInstanceViewModel.resumeStreaming(checkedInstanceIds);
                }
            }
        }

        Item { Layout.fillWidth: true }

        Button {
            text: "重新连接"
            onClicked: {
                if (instanceIds && instanceIds.length > 0) {
                    var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
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
            
            if (cellHeight <= 0 || height <= 0) {
                console.warn("[checkVisibleItems] GridView 尺寸未初始化 - cellHeight:", cellHeight, "height:", height);
                return;
            }
            
            var startRow = Math.floor(contentY / cellHeight);
            var endRow = Math.ceil((contentY + height) / cellHeight);
            var startIndex = startRow * cols;
            var endIndex = Math.min(endRow * cols, instanceConfigs.length);
            
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
            
            multiInstanceViewModel.onVisibilityChanged(visibleIds, invisibleIds);
        }
        
        delegate: Rectangle {
            id: videoCell
            width: videoGridView.cellWidth - 10
            height: videoGridView.cellHeight - 10
            border.color: "gray"
            border.width: 1
            color: "transparent"
            
            property string instanceId: modelData.instanceId
            property int instanceIndex: modelData.instanceIndex
            property int connectionState: 0
            property bool isConnected: connectionState === 2
            
            Component.onCompleted: {
                connectionState = multiInstanceViewModel.getInstanceConnectionState(instanceId)
            }
            
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
            
            // 视频渲染项
            VideoRenderItem {
                id: videoRenderItem
                objectName: "videoRenderItem_" + videoCell.instanceId
                anchors.centerIn: parent
                
                width: {
                    if (videoWidth <= 0 || videoHeight <= 0) return parent.width;
                    var aspectRatio = videoWidth / videoHeight;
                    var parentAspectRatio = parent.width / parent.height;
                    return aspectRatio > parentAspectRatio ? parent.width : parent.height * aspectRatio;
                }
                
                height: {
                    if (videoWidth <= 0 || videoHeight <= 0) return parent.height;
                    var aspectRatio = videoWidth / videoHeight;
                    var parentAspectRatio = parent.width / parent.height;
                    return aspectRatio > parentAspectRatio ? parent.width / aspectRatio : parent.height;
                }
                
                Component.onCompleted: {
                    console.log("Registering VideoRenderItem for instance:", videoCell.instanceId)
                    multiInstanceViewModel.registerVideoRenderItem(videoCell.instanceId, videoRenderItem)
                }
            }
            
            // 统计数据显示蒙层
            Components.StatsOverlay {
                id: statsOverlay
                anchors.top: videoRenderItem.top
                anchors.left: videoRenderItem.left
                anchors.right: videoRenderItem.right
                clientStats: ""
                visible: showStatsOverlay && statsOverlay.clientStats !== ""
                
                Connections {
                    target: multiInstanceViewModel
                    function onClientStatsChanged() {
                        var newStats = multiInstanceViewModel.getInstanceStats(videoCell.instanceId);
                        if (newStats && newStats.length > 0) {
                            statsOverlay.clientStats = newStats;
                        }
                    }
                }
                
                Component.onCompleted: {
                    var initialStats = multiInstanceViewModel.getInstanceStats(videoCell.instanceId);
                    if (initialStats && initialStats.length > 0) {
                        statsOverlay.clientStats = initialStats;
                    }
                }
            }

            // 选择复选框（纯显示，点击由覆盖层统一处理）
            CheckBox { 
                id: instanceCheckBox
                anchors.right: parent.right
                anchors.top: parent.top
                anchors.margins: 2
                checked: checkedInstanceIds.indexOf(videoCell.instanceId) !== -1
                enabled: false
                opacity: 1.0
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
    
    // ============================================
    // 拖拽选择覆盖层
    // ============================================
    
    // 处理：长按拖拽批量选中 / 短按打开单实例窗口
    // 复选框点击由 CheckBox 内嵌的 MouseArea 自行处理，不经过此覆盖层
    MouseArea {
        id: dragSelectOverlay
        anchors.fill: videoGridView
        z: 1
        propagateComposedEvents: true
        
        property bool longPressTriggered: false
        property real pressStartX: 0
        property real pressStartY: 0
        
        // 长按 300ms 进入拖拽模式
        Timer {
            id: longPressTimer
            interval: 300
            repeat: false
            onTriggered: {
                dragSelectOverlay.longPressTriggered = true;
                isDragSelecting = true;
                dragPreCheckedIds = checkedInstanceIds.slice();
                dragStartX = dragSelectOverlay.pressStartX;
                dragStartY = dragSelectOverlay.pressStartY;
                dragEndX = dragStartX;
                dragEndY = dragStartY;
                
                // 根据起始卡片的当前状态决定拖拽方向
                var instanceId = getInstanceIdAtPosition(dragStartX, dragStartY);
                if (instanceId !== "") {
                    dragSelectTargetState = checkedInstanceIds.indexOf(instanceId) === -1;
                }
                
                applyDragSelectionToBox();
            }
        }
        
        onPressed: {
            longPressTriggered = false;
            pressStartX = mouse.x;
            pressStartY = mouse.y;
            longPressTimer.restart();
        }
        
        onPositionChanged: {
            if (isDragSelecting) {
                dragEndX = mouse.x;
                dragEndY = mouse.y;
                applyDragSelectionToBox();
            }
        }
        
        onReleased: {
            longPressTimer.stop();
            
            if (isDragSelecting) {
                // 拖拽结束
                isDragSelecting = false;
                syncToSyncWindow();
            } else if (!longPressTriggered) {
                // 短按
                var instanceId = getInstanceIdAtPosition(mouse.x, mouse.y);
                if (instanceId !== "") {
                    if (isClickOnCheckBox(mouse.x, mouse.y)) {
                        // 点击在复选框区域 — 切换勾选
                        var previousCheckedIds = checkedInstanceIds.slice();
                        var newList = checkedInstanceIds.slice();
                        var idx = newList.indexOf(instanceId);
                        if (idx !== -1) {
                            newList.splice(idx, 1);
                        } else {
                            newList.push(instanceId);
                        }
                        checkedInstanceIds = newList;
                        syncToSyncWindow(previousCheckedIds);
                    } else {
                        // 点击在其他区域 — 打开单实例窗口
                        createSingleInstanceWindow(instanceId);
                    }
                }
            }
            
            mouse.accepted = true;
            longPressTriggered = false;
            dragPreCheckedIds = [];
            dragStartX = 0;
            dragStartY = 0;
            dragEndX = 0;
            dragEndY = 0;
        }
        
        onCanceled: {
            longPressTimer.stop();
            if (isDragSelecting) {
                checkedInstanceIds = dragPreCheckedIds.slice();
            }
            isDragSelecting = false;
            longPressTriggered = false;
            dragPreCheckedIds = [];
            dragStartX = 0;
            dragStartY = 0;
            dragEndX = 0;
            dragEndY = 0;
        }
        
        onWheel: {
            var delta = wheel.angleDelta.y;
            videoGridView.contentY = Math.max(
                0, 
                Math.min(
                    videoGridView.contentHeight - videoGridView.height,
                    videoGridView.contentY - delta
                )
            );
            videoGridView.returnToBounds();
            scrollStopTimer.restart();
            wheel.accepted = true;
        }
    }
    
    // 拖拽选择框可视化
    Canvas {
        id: dragSelectBox
        anchors.fill: videoGridView
        z: 2
        visible: isDragSelecting
        
        onPaint: {
            var ctx = getContext("2d");
            ctx.clearRect(0, 0, width, height);
            
            if (!isDragSelecting) return;
            
            var minX = Math.min(dragStartX, dragEndX);
            var maxX = Math.max(dragStartX, dragEndX);
            var minY = Math.min(dragStartY, dragEndY);
            var maxY = Math.max(dragStartY, dragEndY);
            
            ctx.fillStyle = "rgba(100, 150, 255, 0.2)";
            ctx.fillRect(minX, minY, maxX - minX, maxY - minY);
            
            ctx.strokeStyle = "rgba(70, 130, 255, 0.8)";
            ctx.lineWidth = 2;
            ctx.strokeRect(minX, minY, maxX - minX, maxY - minY);
        }
    }
    
    // 拖拽坐标变化时刷新选择框绘制
    onDragEndXChanged: dragSelectBox.requestPaint()
    onDragEndYChanged: dragSelectBox.requestPaint()
    onIsDragSelectingChanged: dragSelectBox.requestPaint()
    
    // ============================================
    // 底部状态栏
    // ============================================
    
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
                    } catch(e) {}
                    return "统计数据: --";
                }
                font.pixelSize: 12
                color: "#666666"
            }
            
            Item { Layout.fillWidth: true }
        }
    }
    
    // ============================================
    // 对话框
    // ============================================
    
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
                    if (instanceIds && instanceIds.length > 0) {
                        var visibleInstancesPerPage = calculateVisibleInstancesPerPage();
                        multiInstanceViewModel.connectMultipleInstances(instanceIds, visibleInstancesPerPage);
                    }
                }
            }
        }
    }
}
