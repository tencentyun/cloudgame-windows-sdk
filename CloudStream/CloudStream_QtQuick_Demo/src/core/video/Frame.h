#pragma once

#include <QMetaType>
#include <QSharedPointer>
#include <QVector>

#include "tcr_c_api.h"
#include "tcr_types.h"

/**
 * @brief 视频帧类型枚举
 *
 * 标识视频帧数据的存储格式和位置
 */
enum class VideoFrameType {
  I420_CPU = 0,  ///< I420(YUV420P)格式，数据位于CPU内存
};

/**
 * @brief 视频帧数据结构体
 *
 * 统一的视频帧数据结构，支持 I420_CPU（YUV420格式的CPU内存数据）。
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
  int strideY = 0;  ///< Y分量每行字节数
  int strideU = 0;  ///< U分量每行字节数
  int strideV = 0;  ///< V分量每行字节数

  // 帧资源管理
  void* frame_handle = nullptr;  ///< 底层帧句柄，用于引用计数管理

  // 帧尺寸信息（所有类型通用）
  int width = 0;             ///< 帧宽度（像素）
  int height = 0;            ///< 帧高度（像素）
  int64_t timestamp_us = 0;  ///< 时间戳（微秒）

  /**
   * @brief I420格式构造函数
   *
   * @param handle 帧句柄
   * @param y Y分量数据指针
   * @param u U分量数据指针
   * @param v V分量数据指针
   * @param stride_y Y分量步长
   * @param stride_u U分量步长
   * @param stride_v V分量步长
   * @param w 帧宽度
   * @param h 帧高度
   * @param timestamp 时间戳（微秒）
   */
  VideoFrameData(void* handle, const uint8_t* y, const uint8_t* u, const uint8_t* v, int stride_y, int stride_u,
                 int stride_v, int w, int h, int64_t timestamp);

  /**
   * @brief 析构函数
   *
   * 释放底层帧资源的引用计数，确保资源正确释放。
   */
  ~VideoFrameData();
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