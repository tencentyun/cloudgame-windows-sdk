#pragma once

#include <QVector>
#include <QMetaType>
#include <QSharedPointer>
#include "tcr_types.h"
#include "tcr_c_api.h"

/**
 * @brief 视频帧数据结构体
 * 
 * 存储YUV格式的视频帧数据，包含Y、U、V三个分量的数据指针和步长信息。
 * 使用引用计数管理底层帧资源的生命周期。
 */
struct VideoFrameData {
    // YUV数据指针（只读引用，不拥有所有权）
    const uint8_t* data_y = nullptr;  ///< Y分量数据指针
    const uint8_t* data_u = nullptr;  ///< U分量数据指针
    const uint8_t* data_v = nullptr;  ///< V分量数据指针
    
    // 帧资源管理
    void* frame_handle = nullptr;     ///< 底层帧句柄，用于引用计数管理
    
    // 步长信息（每行字节数，可能大于实际宽度）
    int strideY = 0;                  ///< Y分量每行字节数
    int strideU = 0;                  ///< U分量每行字节数
    int strideV = 0;                  ///< V分量每行字节数
    
    // 帧尺寸信息
    int width = 0;                    ///< 帧宽度（像素）
    int height = 0;                   ///< 帧高度（像素）
    
    /**
     * @brief 析构函数
     * 
     * 释放底层帧资源的引用计数，确保资源正确释放。
     */
    ~VideoFrameData() {
        if (frame_handle) {
            tcr_video_frame_release(static_cast<TcrVideoFrameHandle>(frame_handle));
        }
    }
};

/**
 * @brief 视频帧数据的智能指针类型
 * 
 * 使用QSharedPointer进行自动内存管理，支持跨线程安全传递。
 */
using VideoFrameDataPtr = QSharedPointer<VideoFrameData>;

// Qt元类型声明，使类型可用于信号槽机制
Q_DECLARE_METATYPE(VideoFrameData)
Q_DECLARE_METATYPE(VideoFrameDataPtr)