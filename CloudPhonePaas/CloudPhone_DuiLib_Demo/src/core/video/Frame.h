#pragma once

#include <cstdint>
#include <cstring>
#include <memory>
#include <vector>

#include "tcr_c_api.h"

/**
 * @brief Video frame data holding copied I420 pixel data.
 *
 * The SDK's video frame buffer is only valid during the callback.
 * We copy the YUV planes into owned memory so the frame can be
 * safely passed to the UI thread for rendering.
 */
struct VideoFrameData {
    std::vector<uint8_t> dataY;
    std::vector<uint8_t> dataU;
    std::vector<uint8_t> dataV;

    int strideY = 0;
    int strideU = 0;
    int strideV = 0;
    int width = 0;
    int height = 0;
    int64_t timestamp_us = 0;

    /// TcrSdk frame handle — we add_ref in the callback and release here
    TcrVideoFrameHandle frameHandle = nullptr;

    VideoFrameData() = default;

    VideoFrameData(TcrVideoFrameHandle handle,
                   const uint8_t* y, const uint8_t* u, const uint8_t* v,
                   int stride_y, int stride_u, int stride_v,
                   int w, int h, int64_t timestamp)
        : strideY(stride_y), strideU(stride_u), strideV(stride_v),
          width(w), height(h), timestamp_us(timestamp), frameHandle(handle) {
        // Copy Y plane
        size_t sizeY = static_cast<size_t>(stride_y) * h;
        dataY.resize(sizeY);
        std::memcpy(dataY.data(), y, sizeY);

        // Copy U plane (half height)
        int halfH = (h + 1) / 2;
        size_t sizeU = static_cast<size_t>(stride_u) * halfH;
        dataU.resize(sizeU);
        std::memcpy(dataU.data(), u, sizeU);

        // Copy V plane (half height)
        size_t sizeV = static_cast<size_t>(stride_v) * halfH;
        dataV.resize(sizeV);
        std::memcpy(dataV.data(), v, sizeV);
    }

    ~VideoFrameData() {
        if (frameHandle) {
            tcr_video_frame_release(frameHandle);
            frameHandle = nullptr;
        }
    }

    // Non-copyable
    VideoFrameData(const VideoFrameData&) = delete;
    VideoFrameData& operator=(const VideoFrameData&) = delete;

    // Movable
    VideoFrameData(VideoFrameData&& other) noexcept
        : dataY(std::move(other.dataY)), dataU(std::move(other.dataU)),
          dataV(std::move(other.dataV)), strideY(other.strideY),
          strideU(other.strideU), strideV(other.strideV),
          width(other.width), height(other.height),
          timestamp_us(other.timestamp_us), frameHandle(other.frameHandle) {
        other.frameHandle = nullptr;
    }

    VideoFrameData& operator=(VideoFrameData&& other) noexcept {
        if (this != &other) {
            if (frameHandle) tcr_video_frame_release(frameHandle);
            dataY = std::move(other.dataY);
            dataU = std::move(other.dataU);
            dataV = std::move(other.dataV);
            strideY = other.strideY;
            strideU = other.strideU;
            strideV = other.strideV;
            width = other.width;
            height = other.height;
            timestamp_us = other.timestamp_us;
            frameHandle = other.frameHandle;
            other.frameHandle = nullptr;
        }
        return *this;
    }
};

using VideoFrameDataPtr = std::shared_ptr<VideoFrameData>;
