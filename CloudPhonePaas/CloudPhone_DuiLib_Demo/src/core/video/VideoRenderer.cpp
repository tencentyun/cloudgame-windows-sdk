#include "VideoRenderer.h"

#include <cmath>
#include <libyuv/rotate_argb.h>

#include "utils/Logger.h"

VideoRenderer::VideoRenderer(HWND hwnd) : m_hwnd(hwnd) {}

VideoRenderer::~VideoRenderer() = default;

void VideoRenderer::setRotationAngle(double angle) {
    m_rotationAngle = angle;
}

void VideoRenderer::setRenderScale(double scale) {
    // Clamp to 0.1x ~ 3.0x to avoid extreme values
    double old = m_renderScale;
    m_renderScale = std::max(0.1, std::min(scale, 3.0));
    Logger::info("[VideoRenderer] render scale changed: " + std::to_string(old) + " -> " + std::to_string(m_renderScale));
}

void VideoRenderer::ensureBITMAPINFO(int width, int height) {
    if (width == m_lastWidth && height == m_lastHeight) return;

    m_lastWidth = width;
    m_lastHeight = height;

    memset(&m_bmi, 0, sizeof(m_bmi));
    m_bmi.bmiHeader.biSize = sizeof(BITMAPINFOHEADER);
    m_bmi.bmiHeader.biWidth = width;
    m_bmi.bmiHeader.biHeight = -height;  // top-down DIB
    m_bmi.bmiHeader.biPlanes = 1;
    m_bmi.bmiHeader.biBitCount = 32;
    m_bmi.bmiHeader.biCompression = BI_RGB;
}

void VideoRenderer::renderFrame(TcrVideoFrameHandle frameHandle, int videoWidth, int videoHeight) {
    if (!m_hwnd || !frameHandle || videoWidth <= 0 || videoHeight <= 0) return;

    // 1. Convert I420 to ARGB via TcrSdk (with optional render scale)
    int outW = 0;
    int outH = 0;
    if (m_renderScale != 1.0) {
        outW = static_cast<int>(videoWidth * m_renderScale);
        outH = static_cast<int>(videoHeight * m_renderScale);
    }

    TcrBGRABuffer* buf = tcr_video_frame_convert_to_argb(frameHandle, outW, outH);
    if (!buf) {
        Logger::error("[VideoRenderer] tcr_video_frame_convert_to_argb returned null");
        return;
    }

    int angle = static_cast<int>(std::round(m_rotationAngle)) % 360;
    if (angle < 0) angle += 360;

    int renderW = buf->width;
    int renderH = buf->height;
    const uint8_t* renderData = buf->data;
    int renderStride = buf->stride;

    // 2. Optional rotation on ARGB data via libyuv
    if (angle != 0) {
        libyuv::RotationMode rotMode;
        if (angle == 90) {
            rotMode = libyuv::kRotate90;
            renderW = buf->height;
            renderH = buf->width;
        } else if (angle == 180) {
            rotMode = libyuv::kRotate180;
        } else if (angle == 270) {
            rotMode = libyuv::kRotate270;
            renderW = buf->height;
            renderH = buf->width;
        } else {
            rotMode = libyuv::kRotate0;
        }

        m_argbBuffer.resize(static_cast<size_t>(renderW) * renderH * 4);
        libyuv::ARGBRotate(buf->data, buf->stride,
                          m_argbBuffer.data(), renderW * 4,
                          buf->width, buf->height, rotMode);

        renderData = m_argbBuffer.data();
        renderStride = renderW * 4;
    }

    // 3. Setup BITMAPINFO for StretchDIBits
    ensureBITMAPINFO(renderW, renderH);

    // 4. Blit to window
    if (!::IsWindow(m_hwnd)) {
        Logger::error("[VideoRenderer] m_hwnd is not a valid window");
        tcr_bgra_buffer_free(buf);
        return;
    }

    RECT rc;
    ::GetClientRect(m_hwnd, &rc);
    int dstW = rc.right - rc.left;
    int dstH = rc.bottom - rc.top;

    if (dstW <= 0 || dstH <= 0) {
        tcr_bgra_buffer_free(buf);
        return;
    }

    HDC hdc = ::GetDC(m_hwnd);
    if (hdc) {
        ::SetStretchBltMode(hdc, COLORONCOLOR);
        ::StretchDIBits(hdc,
                        0, 0, dstW, dstH,
                        0, 0, renderW, renderH,
                        renderData,
                        &m_bmi,
                        DIB_RGB_COLORS,
                        SRCCOPY);
        ::ReleaseDC(m_hwnd, hdc);
    }

    tcr_bgra_buffer_free(buf);
}
