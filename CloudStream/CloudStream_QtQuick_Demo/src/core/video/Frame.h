#pragma once

#include <QVector>
#include <QMetaType>
#include <QSharedPointer>

// 视频帧数据结构体，存储YUV分量及其步长和尺寸信息
struct VideoFrameData {
    QVector<uint8_t> y;   // Y分量数据
    QVector<uint8_t> u;   // U分量数据
    QVector<uint8_t> v;   // V分量数据
    int strideY = 0;      // Y分量每行字节数
    int strideU = 0;      // U分量每行字节数
    int strideV = 0;      // V分量每行字节数
    int width = 0;        // 帧宽度
    int height = 0;       // 帧高度
};

// 视频帧数据的智能指针类型，便于内存管理和跨线程传递
using VideoFrameDataPtr = QSharedPointer<VideoFrameData>;

// Qt元类型声明，便于在信号槽等机制中使用
Q_DECLARE_METATYPE(VideoFrameData)
Q_DECLARE_METATYPE(VideoFrameDataPtr)