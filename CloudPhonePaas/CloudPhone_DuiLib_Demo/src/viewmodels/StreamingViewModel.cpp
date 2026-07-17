#include "StreamingViewModel.h"

#include <cstring>
#include <sstream>

#include "core/StreamConfig.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/Logger.h"

// JSON helpers (using jsoncpp via the project's existing dependency)
#include <json/json.h>

// ==================== Constants ====================

namespace {
constexpr int32_t KEYCODE_BACK = 158;
constexpr int32_t KEYCODE_HOME = 172;
constexpr int32_t KEYCODE_MENU = 139;
constexpr int32_t KEYCODE_VOLUME_UP = 58;
constexpr int32_t KEYCODE_VOLUME_DOWN = 59;

constexpr int32_t DATA_CHANNEL_PORT = 23332;
constexpr char DATA_CHANNEL_TYPE[] = "android";
constexpr size_t REQUEST_ID_BUFFER_SIZE = 256;

// Session event sub-types posted via WM_TCR_SESSION_EVENT wParam
enum SessionEventType : WPARAM {
  SE_CONNECTED = 1,
  SE_CLOSED,
  SE_CAMERA_STATUS,
  SE_CLIENT_STATS,
  SE_SCREEN_CONFIG_CHANGE,
};
}  // namespace

// ==================== Construction / Destruction ====================

StreamingViewModel::StreamingViewModel(HWND notifyHwnd) : m_notifyHwnd(notifyHwnd) {
  Logger::info("[StreamingViewModel] constructed");

  m_sessionObserver.user_data = this;
  m_sessionObserver.on_event = &StreamingViewModel::SessionEventCallback;

  m_videoFrameObserver.user_data = this;
  m_videoFrameObserver.on_frame = &StreamingViewModel::VideoFrameCallback;
}

StreamingViewModel::~StreamingViewModel() {
  Logger::info("[~StreamingViewModel] destroying");
  m_isDestroying.store(true, std::memory_order_release);
  closeSession();
  Logger::info("[~StreamingViewModel] done");
}

// ==================== Session Management ====================

void StreamingViewModel::connectSession(const std::vector<std::string>& instanceIds, bool isGroupControl) {
  if (instanceIds.empty()) {
    Logger::debug("[connectSession] empty instance list");
    return;
  }

  m_groupInstanceIds = instanceIds;

  std::string ids;
  for (size_t i = 0; i < instanceIds.size(); ++i) {
    if (i > 0) ids += ",";
    ids += instanceIds[i];
  }
  Logger::debug("[connectSession] " + std::string(isGroupControl ? "group" : "single") + " mode, ids: " + ids);

  createAndInitSession();

  if (m_session) {
    std::vector<const char*> ptrs;
    ptrs.reserve(instanceIds.size());
    for (const auto& id : instanceIds) ptrs.push_back(id.c_str());

    tcr_session_access(m_session, ptrs.data(), static_cast<int32_t>(ptrs.size()), isGroupControl);
  } else {
    Logger::error("[connectSession] failed to create session");
  }
}

void StreamingViewModel::closeSession() {
  if (m_session) {
    Logger::info("[closeSession] closing session");
    tcr_session_set_observer(m_session, nullptr);
    tcr_session_set_video_frame_observer(m_session, nullptr);

    if (m_tcrClient) {
      tcr_client_destroy_session(m_tcrClient, m_session);
    }
    m_session = nullptr;
    m_sessionConnected = false;
    m_dataChannel = nullptr;
  }
}

void StreamingViewModel::createAndInitSession() {
  closeSession();

  if (!m_tcrClient) {
    m_tcrClient = tcr_client_get_instance();
  }
  m_instance = tcr_client_get_android_instance(m_tcrClient);

  TcrSessionConfig config = tcr_session_config_default();
  StreamConfig* sc = StreamConfig::instance();
  config.stream_profile.video_width = sc->mainStreamWidth();
  config.stream_profile.fps = sc->mainStreamFps();
  config.stream_profile.max_bitrate = sc->mainStreamMaxBitrate();
  config.stream_profile.min_bitrate = sc->mainStreamMinBitrate();
  config.enable_passive_probe = true;

  Logger::info("[createAndInitSession] stream params: w=" + std::to_string(config.stream_profile.video_width) +
               " fps=" + std::to_string(config.stream_profile.fps) +
               " bitrate=" + std::to_string(config.stream_profile.min_bitrate) + "-" +
               std::to_string(config.stream_profile.max_bitrate));

  m_session = tcr_client_create_session(m_tcrClient, &config);
  setSessionObservers();
}

void StreamingViewModel::setSessionObservers() {
  if (m_session) {
    tcr_session_set_observer(m_session, &m_sessionObserver);
    tcr_session_set_video_frame_observer(m_session, &m_videoFrameObserver);
  }
}

void StreamingViewModel::updateCheckedInstanceIds(const std::vector<std::string>& instanceIds,
                                                  const std::vector<std::string>& addedInstanceIds) {
  if (!m_session) {
    Logger::debug("[updateCheckedInstanceIds] no session");
    return;
  }

  // TODO: tcr_instance_join_group / tcr_instance_set_sync_list not yet available
  // in current SDK build; re-enable when SDK is updated.
  (void)addedInstanceIds;
  (void)instanceIds;
}

// ==================== Input ====================

void StreamingViewModel::sendTouchEvent(int x, int y, int width, int height, int eventType, int64_t timestamp) {
  if (m_session && m_sessionConnected) {
    // Transform touch coordinates to account for screen rotation
    int cloudX = x, cloudY = y, cloudW = width, cloudH = height;
    if (m_currentRotationAngle != 0.0 && m_currentVideoWidth > 0 && m_currentVideoHeight > 0) {
      m_transformHelper.transformTouchCoordinates(x, y, width, height, cloudX, cloudY, cloudW, cloudH);
    }
    tcr_session_touchscreen_touch(m_session, static_cast<float>(cloudX), static_cast<float>(cloudY), eventType, cloudW,
                                  cloudH, timestamp);
  }
}

void StreamingViewModel::sendMouseScrollEvent(float delta) {
  if (m_session && m_sessionConnected) {
    tcr_session_send_mouse_scroll(m_session, delta);
  }
}

bool StreamingViewModel::isSessionReady() const { return m_session && m_sessionConnected; }

void StreamingViewModel::sendKeyEvent(int32_t keycode) {
  if (!isSessionReady()) return;
  tcr_session_send_keyboard_event(m_session, keycode, true);
  tcr_session_send_keyboard_event(m_session, keycode, false);
}

void StreamingViewModel::onBackClicked() { sendKeyEvent(KEYCODE_BACK); }
void StreamingViewModel::onHomeClicked() { sendKeyEvent(KEYCODE_HOME); }
void StreamingViewModel::onMenuClicked() { sendKeyEvent(KEYCODE_MENU); }
void StreamingViewModel::onVolumUp() { sendKeyEvent(KEYCODE_VOLUME_UP); }
void StreamingViewModel::onVolumDown() { sendKeyEvent(KEYCODE_VOLUME_DOWN); }

// ==================== Stream Control ====================

void StreamingViewModel::onPauseVideoStreamClicked() {
  if (!isSessionReady()) return;
  tcr_session_pause_streaming(m_session, "video");
}

void StreamingViewModel::onResumeVideoStreamClicked() {
  if (!isSessionReady()) return;
  tcr_session_resume_streaming(m_session, "video");
}

void StreamingViewModel::onPauseAudioStreamClicked() {
  if (!isSessionReady()) return;
  tcr_session_pause_streaming(m_session, "audio");
}

void StreamingViewModel::onResumeAudioStreamClicked() {
  if (!isSessionReady()) return;
  tcr_session_resume_streaming(m_session, "audio");
}

// ==================== Device Control ====================

void StreamingViewModel::onEnableCameraClicked() {
  if (!isSessionReady()) return;
  tcr_session_enable_local_camera(m_session, true);
}

void StreamingViewModel::onDisableCameraClicked() {
  if (!isSessionReady()) return;
  tcr_session_disable_local_camera(m_session);
}

void StreamingViewModel::onEnableMicrophoneClicked() {
  if (!isSessionReady()) return;
  tcr_session_enable_local_microphone(m_session, true);
}

void StreamingViewModel::onDisableMicrophoneClicked() {
  if (!isSessionReady()) return;
  tcr_session_enable_local_microphone(m_session, false);
}

void StreamingViewModel::onVideoStreamSettingsClicked() {
  if (!isSessionReady()) return;
  tcr_session_set_remote_video_profile(m_session, 30, 1000, 2000, 720, 1280);
}

void StreamingViewModel::setRemoteVideoProfile(int fps, int minBitrate, int maxBitrate, int width, int height) {
  if (!isSessionReady()) {
    Logger::warning("[setRemoteVideoProfile] session not ready");
    return;
  }
  Logger::info("[setRemoteVideoProfile] fps=" + std::to_string(fps) + " bitrate=" + std::to_string(minBitrate) + "-" +
               std::to_string(maxBitrate) + " " + std::to_string(width) + "x" + std::to_string(height));
  tcr_session_set_remote_video_profile(m_session, fps, minBitrate, maxBitrate, width, height);
}

std::vector<std::string> StreamingViewModel::getCameraDeviceList() {
  std::vector<std::string> list;
  if (!m_session) return list;

  int32_t count = tcr_session_get_camera_device_count(m_session);
  for (int32_t i = 0; i < count; ++i) {
    TcrCameraDeviceInfo info;
    if (tcr_session_get_camera_device(m_session, i, &info)) {
      list.emplace_back(info.device_id);
    }
  }
  return list;
}

std::vector<std::string> StreamingViewModel::getMicrophoneDeviceList() {
  std::vector<std::string> list;
  if (!m_session) return list;

  int32_t count = tcr_session_get_microphone_device_count(m_session);
  for (int32_t i = 0; i < count; ++i) {
    TcrMicrophoneDeviceInfo info;
    if (tcr_session_get_microphone_device(m_session, i, &info)) {
      list.emplace_back(info.device_id);
    }
  }
  return list;
}

void StreamingViewModel::enableCameraWithDevice(const std::string& deviceId) {
  if (!isSessionReady() || deviceId.empty()) return;

  TcrVideoConfig videoConfig = tcr_video_config_default();
  videoConfig.device_id = deviceId.c_str();

  if (tcr_session_enable_local_camera_with_config(m_session, &videoConfig)) {
    Logger::info("[enableCameraWithDevice] success: " + deviceId);
  } else {
    Logger::error("[enableCameraWithDevice] failed: " + deviceId);
  }
}

void StreamingViewModel::enableMicrophoneWithDevice(const std::string& deviceId) {
  if (!isSessionReady() || deviceId.empty()) return;

  TcrMicrophoneConfig config = {};
  strncpy(config.device_id, deviceId.c_str(), sizeof(config.device_id) - 1);

  if (tcr_session_enable_microphone_with_config(m_session, &config)) {
    Logger::info("[enableMicrophoneWithDevice] success: " + deviceId);
  } else {
    Logger::error("[enableMicrophoneWithDevice] failed: " + deviceId);
  }
}

std::string StreamingViewModel::getInstanceStats() const { return m_clientStats; }

// ==================== SDK Callbacks (static, called from SDK threads) ====================

void StreamingViewModel::SessionEventCallback(void* user_data, TcrSessionEvent event, const char* eventData) {
  auto* self = static_cast<StreamingViewModel*>(user_data);
  if (!self || self->m_isDestroying.load(std::memory_order_acquire)) return;

  // Copy event data to heap string so it survives the PostMessage
  std::string* dataCopy = new std::string(eventData ? eventData : "");
  WPARAM eventType = 0;

  if (event == TCR_SESSION_EVENT_STATE_CONNECTED) {
    eventType = SE_CONNECTED;
  } else if (event == TCR_SESSION_EVENT_STATE_CLOSED) {
    eventType = SE_CLOSED;
  } else if (event == TCR_SESSION_EVENT_CAMERA_STATUS) {
    eventType = SE_CAMERA_STATUS;
  } else if (event == TCR_SESSION_EVENT_CLIENT_STATS) {
    eventType = SE_CLIENT_STATS;
  } else if (event == TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE) {
    eventType = SE_SCREEN_CONFIG_CHANGE;
  } else {
    // Unknown event — still forward so we don't leak the string
    delete dataCopy;
    return;
  }

  if (!::PostMessage(self->m_notifyHwnd, WM_TCR_SESSION_EVENT, eventType, reinterpret_cast<LPARAM>(dataCopy))) {
    delete dataCopy;  // PostMessage failed (window destroyed?)
  }
}

void StreamingViewModel::VideoFrameCallback(void* user_data, TcrVideoFrameHandle frame_handle) {
  auto* self = static_cast<StreamingViewModel*>(user_data);
  if (!self || !frame_handle) {
    Logger::warning("[VideoFrameCallback] self=" + std::to_string(reinterpret_cast<uintptr_t>(self)) +
                    " frame_handle=" + std::to_string(reinterpret_cast<uintptr_t>(frame_handle)));
    return;
  }
  if (self->m_isDestroying.load(std::memory_order_acquire)) return;

  // Get frame buffer
  const TcrVideoFrameBuffer* buf = tcr_video_frame_get_buffer(frame_handle);
  if (!buf || buf->type != TCR_VIDEO_BUFFER_TYPE_I420) {
    Logger::warning(
        "[VideoFrameCallback] invalid buf or not I420, buf=" + std::to_string(reinterpret_cast<uintptr_t>(buf)) +
        " type=" + (buf ? std::to_string(buf->type) : "N/A"));
    return;
  }

  // Add ref so the SDK doesn't free the frame while we copy
  tcr_video_frame_add_ref(frame_handle);

  const TcrI420Buffer& i420 = buf->buffer.i420;

  // Log first frame arrival (throttle: only log the very first call)
  static std::atomic<int> s_frameCount{0};
  int cnt = s_frameCount.fetch_add(1);
  if (cnt == 0) {
    Logger::info("[VideoFrameCallback] first frame received: " + std::to_string(i420.width) + "x" +
                 std::to_string(i420.height) +
                 " notifyHwnd=" + std::to_string(reinterpret_cast<uintptr_t>(self->m_notifyHwnd)));
  }

  // Copy I420 data into owned memory
  auto frameData =
      std::make_shared<VideoFrameData>(frame_handle, i420.data_y, i420.data_u, i420.data_v, i420.stride_y,
                                       i420.stride_u, i420.stride_v, i420.width, i420.height, buf->timestamp_us);

  // Transfer ownership to the UI thread via PostMessage.
  // We use new shared_ptr on the heap so the ref-count stays alive.
  auto* heapPtr = new VideoFrameDataPtr(std::move(frameData));

  if (!::PostMessage(self->m_notifyHwnd, WM_TCR_VIDEO_FRAME, 0, reinterpret_cast<LPARAM>(heapPtr))) {
    Logger::error("[VideoFrameCallback] PostMessage failed (hwnd=" +
                  std::to_string(reinterpret_cast<uintptr_t>(self->m_notifyHwnd)) + ")");
    delete heapPtr;  // PostMessage failed — prevent leak
  }
}

// ==================== UI-thread message handlers ====================

void StreamingViewModel::handleVideoFrameMessage(WPARAM /*wParam*/, LPARAM lParam) {
  auto* heapPtr = reinterpret_cast<VideoFrameDataPtr*>(lParam);
  if (!heapPtr) {
    Logger::error("[handleVideoFrameMessage] heapPtr is null");
    return;
  }

  VideoFrameDataPtr frame = std::move(*heapPtr);
  delete heapPtr;

  if (!frame) {
    Logger::error("[handleVideoFrameMessage] frame is null after move");
    return;
  }

  static std::atomic<int> s_uiFrameCount{0};
  int cnt = s_uiFrameCount.fetch_add(1);
  if (cnt == 0) {
    Logger::info("[handleVideoFrameMessage] first frame on UI thread: " + std::to_string(frame->width) + "x" +
                 std::to_string(frame->height) + " onNewVideoFrame=" + std::to_string(onNewVideoFrame ? 1 : 0));
  }

  if (onNewVideoFrame && frame) {
    onNewVideoFrame(std::move(frame));
  } else {
    Logger::warning("[handleVideoFrameMessage] onNewVideoFrame callback not set");
  }
}

void StreamingViewModel::handleSessionEventMessage(WPARAM wParam, LPARAM lParam) {
  auto* dataCopy = reinterpret_cast<std::string*>(lParam);
  std::string eventData = dataCopy ? std::move(*dataCopy) : std::string();
  delete dataCopy;

  switch (static_cast<SessionEventType>(wParam)) {
    case SE_CONNECTED:
      handleSessionConnected();
      break;
    case SE_CLOSED:
      handleSessionClosed(eventData);
      break;
    case SE_CAMERA_STATUS:
      handleCameraStatusChange(eventData);
      break;
    case SE_CLIENT_STATS:
      handleClientStatsUpdate(eventData);
      break;
    case SE_SCREEN_CONFIG_CHANGE:
      handleScreenConfigChange(eventData);
      break;
  }
}

// ==================== Event Handlers (UI thread) ====================

void StreamingViewModel::handleSessionConnected() {
  char requestIdBuf[REQUEST_ID_BUFFER_SIZE] = {0};
  if (tcr_session_get_request_id(m_session, requestIdBuf, sizeof(requestIdBuf))) {
    Logger::info("[handleSessionConnected] RequestId: " + std::string(requestIdBuf));
  } else {
    Logger::info("[handleSessionConnected] session connected");
  }

  m_sessionConnected = true;
  createDataChannel();

  if (onSessionConnected) onSessionConnected();
}

void StreamingViewModel::handleCameraStatusChange(const std::string& eventData) {
  Logger::info("[handleCameraStatusChange] " + eventData);
}

void StreamingViewModel::handleClientStatsUpdate(const std::string& eventData) {
  m_clientStats = eventData;
  if (onClientStatsChanged) onClientStatsChanged(eventData);
}

void StreamingViewModel::handleSessionClosed(const std::string& eventData) {
  m_sessionConnected = false;
  Logger::error("[handleSessionClosed] " + eventData);
  if (onSessionClosed) onSessionClosed(eventData);
}

void StreamingViewModel::handleScreenConfigChange(const std::string& eventData) {
  Logger::info("[handleScreenConfigChange] " + eventData);

  Json::Value root;
  Json::CharReaderBuilder builder;
  std::istringstream stream(eventData);
  std::string errors;
  if (!Json::parseFromStream(builder, stream, &root, &errors)) {
    Logger::warning("[handleScreenConfigChange] parse failed: " + errors);
    return;
  }

  int width = root.get("width", 0).asInt();
  int height = root.get("height", 0).asInt();
  std::string degree = root.get("degree", "").asString();

  // Map cloud rotation to client rotation
  double rotationAngle = VideoTransformHelper::mapCloudRotationToClient(degree);

  bool isLandscape = (width > height);

  Logger::info("[handleScreenConfigChange] width=" + std::to_string(width) + " height=" + std::to_string(height) +
               " degree=" + degree + " isLandscape=" + std::string(isLandscape ? "true" : "false") +
               " clientRotation=" + std::to_string(static_cast<int>(rotationAngle)));

  // Save current rotation state
  m_currentRotationAngle = rotationAngle;
  m_currentVideoWidth = width;
  m_currentVideoHeight = height;

  // Update transform helper for touch coordinate mapping
  m_transformHelper.setRotation(rotationAngle, width, height);

  if (onScreenOrientationChanged) {
    onScreenOrientationChanged(isLandscape, rotationAngle, width, height);
  }
}

// ==================== Data Channel ====================

void StreamingViewModel::createDataChannel() {
  static TcrDataChannelObserver s_dc_observer = {
      nullptr,  // user_data — set below

      // Connected
      [](void* user_data, int32_t port) {
        auto* self = static_cast<StreamingViewModel*>(user_data);
        Logger::info("[DataChannel] connected on port " + std::to_string(port));
        self->sendDataChannelTestMessage();
      },

      // Error
      [](void* /*user_data*/, int32_t port, const TcrErrorCode& code, const char* msg) {
        Logger::error("[DataChannel] error port=" + std::to_string(port) + " code=" + std::to_string(code) +
                      " msg=" + (msg ? msg : ""));
      },

      // Message received
      [](void* /*user_data*/, int32_t port, const uint8_t* data, size_t size) {
        Logger::debug("[DataChannel] recv port=" + std::to_string(port) + " size=" + std::to_string(size));
        if (data && size > 0) {
          std::string content(reinterpret_cast<const char*>(data), size);
          Logger::debug("[DataChannel] content: " + content);
        }
      }};
  s_dc_observer.user_data = this;

  m_dataChannel = tcr_session_create_data_channel(m_session, DATA_CHANNEL_PORT, &s_dc_observer, DATA_CHANNEL_TYPE);

  if (m_dataChannel) {
    Logger::info("[DataChannel] created on port " + std::to_string(DATA_CHANNEL_PORT));
  } else {
    Logger::error("[DataChannel] creation failed");
  }
}

void StreamingViewModel::sendDataChannelTestMessage() {
  if (!m_dataChannel) return;

  std::string data =
      "{\"action\":0,\"content\":\"{\\\"type\\\":\\\"PullStreamConnected\\\"}\","
      "\"coords\":[],\"heightPixels\":0,\"isOpenScreenFollowRotation\":false,"
      "\"keyCode\":0,\"pointCount\":0,\"properties\":[],\"text\":\"\","
      "\"touchType\":\"eventSdk\",\"widthPixels\":0}";

  TcrErrorCode code = tcr_data_channel_send(m_dataChannel, reinterpret_cast<const uint8_t*>(data.data()), data.size());
  Logger::info("[DataChannel] send result code=" + std::to_string(code));
}
