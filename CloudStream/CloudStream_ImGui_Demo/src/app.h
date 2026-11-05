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

#include "automation.h"
#include "config.h"
#include "diagnostics.h"
#include "frame_queue.h"
#include "video_renderer.h"

struct SDL_Window;
union SDL_Event;
typedef void* SDL_GLContext;

// 实例级状态。全部由「实际出流情况」驱动，而不是会话级状态。
//   Offline        会话未连接，或该实例当前没有出流
//   Connecting     已请求出流，但还没收到帧
//   Connected      正在出帧（On）
//   InstanceOffline 服务端明确回复该实例 offline，此时不会有流
enum class InstanceState { Offline = 0, Connecting = 1, Connected = 2, InstanceOffline = 3 };

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
  std::atomic<uint64_t> frame_count{0};  // 收到的解码帧数（SDK 解码线程写）
  // 会话状态文案：连接中 / 已关闭。close_reason 为 STATE_CLOSED 带回的关闭原因，用于 UI 显示。
  std::string close_reason;

  // Screen rotation (from TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE)
  float rotation_angle = 0;  // 0, 90, 180, 270
  int video_width = 0;       // cloud phone physical screen width
  int video_height = 0;      // cloud phone physical screen height
};

// 多流网格的当前布局，每帧由 render_multi_stream_page 刷新
struct GridLayout {
  int cols = 10;       // 每行格子数（按窗口宽度动态计算）
  float row_h = 0;     // 行高（含间距）
  float view_h = 0;    // 可视区高度
  float scroll_y = 0;  // 当前滚动位置
};

class App;

// === 本地构建的扩展点（内部调试需要）===
class LocalBuildHooks {
 public:
  virtual ~LocalBuildHooks() {}
  // 在 token 页渲染额外控件，返回 true 表示请求连接
  virtual bool on_token_page_ui(App* app) { return false; }
  // 在 tcr_client_init 成功后、access 之前调用
  virtual void before_access(void* tcr_client) {}
};

// 返回本地构建提供的 hook，外部构建返回 nullptr
LocalBuildHooks* get_local_build_hooks();

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

  Diagnostics& diagnostics() { return m_diag; }

  // === 查询与操作接口 ===
  // 界面上的交互最终都落到这些方法；Automation / Diagnostics 也只通过它们访问 App，不碰内部状态。
  AppState state() const { return m_state; }
  float elapsed_seconds() const { return m_elapsed_seconds; }
  AppConfig& config() { return m_config; }
  const std::vector<std::string>& instance_ids() const { return m_all_instance_ids; }
  const std::set<std::string>& streaming_ids() const { return m_current_streaming_ids; }
  const std::map<std::string, VideoRenderer*>& renderers() const { return m_instance_renderers; }
  const std::vector<PopupWindow*>& popups() const { return m_popups; }
  const GridLayout& grid_layout() const { return m_grid; }
  bool has_session() const { return m_tcr_session != nullptr; }
  int concurrent_limit() const;
  bool is_rendered(const std::string& instance_id) const;  // 该格子已有画面
  std::string fail_reason(const std::string& instance_id);
  std::map<std::string, uint64_t> frame_totals();              // 每路累计帧数
  std::map<std::string, uint64_t> take_window_frame_counts();  // 取走自上次调用以来每路的帧数

  // 覆盖 token 页输入框；ids 为 nullptr / grid_cell_width<=0 表示不改
  void set_token_page_inputs(const char* ids, int grid_cell_width);
  void request_token();
  void scroll_grid_to(float y) { m_pending_scroll_y = y; }  // 下一帧生效
  void switch_streaming_instances(const std::vector<std::string>& ids);
  PopupWindow* open_instance_popup(const std::string& instance_id);
  void request_close_popups();
  void quit() { m_quit = true; }

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
  char m_instance_ids_buf[1 << 20] = {};  // 1MB：字符串形式存实例列表，500 路约 7KB，这里留足量避免截断
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
  // 服务端在 switch 回执里给出的实例级失败原因：instance_id -> reason 原文。
  // 直接展示服务端返回的文本（如 "instance offline"），不在这里做映射或改写。
  // 事件回调在 SDK 线程写入，主线程渲染时读取，统一由下面的 mutex 保护。
  std::map<std::string, std::string> m_instance_fail_reason;
  // 保护 m_instance_states 与 m_instance_fail_reason：SDK 事件线程写，主线程渲染读。
  std::mutex m_instance_offline_mutex;
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
  float m_pending_scroll_y = -1;  // >=0 时下一帧把 grid 滚动到该位置
  GridLayout m_grid;

  // --- 动态并发数（concurrentStreaming = 窗口可渲染的子流画面数量，作为同时出流上限）---
  int m_computed_concurrent = 0;       // 按窗口尺寸算出的并发数（= cols * rows）
  int m_session_concurrent_limit = 0;  // 当前 session 创建时的 concurrentStreamingInstances（SSRC 池上限）
  float m_last_grid_display_w = 0;     // 上次计算时的窗口可用宽度，用于检测 resize
  float m_last_grid_display_h = 0;     // 上次计算时的窗口可用高度，用于检测 resize

  float m_elapsed_seconds = 0;

  // --- per-instance 帧计数 ---
  // on_multi_video_frame 在 SDK 解码线程累加，主线程读取，跨线程读写必须加锁。
  // total 决定格子的 On 标签；window 供 Diagnostics 按 2s 统计段取走。
  std::map<std::string, uint64_t> m_frame_count_window;
  std::map<std::string, uint64_t> m_frame_count_total;
  std::mutex m_frame_stat_mutex;

  Diagnostics m_diag{*this};
  Automation m_automation{*this};

  // 出流后「滚走再滚回」一次：0=未触发（进入多流页面后计时到延迟），
  // 1=已切到别处，2=已切回并结束。
  int m_reswitch_phase = 0;
  float m_reswitch_timer = 0;

  // --- Popup windows (multiple, independent) ---
  std::vector<PopupWindow*> m_popups;
  std::map<Uint32, std::vector<SDL_Event>> m_popup_events_map;  // windowID -> events
  int m_next_popup_id = 1;

  PopupWindow* create_popup(const std::string& title, bool is_sync, const std::string& instance_id,
                            const std::vector<std::string>& sync_ids);
  void destroy_popup(PopupWindow* pw);
  void render_single_popup(PopupWindow* pw);

  void open_sync_popup(const std::set<std::string>& instance_ids);
  void close_all_popups();

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
