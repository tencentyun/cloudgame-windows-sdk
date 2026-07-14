#pragma once

#include <string>

/**
 * @brief Video transform helper for rotation and coordinate conversion.
 *
 * Ported from the Qt Quick Demo's VideoTransformHelper.
 *
 * Handles:
 * 1. Rotation mapping: cloud degree -> client rotation angle
 * 2. Touch coordinate transformation: client view coords -> cloud screen coords
 *
 * Rotation mapping:
 *   Cloud 0°   -> Client 0°   (no rotation)
 *   Cloud 90°  -> Client 270° (CCW 90°)
 *   Cloud 180° -> Client 180°
 *   Cloud 270° -> Client 90°  (CW 90°)
 */
class VideoTransformHelper {
public:
    VideoTransformHelper() = default;

    /// Set rotation angle and cloud screen dimensions
    void setRotation(double angle, int videoWidth, int videoHeight);

    double rotationAngle() const { return m_rotationAngle; }
    int videoWidth() const { return m_videoWidth; }
    int videoHeight() const { return m_videoHeight; }

    /**
     * @brief Transform touch coordinates from client view to cloud screen coords.
     *
     * @param x         Client view X
     * @param y         Client view Y
     * @param viewWidth Client view width
     * @param viewHeight Client view height
     * @param outX      Cloud screen X (output)
     * @param outY      Cloud screen Y (output)
     * @param outW      Cloud screen width (output, for SDK)
     * @param outH      Cloud screen height (output, for SDK)
     */
    void transformTouchCoordinates(int x, int y, int viewWidth, int viewHeight,
                                   int& outX, int& outY, int& outW, int& outH) const;

    /// Map cloud rotation degree string (e.g. "90_degree") to client rotation angle
    static double mapCloudRotationToClient(const std::string& cloudDegree);

private:
    double m_rotationAngle = 0.0;
    int m_videoWidth = 0;   // Cloud screen physical width (doesn't change with rotation)
    int m_videoHeight = 0;  // Cloud screen physical height
};
