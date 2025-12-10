import QtQuick 2.15

// 统计数据显示蒙层组件
Rectangle {
    id: root
    
    // 对外暴露的属性
    property string clientStats: ""
    
    // 自动调整高度
    height: statsText.contentHeight + 20
    color: "#80000000"
    visible: clientStats !== ""
    
    Text {
        id: statsText
        anchors.fill: parent
        anchors.margins: 10
        color: "white"
        font.pixelSize: 12
        wrapMode: Text.WordWrap
        text: {
            if (root.clientStats === "") return ""
            
            try {
                var stats = JSON.parse(root.clientStats)
                var bitrateKbps = ((stats.bitrate || 0) / 1000).toFixed(2)
                return "FPS: " + (stats.fps || 0) + 
                       " | 码率: " + bitrateKbps + " kbps" +
                       " | RTT: " + (stats.rtt || 0) + "ms" +
                       " | RAW RTT: " + (stats.raw_rtt || 0) + "ms" +
                       "\n分辨率: " + (stats.frame_recv_res || "N/A") +
                       " | NACK: " + (stats.nack_count || 0) +
                       "\n视频包 - 接收: " + (stats.video_packet_recv || 0) + 
                       " | 发送: " + (stats.video_packet_sent || 0) + 
                       " | 丢包: " + (stats.video_packet_lost || 0) +
                       "\n音频包 - 接收: " + (stats.audio_packet_recv || 0) + 
                       " | 发送: " + (stats.audio_packet_sent || 0) +
                       " | 丢包: " + (stats.audio_packet_lost || 0) +
                       "\n视频帧 - 接收: " + (stats.frame_recv || 0) + 
                       " | 解码: " + (stats.frame_decode || 0) +
                       " | 编码: " + (stats.frame_encoded || 0) + 
                       " | 发送: " + (stats.frame_sent || 0) + 
                       " | 丢帧: " + (stats.frame_drop || 0) +
                       "\nRequest ID: " + (stats.request_id || "N/A") +
                       "\nInstance IDs: " + (stats.instance_ids || "N/A") +
                       "\nCodec: " + (stats.codec || "N/A")
            } catch (e) {
                return "统计数据解析错误"
            }
        }
    }
}
