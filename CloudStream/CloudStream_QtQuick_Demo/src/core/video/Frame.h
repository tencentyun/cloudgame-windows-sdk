#pragma once

#include <QVector>
#include <QMetaType>
#include <QSharedPointer>
#include "tcr_types.h"
#include "tcr_c_api.h"

/**
 * @brief 视频帧类型枚举
 * 
 * 标识视频帧数据的存储格式和位置
 */
enum class VideoFrameType {
    I420_CPU = 0,    ///< I420(YUV420P)格式，数据位于CPU内存
    D3D11_GPU = 1    ///< D3D11纹理格式，数据位于GPU显存
};

/**
 * @brief D3D11纹理数据结构
 * 
 * 存储GPU纹理相关信息，用于硬件加速渲染
 */
struct D3D11TextureData {
    void* texture = nullptr;      ///< ID3D11Texture2D指针
    void* device = nullptr;       ///< ID3D11Device指针
    int array_index = 0;          ///< 纹理数组索引
    int format = 0;               ///< 纹理格式(DXGI_FORMAT)
};

/**
 * @brief 视频帧数据结构体
 * 
 * 统一的视频帧数据结构，支持多种帧类型：
 * - I420_CPU: YUV420格式的CPU内存数据
 * - D3D11_GPU: D3D11格式的GPU纹理数据
 * 
 * 使用引用计数管理底层帧资源的生命周期。
 */
struct VideoFrameData {
    // 帧类型标识
    VideoFrameType frame_type = VideoFrameType::I420_CPU;  ///< 帧数据类型
    
    // YUV数据指针（仅当frame_type为I420_CPU时有效）
    const uint8_t* data_y = nullptr;  ///< Y分量数据指针
    const uint8_t* data_u = nullptr;  ///< U分量数据指针
    const uint8_t* data_v = nullptr;  ///< V分量数据指针
    
    // 步长信息（仅当frame_type为I420_CPU时有效）
    int strideY = 0;                  ///< Y分量每行字节数
    int strideU = 0;                  ///< U分量每行字节数
    int strideV = 0;                  ///< V分量每行字节数
    
    // D3D11纹理数据（仅当frame_type为D3D11_GPU时有效）
    D3D11TextureData d3d11_data;      ///< D3D11纹理信息
    
    // 帧资源管理
    void* frame_handle = nullptr;     ///< 底层帧句柄，用于引用计数管理
    
    // 帧尺寸信息（所有类型通用）
    int width = 0;                    ///< 帧宽度（像素）
    int height = 0;                   ///< 帧高度（像素）
    int64_t timestamp_us = 0;         ///< 时间戳（微秒）
    
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