#pragma once

#include <atomic>
#include <cstdint>
#include <map>
#include <memory>
#include <mutex>
#include <set>
#include <string>
#include <vector>
#include "config.h"
#include "frame_queue.h"
#include "video_renderer.h"

#include <SDL.h>

struct SDL_Window;
union SDL_Event;
typedef void* SDL_GLContext;

enum class InstanceState { Offline = 0, Connecting = 1, Connected = 2 };

enum class AppState {
  TOKEN_PAGE,
  REQUESTING_TOKEN,
  MULTI_STREAM,
  DISCONNECTED,
};

struct ImGuiContext;

// Each popup is fully independent: own SDL window, ImGui context, TcrSDK session, renderer
struct PopupWindow {
  int id = 0;
  SDL_Window* sdl_window = nullptr;
  ImGuiContext* imgui_ctx = nullptr;

  bool is_sync = false;
  std::string instance_id;
  std::vector<std::string> sync_ids;

  void* session = nullptr;
  char session_obs[64] = {};
  char frame_obs[64] = {};
  FrameQueue frame_queue;
  VideoRenderer* renderer = nullptr;
  bool connected = false;
  bool close_requested = false;

  // Screen rotation (from TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE)
  float rotation_angle = 0;  // 0, 90, 180, 270
  int video_width = 0;       // cloud phone physical screen width
  int video_height = 0;      // cloud phone physical screen height
};

class App {
 public:
  App();
  ~App();

#if defined(RENDERER_D3D11)
  bool init(SDL_Window* window, void* d3d_device, void* d3d_context);
#else
  bool init(SDL_Window* window, SDL_GLContext gl_context);
#endif

  void update(float delta_time);
  void process_event(const SDL_Event& event);
  bool should_quit() const { return m_quit; }
  void render_popup_windows();

 private:
  AppState m_state = AppState::TOKEN_PAGE;
  bool m_quit = false;
  SDL_Window* m_window = nullptr;
  std::string m_error_message;
  AppConfig m_config;
  ImGuiContext* m_main_imgui_ctx = nullptr;

#if !defined(RENDERER_D3D11)
  SDL_GLContext m_gl_context = nullptr;
#endif

  // --- Token ---
  std::string m_token;
  std::string m_access_info;

  // --- TcrSDK (multi-stream session) ---
  void* m_tcr_client = nullptr;
  void* m_tcr_session = nullptr;
  void* m_tcr_instance = nullptr;
  std::atomic<bool> m_is_destroying{false};
  char m_session_observer_storage[64] = {};
  char m_video_frame_observer_storage[64] = {};

  // --- Multi-instance ---
  std::vector<std::string> m_all_instance_ids;
  std::map<std::string, InstanceState> m_instance_states;
  std::set<std::string> m_current_streaming_ids;
  std::string m_client_stats;

  // --- Checkboxes ---
  std::set<std::string> m_checked_instances;

  // --- Per-instance renderers (multi-stream grid) ---
  std::map<std::string, VideoRenderer*> m_instance_renderers;
  MultiFrameCache m_multi_frame_cache;

  // --- Scroll ---
  float m_prev_scroll_y = 0;
  float m_debounce_timer = 0;
  bool m_scroll_dirty = false;
  int m_grid_columns = 5;

  // --- Popup windows (multiple, independent) ---
  std::vector<PopupWindow*> m_popups;
  std::map<Uint32, std::vector<SDL_Event>> m_popup_events_map;  // windowID -> events
  int m_next_popup_id = 1;

  PopupWindow* create_popup(const std::string& title, bool is_sync, const std::string& instance_id,
                            const std::vector<std::string>& sync_ids);
  void destroy_popup(PopupWindow* pw);
  void render_single_popup(PopupWindow* pw);

  void open_instance_popup(const std::string& instance_id);
  void open_sync_popup(const std::set<std::string>& instance_ids);
  void close_all_popups();

  // === Token ===
  void request_token();

  // === Multi-Stream ===
  void start_multi_streaming();
  void create_multi_session();
  void access_all_instances();
  void close_session();

  void batch_render_frames();
  std::vector<std::string> calculate_visible_instances(float scroll_y, float view_height, float cell_height);
  void switch_streaming_instances(const std::vector<std::string>& ids);
  VideoRenderer* get_or_create_renderer(const std::string& instance_id);

  // === UI ===
  void render_token_page();
  void render_multi_stream_page(float delta_time);

  // === SDK callbacks (multi-stream) ===
  static void on_multi_session_event(void* user_data, int event, const char* event_data);
  static void on_multi_video_frame(void* user_data, void* frame_handle);

  // === SDK callbacks (popup session) ===
  static void on_popup_session_event(void* user_data, int event, const char* event_data);
  static void on_popup_video_frame(void* user_data, void* frame_handle);
};
