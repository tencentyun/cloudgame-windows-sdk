#include "video_renderer.h"
#include <algorithm>
//
// VideoRenderer
//

VideoRenderer::VideoRenderer(
    HWND wnd,
    int width,
    int height) : wnd_(wnd) {
    ::InitializeCriticalSection(&buffer_lock_);
    ZeroMemory(&bmi_, sizeof(bmi_));
    bmi_.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    bmi_.bmiHeader.biPlanes = 1;
    bmi_.bmiHeader.biBitCount = 32;
    bmi_.bmiHeader.biCompression = BI_RGB;
    bmi_.bmiHeader.biWidth = width;
    bmi_.bmiHeader.biHeight = -height;
    bmi_.bmiHeader.biSizeImage =
        width * height * (bmi_.bmiHeader.biBitCount >> 3);
}

VideoRenderer::~VideoRenderer() {
    ::DeleteCriticalSection(&buffer_lock_);
}

void VideoRenderer::I420ToARGB(const uint8_t* src_y,
    int src_stride_y,
    const uint8_t* src_u,
    int src_stride_u,
    const uint8_t* src_v,
    int src_stride_v,
    uint8_t* dst_argb,
    int dst_stride_argb,
    int width,
    int height) {
    const int uv_width = (width + 1) / 2;
    const int uv_height = (height + 1) / 2;

    for (int y = 0; y < height; ++y) {
        uint8_t* dst = dst_argb + y * dst_stride_argb;
        const uint8_t* src_y_row = src_y + y * src_stride_y;
        const uint8_t* src_u_row = src_u + (y / 2) * src_stride_u;
        const uint8_t* src_v_row = src_v + (y / 2) * src_stride_v;

        for (int x = 0; x < width; ++x) {
            const int uv_x = x / 2;
            const int uv_y = y / 2;
            const uint8_t y_val = src_y_row[x];
            const uint8_t u_val = src_u_row[uv_x];
            const uint8_t v_val = src_v_row[uv_x];

            const int c_val = y_val - 16;
            const int d_val = u_val - 128;
            const int e_val = v_val - 128;

            const uint8_t r_val = std::max(0, std::min(255, (298 * c_val + 409 * e_val + 128) >> 8));
            const uint8_t g_val =
                std::max(0, std::min(255, (298 * c_val - 100 * d_val - 208 * e_val + 128) >> 8));
            const uint8_t b_val = std::max(0, std::min(255, (298 * c_val + 516 * d_val + 128) >> 8));

            dst[4 * x + 0] = b_val;
            dst[4 * x + 1] = g_val;
            dst[4 * x + 2] = r_val;
            dst[4 * x + 3] = 0xff;
        }
    }
}

void VideoRenderer::SetSize(int width, int height) {
    AutoLock<VideoRenderer> lock(this);

    if (width == bmi_.bmiHeader.biWidth && height == bmi_.bmiHeader.biHeight) {
        return;
    }

    bmi_.bmiHeader.biWidth = width;
    bmi_.bmiHeader.biHeight = -height;
    bmi_.bmiHeader.biSizeImage =
        width * height * (bmi_.bmiHeader.biBitCount >> 3);
    image_.reset(new uint8_t[bmi_.bmiHeader.biSizeImage]);
}

void VideoRenderer::OnFrame(const tcrsdk::VideoFrame& video_frame) {
    {
        AutoLock<VideoRenderer> lock(this);

        std::shared_ptr<tcrsdk::I420Buffer> buffer = video_frame.i420_buffer();

        SetSize(buffer->width(), buffer->height());

        I420ToARGB(buffer->DataY(), buffer->StrideY(), buffer->DataU(),
            buffer->StrideU(), buffer->DataV(), buffer->StrideV(),
            image_.get(),
            bmi_.bmiHeader.biWidth * bmi_.bmiHeader.biBitCount / 8,
            buffer->width(), buffer->height());

    }
    InvalidateRect(wnd_, NULL, TRUE);
}
