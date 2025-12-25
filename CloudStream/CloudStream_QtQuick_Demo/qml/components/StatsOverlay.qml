import QtQuick 2.15

import QtQuick 2.15
import QtQuick.Controls 2.15

// 统计数据显示蒙层组件
Rectangle {
    id: root
    
    // 对外暴露的属性
    property string clientStats: ""
    
    color: "#80000000"
    
    // 高度自适应内容
    width: parent.width
    height: statsText.contentHeight + 10
    
    Text {
        id: statsText
        anchors.fill: parent
        anchors.margins: 5
        color: "white"
        font.pixelSize: 12
        font.family: "Courier New"
        wrapMode: Text.WordWrap
        text: {
            // 检查输入是否为空
            if (!root.clientStats || root.clientStats === "") {
                return ""
            }
            
            try {
                // 验证是否为有效的 JSON
                var stats = JSON.parse(root.clientStats)

                // 直接返回格式化的 JSON 字符串
                // C++ 端已经返回了格式化的 JSON（带缩进）
                return root.clientStats
                
            } catch (e) {
                console.error("[StatsOverlay] JSON 解析错误:", e.toString())
                console.error("[StatsOverlay] 原始数据:", root.clientStats)
                return "数据解析错误"
            }
        }
    }
}
