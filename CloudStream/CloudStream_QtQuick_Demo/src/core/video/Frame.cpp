#include "YuvNode.h"
#include "YuvMaterial.h"
#include "Frame.h"
#include "utils/Logger.h"
#include <QSGGeometry>
#include <QRectF>
#include <cstring>
#include "tcr_c_api.h"
#include "tcr_types.h"

VideoFrameData::VideoFrameData(void* handle,
                               const uint8_t* y, const uint8_t* u, const uint8_t* v,
                               int stride_y, int stride_u, int stride_v,
                               int w, int h, int64_t timestamp)
    : frame_type(VideoFrameType::I420_CPU)
    , data_y(y)
    , data_u(u)
    , data_v(v)
    , strideY(stride_y)
    , strideU(stride_u)
    , strideV(stride_v)
    , frame_handle(handle)
    , width(w)
    , height(h)
    , timestamp_us(timestamp)
{}

#ifdef _WIN32
VideoFrameData::VideoFrameData(void* handle,
                               const D3D11TextureData& texture_data,
                               int w, int h, int64_t timestamp)
    : frame_type(VideoFrameType::D3D11_GPU)
    , d3d11_data(texture_data)
    , frame_handle(handle)
    , width(w)
    , height(h)
    , timestamp_us(timestamp)
{}
#endif

VideoFrameData::~VideoFrameData() {
    if (frame_handle) {
        // 释放帧引用
        tcr_video_frame_release(static_cast<TcrVideoFrameHandle>(frame_handle));
    }
}
