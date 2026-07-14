#pragma once

#include <atomic>
#include <cstdint>
#include <functional>
#include <memory>
#include <string>
#include <vector>

#include <windows.h>

#include "core/video/Frame.h"
#include "core/video/VideoTransformHelper.h"
#include "tcr_c_api.h"

// Custom window messages for thread switching
#define WM_TCR_VIDEO_FRAME   (WM_USER + 300)
#define WM_TCR_SESSION_EVENT (WM_USER + 301)

/**
 * @brief Streaming business logic ViewModel — ports the Qt Quick version to pure Win32.
 *
 * All TcrSdk callbacks arrive on background threads. We use PostMessage with
 * WM_TCR_VIDEO_FRAME / WM_TCR_SESSION_EVENT to marshal them onto the UI thread.
 * The owner window must call handleVideoFrameMessage() / handleSessionEventMessage()
 * from its HandleMessage.
 */
class StreamingViewModel {
public:
    explicit StreamingViewModel(HWND notifyHwnd);
    ~StreamingViewModel();

    StreamingViewModel(const StreamingViewModel&) = delete;
    StreamingViewModel& operator=(const StreamingViewModel&) = delete;

    // ==================== Callbacks (set by StreamingWindow) ====================

    /// Called on UI thread when a new video frame is ready for rendering
    std::function<void(VideoFrameDataPtr)> onNewVideoFrame;

    /// Called on UI thread when the session is connected
    std::function<void()> onSessionConnected;

    /// Called on UI thread when the session is closed
    std::function<void(const std::string& reason)> onSessionClosed;

    /// Called on UI thread when client stats are updated (JSON string)
    std::function<void(const std::string& stats)> onClientStatsChanged;

    /// Called on UI thread when screen orientation changes
    std::function<void(bool isLandscape, double rotationAngle, int width, int height)> onScreenOrientationChanged;

    // ==================== Session management ====================

    void connectSession(const std::vector<std::string>& instanceIds, bool isGroupControl);
    void closeSession();
    void updateCheckedInstanceIds(const std::vector<std::string>& instanceIds,
                                  const std::vector<std::string>& addedInstanceIds = {});

    // ==================== Input ====================

    void sendTouchEvent(int x, int y, int width, int height, int eventType, int64_t timestamp);
    void sendMouseScrollEvent(float delta);

    // System keys
    void onBackClicked();
    void onHomeClicked();
    void onMenuClicked();
    void onVolumUp();
    void onVolumDown();

    // ==================== Stream control ====================

    void onPauseVideoStreamClicked();
    void onResumeVideoStreamClicked();
    void onPauseAudioStreamClicked();
    void onResumeAudioStreamClicked();

    // ==================== Device control ====================

    void onEnableCameraClicked();
    void onDisableCameraClicked();
    void onEnableMicrophoneClicked();
    void onDisableMicrophoneClicked();

    void onVideoStreamSettingsClicked();
    void setRemoteVideoProfile(int fps, int minBitrate, int maxBitrate, int width, int height);

    std::vector<std::string> getCameraDeviceList();
    std::vector<std::string> getMicrophoneDeviceList();
    void enableCameraWithDevice(const std::string& deviceId);
    void enableMicrophoneWithDevice(const std::string& deviceId);

    // ==================== Stats ====================

    std::string getInstanceStats() const;

    // ==================== Message handlers (call from window proc) ====================

    /// Handle WM_TCR_VIDEO_FRAME — lParam is VideoFrameData* (shared_ptr transferred via raw ptr)
    void handleVideoFrameMessage(WPARAM wParam, LPARAM lParam);

    /// Handle WM_TCR_SESSION_EVENT — wParam = event type, lParam = heap-allocated std::string*
    void handleSessionEventMessage(WPARAM wParam, LPARAM lParam);

private:
    HWND m_notifyHwnd = nullptr;
    std::atomic<bool> m_isDestroying{false};

    // SDK handles
    TcrClientHandle m_tcrClient = nullptr;
    TcrSessionHandle m_session = nullptr;
    TcrAndroidInstance m_instance = nullptr;
    TcrDataChannelHandle m_dataChannel = nullptr;

    // SDK callback structs (lifetime must cover the entire session)
    TcrSessionObserver m_sessionObserver = {};
    TcrVideoFrameObserver m_videoFrameObserver = {};

    // State
    std::vector<std::string> m_groupInstanceIds;
    bool m_sessionConnected = false;
    std::string m_clientStats;

    // Rotation / transform state
    VideoTransformHelper m_transformHelper;
    double m_currentRotationAngle = 0.0;
    int m_currentVideoWidth = 0;
    int m_currentVideoHeight = 0;

    // Internal methods
    void createAndInitSession();
    void setSessionObservers();
    bool isSessionReady() const;
    void sendKeyEvent(int32_t keycode);

    void handleSessionConnected();
    void handleCameraStatusChange(const std::string& eventData);
    void handleClientStatsUpdate(const std::string& eventData);
    void handleSessionClosed(const std::string& eventData);
    void handleScreenConfigChange(const std::string& eventData);

    void createDataChannel();
    void sendDataChannelTestMessage();

    // SDK callbacks (static, called from SDK threads)
    static void SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData);
    static void VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame);
};
