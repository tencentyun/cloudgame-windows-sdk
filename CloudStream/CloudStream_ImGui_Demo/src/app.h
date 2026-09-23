#pragma once

#include <SDL.h>

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

  // --- 配置选择 ---
  std::string m_config_dir;                 // config 目录路径（含结尾分隔符）
  std::vector<std::string> m_config_names;  // 目录下所有 json 文件名（不含后缀）
  int m_selected_config = -1;               // 当前选中配置索引，-1 表示未加载
  // instanceIds 可编辑输入框 buffer（select_config 时用配置值刷新，用户可改）
  char m_instance_ids_buf[65536] = {};
  // gridCellWidth 可编辑输入框 buffer（select_config 时用配置值刷新，用户可改）
  char m_grid_cell_width_buf[32] = {};

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
  int m_grid_columns = 10;  // 当前每行格子数（render 时按窗口宽度动态计算）

  // --- 动态并发数（concurrentStreaming = 窗口可渲染的子流画面数量，作为同时出流上限）---
  int m_computed_concurrent = 0;          // 按窗口尺寸算出的并发数（= cols * rows）
  int m_session_concurrent_limit = 0;     // 当前 session 创建时的 concurrentStreamingInstances（SSRC 池上限）
  float m_last_grid_display_w = 0;        // 上次计算时的窗口可用宽度，用于检测 resize
  float m_last_grid_display_h = 0;        // 上次计算时的窗口可用高度，用于检测 resize

  // --- 自动化验证辅助（autoStart / autoExitSeconds）---
  float m_elapsed_seconds = 0;
  bool m_auto_start_fired = false;
  // 统计各类型帧的到达数量，用于确认硬解/软解是否真的生效
  std::atomic<uint64_t> m_frames_i420{0};
  std::atomic<uint64_t> m_frames_gpu{0};
  float m_stats_log_timer = 0;

  // --- per-instance 帧统计（性能测试主指标）---
  // 每个 2s 统计段内，按 instance_id 累加收到的帧数。on_multi_video_frame 在 SDK
  // 解码线程执行，统计在主线程（update）读取并清零，跨线程读写必须加锁保护。
  std::map<std::string, uint64_t> m_frame_count_window;
  std::mutex m_frame_stat_mutex;

  // --- 模拟滚动窗口列表（autoSwitch）---
  float m_auto_switch_timer = 0;      // 距离上次切换的累计秒数
  size_t m_auto_switch_offset = 0;    // 当前滑动窗口的起始索引（在 m_all_instance_ids 里）
  bool m_auto_switch_started = false; // 是否已开始模拟滚动

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

  // === 配置 ===
  // 加载指定索引的配置文件（m_config_names[index]）
  void select_config(int index);

  // === Multi-Stream ===
  void start_multi_streaming();
  void create_multi_session();
  void access_all_instances();
  void close_session();

  void batch_render_frames();
  std::vector<std::string> calculate_visible_instances(float scroll_y, float view_height, float cell_height);
  void switch_streaming_instances(const std::vector<std::string>& ids);
  VideoRenderer* get_or_create_renderer(const std::string& instance_id);

  // 根据当前窗口可用尺寸，重新计算并发数（= cols * rows，作为同时出流上限）。
  void recompute_concurrent_streaming(float avail_w, float avail_h);

  // 重建 session：当窗口 resize 后并发上限超过当前 session 的 SSRC 池时，
  // 需要以更大的 concurrentStreamingInstances 重建 session 并重新 access。
  void rebuild_session_with_new_limit();

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
