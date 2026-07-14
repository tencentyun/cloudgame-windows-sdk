#pragma once

#include <cstdint>
#include <vector>

#define NOMINMAX
#include <windows.h>

#include "tcr_c_api.h"

/**
 * @brief GDI-based video renderer with rotation support.
 *
 * Uses TcrSdk's tcr_video_frame_convert_to_argb for I420→ARGB conversion,
 * and libyuv::ARGBRotate for optional rotation on ARGB data.
 * Final rendering to the target HWND uses StretchDIBits.
 */
class VideoRenderer {
public:
    explicit VideoRenderer(HWND hwnd);
    ~VideoRenderer();

    VideoRenderer(const VideoRenderer&) = delete;
    VideoRenderer& operator=(const VideoRenderer&) = delete;

    /**
     * @brief Set the rotation angle for rendering.
     * @param angle Rotation angle: 0, 90, 180, or 270 degrees.
     */
    void setRotationAngle(double angle);

    /**
     * @brief Set the render scale factor for SDK-side scaling.
     *
     * @param scale Scale factor, e.g. 0.5 = half resolution, 1.0 = original, 2.0 = double.
     *              A value of 1.0 (default) passes (0, 0) to the SDK to keep original size.
     */
    void setRenderScale(double scale);

    /**
     * @brief Render a video frame to the target window.
     *
     * @param frameHandle  TcrSdk video frame handle (must be valid at call time).
     * @param videoWidth   Original cloud screen width (used for scale calculation).
     * @param videoHeight  Original cloud screen height (used for scale calculation).
     */
    void renderFrame(TcrVideoFrameHandle frameHandle, int videoWidth, int videoHeight);

private:
    HWND m_hwnd = nullptr;
    double m_rotationAngle = 0.0;
    double m_renderScale = 1.0;

    // ARGB buffer used only as rotation target (when rotation angle != 0).
    // In the no-rotation case, the TcrSdk buffer is used directly.
    std::vector<uint8_t> m_argbBuffer;
    BITMAPINFO m_bmi = {};
    int m_lastWidth = 0;
    int m_lastHeight = 0;

    void ensureBITMAPINFO(int width, int height);
};
