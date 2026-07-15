#include "VideoTransformHelper.h"

#include <cmath>
#include <string>

void VideoTransformHelper::setRotation(double angle, int videoWidth, int videoHeight) {
    m_rotationAngle = angle;
    m_videoWidth = videoWidth;
    m_videoHeight = videoHeight;
}

double VideoTransformHelper::mapCloudRotationToClient(const std::string& cloudDegree) {
    // Cloud phone degree indicates its screen rotation direction.
    // Client needs reverse rotation to display correctly.
    //   Cloud 0°   -> Client 0°
    //   Cloud 90°  -> Client 270°
    //   Cloud 180° -> Client 180°
    //   Cloud 270° -> Client 90°
    if (cloudDegree.find("90") != std::string::npos) {
        return 270.0;
    } else if (cloudDegree.find("180") != std::string::npos) {
        return 180.0;
    } else if (cloudDegree.find("270") != std::string::npos) {
        return 90.0;
    }
    return 0.0;
}

void VideoTransformHelper::transformTouchCoordinates(int x, int y, int viewWidth, int viewHeight,
                                                     int& outX, int& outY, int& outW, int& outH) const {
    outW = m_videoWidth;
    outH = m_videoHeight;

    if (m_videoWidth <= 0 || m_videoHeight <= 0 || viewWidth <= 0 || viewHeight <= 0) {
        outX = x;
        outY = y;
        return;
    }

    int angle = static_cast<int>(std::round(m_rotationAngle)) % 360;

    switch (angle) {
    case 0:
        // No rotation: direct proportional mapping
        outX = x * m_videoWidth / viewWidth;
        outY = y * m_videoHeight / viewHeight;
        break;
    case 90:
        // Client rotated CW 90° (cloud rotated CCW 90°, i.e. 270°)
        // view x maps to cloud y, view y (reversed) maps to cloud x
        outX = y * m_videoWidth / viewHeight;
        outY = (viewWidth - x) * m_videoHeight / viewWidth;
        break;
    case 180:
        // Rotated 180°
        outX = (viewWidth - x) * m_videoWidth / viewWidth;
        outY = (viewHeight - y) * m_videoHeight / viewHeight;
        break;
    case 270:
        // Client rotated CCW 90° (cloud rotated CW 90°, i.e. 90°)
        // view y (reversed) maps to cloud x, view x maps to cloud y
        outX = (viewHeight - y) * m_videoWidth / viewHeight;
        outY = x * m_videoHeight / viewWidth;
        break;
    default:
        outX = x;
        outY = y;
        break;
    }
}
