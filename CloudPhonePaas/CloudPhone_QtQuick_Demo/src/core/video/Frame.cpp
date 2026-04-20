#include "Frame.h"

#include <cstring>
#include <QRectF>
#include <QSGGeometry>

#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"
#include "YuvMaterial.h"
#include "YuvNode.h"

VideoFrameData::VideoFrameData(void* handle, const uint8_t* y, const uint8_t* u, const uint8_t* v, int stride_y,
                               int stride_u, int stride_v, int w, int h, int64_t timestamp)
    : frame_type(VideoFrameType::I420_CPU),
      data_y(y),
      data_u(u),
      data_v(v),
      strideY(stride_y),
      strideU(stride_u),
      strideV(stride_v),
      frame_handle(handle),
      width(w),
      height(h),
      timestamp_us(timestamp) {}

VideoFrameData::~VideoFrameData() {
  if (frame_handle) {
    // 释放帧引用
    tcr_video_frame_release(static_cast<TcrVideoFrameHandle>(frame_handle));
  }
}
