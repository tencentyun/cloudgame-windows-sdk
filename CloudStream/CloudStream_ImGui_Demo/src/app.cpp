#include "app.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>

#include "http_client.h"
#include "logger.h"
#if defined(RENDERER_D3D11)
#  include <imgui_impl_dx11.h>
#else
#  include <imgui_impl_opengl3.h>
#  if defined(__APPLE__)
#    include <OpenGL/gl3.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

#include <algorithm>
#include <cstring>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#include "tcr_c_api.h"
#include "tcr_types.h"

// 当前进程常驻内存（MB）。只用于在 LoopStats 里给个量级参考；
// 精确的 GPU 显存归因要看 DXGI QueryVideoMemoryInfo / PIX，不能拿 RSS 代替。
#if defined(_WIN32)
#  include <psapi.h>
#  include <windows.h>
#  pragma comment(lib, "psapi.lib")
#endif
#if defined(__APPLE__)
#  include <mach/mach.h>
#endif

namespace {
int current_rss_mb() {
#if defined(_WIN32)
  PROCESS_MEMORY_COUNTERS pmc = {};
  if (GetProcessMemoryInfo(GetCurrentProcess(), &pmc, sizeof(pmc))) {
    return (int)(pmc.WorkingSetSize / (1024 * 1024));
  }
  return 0;
#elif defined(__APPLE__)
  mach_task_basic_info info = {};
  mach_msg_type_number_t count = MACH_TASK_BASIC_INFO_COUNT;
  if (task_info(mach_task_self(), MACH_TASK_BASIC_INFO, (task_info_t)&info, &count) == KERN_SUCCESS) {
    return (int)(info.resident_size / (1024 * 1024));
  }
  return 0;
#else
  return 0;
#endif
}
}  // namespace

// =============================================================================
// Lifecycle
// =============================================================================

App::App() {}

App::~App() {
  m_is_destroying.store(true, std::memory_order_release);
  close_all_popups();
  close_session();
  for (auto& kv : m_instance_renderers) {
    kv.second->destroy();
    delete kv.second;
  }
  m_instance_renderers.clear();
  // 所有 renderer 都销毁后才能释放共享资源
  VideoRenderer::shutdown_graphics();
}

// =============================================================================
// Init
// =============================================================================

#if defined(RENDERER_D3D11)
bool App::init(SDL_Window* window, void* d3d_device, void* d3d_context) {
  m_window = window;
  // 建立渲染后端与共享 GPU 资源。此后所有 VideoRenderer 都复用它们。
  if (!VideoRenderer::init_graphics(d3d_device, d3d_context)) return false;
#else
bool App::init(SDL_Window* window, SDL_GLContext gl_context) {
  m_window = window;
  m_gl_context = gl_context;
  // OpenGL 后端不需要 device/context，共享资源建在当前 GL context 上
  if (!VideoRenderer::init_graphics(nullptr, nullptr)) return false;
#endif
  // 定位 config/ 目录：优先 SDL_GetBasePath()（可执行文件同目录；macOS 上为 .app/Contents/Resources/）
  // 下，其次回退到 macOS 的可执行文件所在目录（Contents/MacOS/）。
  {
    char* bp = SDL_GetBasePath();
    std::string base = bp ? std::string(bp) : "";
    SDL_free(bp);

    std::vector<std::string> candidates;
    candidates.push_back(base + "config/");
#if defined(__APPLE__)
    // SDL_GetBasePath() 在 macOS 上返回 Contents/Resources/，而可执行文件在 Contents/MacOS/
    size_t p = base.rfind("Resources/");
    if (p != std::string::npos) {
      candidates.push_back(base.substr(0, p) + "MacOS/config/");
    }
#endif

    m_config_dir.clear();
    for (const auto& dir : candidates) {
      m_config_names = scan_config_files(dir);
      if (!m_config_names.empty()) {
        m_config_dir = dir;
        break;
      }
    }

    // 默认加载第一个配置
    if (!m_config_names.empty()) {
      select_config(0);
    } else {
      LOG_WARN("App", "No json config found in config/ directory");
    }
  }

  static TcrLogCallback lcb = {};
  lcb.on_log = [](void*, TcrLogLevel lv, const char* tag, const char* msg) {
    // 给 TcrSdk 日志统一加 "TcrSdk/" 前缀，与 Demo 自身日志（App/Config/Main 等）区分开。
    std::string sdk_tag = std::string("TcrSdk/") + (tag ? tag : "?");
    switch (lv) {
      case TCR_LOG_LEVEL_DEBUG:
        LOG_DEBUG(sdk_tag.c_str(), "%s", msg);
        break;
      case TCR_LOG_LEVEL_INFO:
        LOG_INFO(sdk_tag.c_str(), "%s", msg);
        break;
      case TCR_LOG_LEVEL_WARN:
        LOG_WARN(sdk_tag.c_str(), "%s", msg);
        break;
      case TCR_LOG_LEVEL_ERROR:
        LOG_ERROR(sdk_tag.c_str(), "%s", msg);
        break;
      default:
        break;
    }
  };
  tcr_set_log_callback(&lcb);
  tcr_set_log_level(TCR_LOG_LEVEL_INFO);
  LOG_INFO("App", "Init ok. instances=%zu concurrent=%d", m_config.get_instance_id_list().size(),
           m_config.concurrent_streaming);
  m_main_imgui_ctx = ImGui::GetCurrentContext();
  return true;
}

// =============================================================================
// Events
// =============================================================================

void App::process_event(const SDL_Event& ev) {
  for (auto* pw : m_popups) {
    if (!pw->sdl_window) continue;
    Uint32 wid = SDL_GetWindowID(pw->sdl_window);

    if (ev.type == SDL_WINDOWEVENT && ev.window.windowID == wid && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
      pw->close_requested = true;
      continue;
    }

    bool match = false;
    if (ev.type == SDL_MOUSEMOTION && ev.motion.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEBUTTONUP && ev.button.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEWHEEL && ev.wheel.windowID == wid) match = true;
    if (ev.type == SDL_KEYDOWN && ev.key.windowID == wid) match = true;
    if (ev.type == SDL_KEYUP && ev.key.windowID == wid) match = true;
    if (ev.type == SDL_TEXTINPUT && ev.text.windowID == wid) match = true;

    if (match) {
      m_popup_events_map[wid].push_back(ev);
    }
  }
}

// =============================================================================
// Main loop
// =============================================================================

void App::update(float dt) {
  // Deferred popup close
  for (size_t i = 0; i < m_popups.size();) {
    if (m_popups[i]->close_requested) {
      destroy_popup(m_popups[i]);
      m_popups.erase(m_popups.begin() + i);
    } else {
      ++i;
    }
  }

  batch_render_frames();

  // Upload frames for all popup windows
  for (auto* pw : m_popups) {
    if (pw->renderer) {
      VideoFrame f;
      if (pw->frame_queue.pop(f)) {
        if (f.is_gpu) {
          pw->renderer->upload_gpu_frame(f.gpu);
        } else {
          pw->renderer->upload_frame(f.data_y, f.data_u, f.data_v, f.stride_y, f.stride_u, f.stride_v, f.width,
                                     f.height);
        }
      }
    }
  }

  switch (m_state) {
    case AppState::TOKEN_PAGE:
    case AppState::REQUESTING_TOKEN:
      render_token_page();
      break;
    case AppState::MULTI_STREAM:
    case AppState::DISCONNECTED:
      render_multi_stream_page(dt);
      break;
  }

  // ---- 自动化验证辅助 ----
  m_elapsed_seconds += dt;
  if (m_state == AppState::MULTI_STREAM && m_stream_start_elapsed < 0) m_stream_start_elapsed = m_elapsed_seconds;
  if (m_stream_start_elapsed >= 0 && !m_coverage_10s_logged && m_elapsed_seconds - m_stream_start_elapsed >= 10.0f) {
    m_coverage_10s_logged = true;
    log_streaming_render_coverage(true);
  }

  // autoStart: 启动 1 秒后自动触发一次取 token + 串流
  if (m_config.auto_start && !m_auto_start_fired && m_elapsed_seconds > 1.0f && m_state == AppState::TOKEN_PAGE) {
    m_auto_start_fired = true;
    LOG_INFO("App", "autoStart triggered");
    request_token();
  }

  // 每 2 秒打印一次帧统计，便于确认硬解/软解是否真的产出帧。
  // 注意：不再调用 sample_average_luma()——那是 GPU→CPU 回读，在多路（300 路）下
  // 是明显的观测开销污染源。per-instance 帧计数（PerInstanceFrames）作为主指标。
  m_stats_log_timer += dt;
  if (m_stats_log_timer >= 2.0f) {
    m_stats_log_timer = 0;
    LOG_INFO("FrameStats", "elapsed=%.1fs i420_frames=%llu gpu_frames=%llu", m_elapsed_seconds,
             (unsigned long long)m_frames_i420.load(), (unsigned long long)m_frames_gpu.load());

    // 汇总本 2s 段每路收到的帧数，分类统计 active/low_fps/stalled 后清零。
    int active = 0, low_fps = 0, stalled = 0;
    size_t total = 0;
    {
      std::lock_guard<std::mutex> lock(m_frame_stat_mutex);
      total = m_frame_count_window.size();
      for (auto& kv : m_frame_count_window) {
        if (kv.second >= 2)
          active++;
        else if (kv.second == 1)
          low_fps++;
        else
          stalled++;
      }
      m_frame_count_window.clear();
    }
    LOG_INFO("PerInstanceFrames", "active=%d low_fps=%d stalled=%d total=%zu", active, low_fps, stalled, total);

    log_streaming_render_coverage(false);

    // 画面正确性：帧计数正常但画面黑屏/错位时只能靠回读发现。
    // 默认关闭（GPU→CPU 同步回读是观测开销），只在取证轮打开。
    if ((m_config.verify_luma || m_config.verify_luma_all) && m_state == AppState::MULTI_STREAM) {
      verify_luma_pass(m_config.verify_luma_all);
    }
  }

  if (m_config.auto_exit_seconds > 0 && m_elapsed_seconds >= (float)m_config.auto_exit_seconds) {
    log_streaming_render_coverage(true);
    LOG_INFO("App", "autoExitSeconds reached, quitting. i420_frames=%llu gpu_frames=%llu",
             (unsigned long long)m_frames_i420.load(), (unsigned long long)m_frames_gpu.load());
    m_quit = true;
  }

  // autoSwitch: 模拟滚动窗口列表，周期性切换实例子集（触发 tcr_session_switch_streaming_instances）
  if (m_config.auto_switch && m_state == AppState::MULTI_STREAM && !m_all_instance_ids.empty() && m_tcr_session) {
    if (!m_auto_switch_started) {
      // 首次：从 instance_ids 头部取一个窗口，记录起点
      m_auto_switch_started = true;
      m_auto_switch_timer = 0;
      m_auto_switch_offset = 0;
    } else {
      m_auto_switch_timer += dt;
      if (m_auto_switch_timer >= (float)m_config.auto_switch_interval_seconds) {
        m_auto_switch_timer = 0;
        // 滑动窗口：每次起点 +1（模拟滚动一行），取当前动态并发数个实例
        size_t n = m_all_instance_ids.size();
        size_t lim = (size_t)(m_computed_concurrent > 0 ? m_computed_concurrent : m_config.concurrent_streaming);
        std::vector<std::string> ids;
        ids.reserve(lim);
        for (size_t i = 0; i < lim && i < n; ++i) {
          ids.push_back(m_all_instance_ids[(m_auto_switch_offset + i) % n]);
        }
        m_auto_switch_offset = (m_auto_switch_offset + 1) % n;
        LOG_INFO("App", "autoSwitch: switching to window offset=%zu, count=%zu", m_auto_switch_offset, ids.size());
        switch_streaming_instances(ids);
      }
    }
  }
}

// =============================================================================
// 主循环分阶段计时
// =============================================================================
//
// 五段：evt（SDL 事件）/ ui（ImGui 控件构建）/ upload（帧上传 GPU）/ gpu（Render+Present）/ tail。
// upload 发生在 update() 内部，从 ui 段里扣掉，否则会全部算到 ui 头上，归因直接错。
// 分位数按 2s 窗口统计后打印，只看平均值会被偶发卡顿稀释。

namespace {
double now_ms() { return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency(); }
const int kStageCount = 5;
enum { kStageEvt = 0, kStageUpload, kStageUi, kStageGpu, kStageTail };
}  // namespace

void App::begin_stage_frame() {
  m_stage_ms.assign(kStageCount, 0.0);
  m_upload_ms_frame = 0.0;
  m_prev_stage_ms = now_ms();
}

void App::mark_evt_begin() { m_prev_stage_ms = now_ms(); }

void App::mark_evt_end() {
  const double t = now_ms();
  add_stage_ms(kStageEvt, t - m_prev_stage_ms);
  m_prev_stage_ms = t;
}

// 取走本帧上传耗时，避免重复累加
float App::take_upload_ms() { return (float)m_upload_ms_frame; }

void App::mark_gpu_begin() {
  const double t = now_ms();
  double ui = t - m_prev_stage_ms - m_upload_ms_frame;
  add_stage_ms(kStageUi, ui > 0 ? ui : 0.0);
  m_stage_ms[kStageUpload] += m_upload_ms_frame;
  m_prev_stage_ms = t;
}

void App::mark_gpu_end() {
  const double t = now_ms();
  add_stage_ms(kStageGpu, t - m_prev_stage_ms);
  m_prev_stage_ms = t;
}

void App::end_stage_frame() {
  const double t = now_ms();
  add_stage_ms(kStageTail, t - m_prev_stage_ms);

  double total = 0;
  for (double v : m_stage_ms) total += v;
  m_loop_ms.push_back(total);
  for (int i = 0; i < kStageCount; ++i) m_stage_hist[i].push_back(m_stage_ms[i]);

  // 与 PerInstanceFrames 同为 2s 周期，便于对照
  if (m_loop_ms.size() >= 2 && m_elapsed_seconds - m_last_loop_log >= 2.0f) {
    m_last_loop_log = m_elapsed_seconds;
    log_loop_stats();
  }
}

void App::add_stage_ms(int stage, double ms) {
  if (stage >= 0 && stage < kStageCount) m_stage_ms[stage] += ms;
}

void App::log_loop_stats() {
  if (m_loop_ms.empty()) return;

  auto pct = [](std::vector<double> v, double p) -> double {
    if (v.empty()) return 0.0;
    std::sort(v.begin(), v.end());
    size_t i = (size_t)(p * (double)(v.size() - 1) + 0.5);
    return v[i];
  };
  auto mean = [](const std::vector<double>& v) -> double {
    if (v.empty()) return 0.0;
    double s = 0;
    for (double x : v) s += x;
    return s / (double)v.size();
  };

  std::vector<double> loop = m_loop_ms;
  std::sort(loop.begin(), loop.end());
  const double secs = m_elapsed_seconds - m_last_loop_log_start;
  const double p50 = pct(loop, 0.50), p95 = pct(loop, 0.95), p99 = pct(loop, 0.99);
  const double maxv = loop.empty() ? 0.0 : loop.back();

  LOG_INFO("LoopStats",
           "fps=%.1f frame_ms avg=%.2f p50=%.2f p95=%.2f p99=%.2f max=%.2f | "
           "evt=%.2f upload=%.2f ui=%.2f gpu=%.2f tail=%.2f | "
           "upload_p95=%.2f gpu_p95=%.2f | uploads=%llu cells=%d draw_cmds=%d verts=%d rss_mb=%zu renderers=%d",
           secs > 0.01 ? (double)m_loop_ms.size() / secs : 0.0, mean(loop), p50, p95, p99, maxv,
           mean(m_stage_hist[kStageEvt]), mean(m_stage_hist[kStageUpload]), mean(m_stage_hist[kStageUi]),
           mean(m_stage_hist[kStageGpu]), mean(m_stage_hist[kStageTail]), pct(m_stage_hist[kStageUpload], 0.95),
           pct(m_stage_hist[kStageGpu], 0.95), (unsigned long long)m_window_uploads, m_window_cells,
           ImGui::GetDrawData() ? ImGui::GetDrawData()->CmdListsCount : 0, ImGui::GetIO().MetricsRenderVertices,
           (size_t)current_rss_mb(), (int)m_instance_renderers.size());

  m_loop_ms.clear();
  for (int i = 0; i < kStageCount; ++i) m_stage_hist[i].clear();
  m_window_uploads = 0;
  m_window_cells = 0;
  m_last_loop_log_start = m_elapsed_seconds;
}

// 回读每路输出纹理的平均亮度。
//
// 这条统计和 PerInstanceFrames 互补：共享 scratch + uvScale 如果算错，或者上传完
// 没解绑 RTV/SRV，帧计数一切正常但画面是黑屏或错位。判据有两条 ——
//   1. nonzero 路数 == 有帧的路数（否则就是黑屏）
//   2. 亮度值不是全同一个数（全相同说明采样位置算错，所有格子采到了同一块像素）
void App::verify_luma_pass(bool all) {
  std::vector<VideoRenderer*> targets;
  std::vector<std::string> ids;
  for (auto& kv : m_instance_renderers) {
    if (!kv.second->has_frame()) continue;
    targets.push_back(kv.second);
    ids.push_back(kv.first);
    if (!all && targets.size() >= 8) break;
  }
  if (targets.empty()) return;

  int nonzero = 0;
  int unavailable = 0;
  int min_l = 255, max_l = 0;
  uint64_t sum = 0;
  std::string sample;
  for (size_t i = 0; i < targets.size(); ++i) {
    int l = targets[i]->sample_average_luma();
    if (l < 0) {
      unavailable++;
      continue;
    }
    if (l > 0) nonzero++;
    min_l = std::min(min_l, l);
    max_l = std::max(max_l, l);
    sum += (uint64_t)l;
    if (sample.size() < 400) {
      sample += ids[i].substr(ids[i].size() > 6 ? ids[i].size() - 6 : 0) + "=" + std::to_string(l) + " ";
    }
  }
  if (min_l > max_l) {
    min_l = max_l = 0;
  }
  const int checked = (int)targets.size() - unavailable;
  LOG_INFO("LumaVerify", "checked=%d nonzero=%d unavailable=%d min=%d max=%d avg=%d spread=%d | %s", checked, nonzero,
           unavailable, min_l, max_l, checked > 0 ? (int)(sum / (uint64_t)checked) : 0, max_l - min_l, sample.c_str());
}

// 统计当前出流（switch_streaming_instances 选中）的实例里，有多少格子已经渲染出过画面。
// PerInstanceFrames 只能看到收到帧的实例，看不到"从未出帧"的格子，所以单独统计。
void App::log_streaming_render_coverage(bool list_all_missing) {
  if (m_state != AppState::MULTI_STREAM && m_state != AppState::DISCONNECTED) return;
  if (m_stream_start_elapsed < 0) return;
  size_t rendered = 0;
  std::vector<std::string> missing;
  for (const auto& id : m_current_streaming_ids) {
    auto it = m_instance_renderers.find(id);
    if (it != m_instance_renderers.end() && it->second->has_frame())
      rendered++;
    else
      missing.push_back(id);
  }
  std::string ms;
  size_t n = list_all_missing ? missing.size() : std::min<size_t>(missing.size(), 5);
  // 区分：SDK 已回调过解码帧但格子没画面（本地渲染链路问题） vs SDK 从未回调（解码前/服务端）
  size_t cb_but_no_render = 0;
  std::string cb_ids;
  {
    std::lock_guard<std::mutex> lock(m_frame_stat_mutex);
    for (const auto& id : missing) {
      auto it = m_frame_count_total.find(id);
      if (it != m_frame_count_total.end() && it->second > 0) {
        cb_but_no_render++;
        cb_ids += id + "(" + std::to_string(it->second) + ") ";
      }
    }
  }
  if (list_all_missing || cb_but_no_render)
    LOG_INFO("RenderCoverage", "missing_with_sdk_callback=%zu %s", cb_but_no_render, cb_ids.c_str());
  if (list_all_missing) {
    // 收到了帧回调、但不在 Demo 期望出流集合里的实例（说明服务端推的集合 ≠ Demo 期望的集合）
    std::set<std::string> want(m_current_streaming_ids.begin(), m_current_streaming_ids.end());
    std::string extra;
    size_t n_extra = 0;
    std::lock_guard<std::mutex> lock(m_frame_stat_mutex);
    for (const auto& kv : m_frame_count_total) {
      if (!want.count(kv.first)) {
        n_extra++;
        extra += kv.first + "(" + std::to_string(kv.second) + ") ";
      }
    }
    LOG_INFO("RenderCoverage", "callback_instances=%zu not_in_streaming_set=%zu %s", m_frame_count_total.size(),
             n_extra, extra.c_str());
  }
  for (size_t i = 0; i < n; ++i) ms += (i ? "," : "") + missing[i];
  LOG_INFO("RenderCoverage", "since_stream=%.1fs streaming=%zu rendered=%zu missing=%zu [%s]%s",
           m_elapsed_seconds - m_stream_start_elapsed, m_current_streaming_ids.size(), rendered, missing.size(),
           ms.c_str(), n < missing.size() ? "..." : "");
}

// =============================================================================
// Popup management
// =============================================================================

PopupWindow* App::create_popup(const std::string& title, bool is_sync, const std::string& instance_id,
                               const std::vector<std::string>& sync_ids) {
  if (!m_tcr_client) return nullptr;

  PopupWindow* pw = new PopupWindow();
  pw->id = m_next_popup_id++;
  pw->is_sync = is_sync;
  pw->instance_id = instance_id;
  pw->sync_ids = sync_ids;

#if !defined(RENDERER_D3D11)
  SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
  pw->sdl_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 540, 960,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
  if (!pw->sdl_window) {
    delete pw;
    return nullptr;
  }

  // ImGui context
  pw->imgui_ctx = ImGui::CreateContext();
  ImGui::SetCurrentContext(pw->imgui_ctx);
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(pw->sdl_window, m_gl_context);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::SetCurrentContext(m_main_imgui_ctx);

  // Renderer
  pw->renderer = new VideoRenderer();
  pw->renderer->init();

  // Session
  TcrSessionConfig cfg = tcr_session_config_default();
  cfg.stream_profile.video_width = m_config.video_width;
  cfg.stream_profile.fps = m_config.video_fps;
  cfg.stream_profile.min_bitrate = m_config.video_min_bitrate;
  cfg.stream_profile.max_bitrate = m_config.video_max_bitrate;

  pw->session = tcr_client_create_session(static_cast<TcrClientHandle>(m_tcr_client), &cfg);
  if (!pw->session) {
    LOG_ERROR("App", "Failed to create popup session");
    destroy_popup(pw);
    return nullptr;
  }

  // Observers - user_data points to the PopupWindow itself
  TcrSessionObserver* obs = reinterpret_cast<TcrSessionObserver*>(pw->session_obs);
  memset(obs, 0, sizeof(TcrSessionObserver));
  obs->user_data = pw;
  obs->on_event = reinterpret_cast<decltype(obs->on_event)>(&App::on_popup_session_event);
  tcr_session_set_observer(static_cast<TcrSessionHandle>(pw->session), obs);

  TcrVideoFrameObserver* vobs = reinterpret_cast<TcrVideoFrameObserver*>(pw->frame_obs);
  memset(vobs, 0, sizeof(TcrVideoFrameObserver));
  vobs->user_data = pw;
  vobs->on_frame = reinterpret_cast<decltype(vobs->on_frame)>(&App::on_popup_video_frame);
  tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(pw->session), vobs);

  // Connect
  if (is_sync) {
    std::vector<const char*> ptrs;
    for (const auto& id : sync_ids) ptrs.push_back(id.c_str());
    tcr_session_access(static_cast<TcrSessionHandle>(pw->session), ptrs.data(), static_cast<int32_t>(ptrs.size()),
                       true);
    LOG_INFO("App", "Popup #%d: sync %zu instances (group control)", pw->id, sync_ids.size());
  } else {
    const char* id_ptr = instance_id.c_str();
    tcr_session_access(static_cast<TcrSessionHandle>(pw->session), &id_ptr, 1, false);
    LOG_INFO("App", "Popup #%d: instance %s", pw->id, instance_id.c_str());
  }

  m_popups.push_back(pw);
  return pw;
}

void App::destroy_popup(PopupWindow* pw) {
  if (!pw) return;

  if (pw->imgui_ctx) {
    ImGui::SetCurrentContext(pw->imgui_ctx);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(pw->imgui_ctx);
    pw->imgui_ctx = nullptr;
    ImGui::SetCurrentContext(m_main_imgui_ctx);
  }
  if (pw->session) {
    tcr_session_set_observer(static_cast<TcrSessionHandle>(pw->session), nullptr);
    tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(pw->session), nullptr);
    if (m_tcr_client)
      tcr_client_destroy_session(static_cast<TcrClientHandle>(m_tcr_client),
                                 static_cast<TcrSessionHandle>(pw->session));
    pw->session = nullptr;
  }
  if (pw->renderer) {
    pw->renderer->destroy();
    delete pw->renderer;
    pw->renderer = nullptr;
  }
  if (pw->sdl_window) {
    Uint32 wid = SDL_GetWindowID(pw->sdl_window);
    m_popup_events_map.erase(wid);
    SDL_DestroyWindow(pw->sdl_window);
    pw->sdl_window = nullptr;
  }
  pw->frame_queue.clear();
  delete pw;
}

void App::open_instance_popup(const std::string& id) {
  std::string title = "Instance: " + (id.size() > 30 ? "..." + id.substr(id.size() - 27) : id);
  create_popup(title, false, id, {});
}

void App::open_sync_popup(const std::set<std::string>& ids) {
  std::vector<std::string> v(ids.begin(), ids.end());
  create_popup("Sync Operations (Group Control)", true, "", v);
}

void App::close_all_popups() {
  for (auto* pw : m_popups) destroy_popup(pw);
  m_popups.clear();
  m_popup_events_map.clear();
}

// =============================================================================
// Popup callbacks - user_data is PopupWindow*
// =============================================================================

void App::on_popup_session_event(void* ud, int event, const char* data) {
  PopupWindow* pw = static_cast<PopupWindow*>(ud);
  if (!pw) return;
  TcrSessionEvent ev = (TcrSessionEvent)event;
  if (ev == TCR_SESSION_EVENT_STATE_CONNECTED) {
    pw->connected = true;
    LOG_INFO("App", "Popup #%d connected", pw->id);
  } else if (ev == TCR_SESSION_EVENT_STATE_CLOSED) {
    pw->connected = false;
    pw->close_reason = data ? data : "";
    LOG_WARN("App", "Popup #%d closed: %s", pw->id, pw->close_reason.c_str());
  } else if (ev == TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE && data) {
    // Parse rotation and screen size from event data
    try {
      auto j = nlohmann::json::parse(data);
      pw->video_width = j.value("width", 0);
      pw->video_height = j.value("height", 0);
      std::string degree = j.value("degree", "");
      // Map cloud rotation to client rotation (reverse)
      if (degree.find("90") != std::string::npos)
        pw->rotation_angle = 270;
      else if (degree.find("180") != std::string::npos)
        pw->rotation_angle = 180;
      else if (degree.find("270") != std::string::npos)
        pw->rotation_angle = 90;
      else
        pw->rotation_angle = 0;
      LOG_INFO("App", "Popup #%d screen config: %dx%d degree=%s rotation=%.0f", pw->id, pw->video_width,
               pw->video_height, degree.c_str(), pw->rotation_angle);
    } catch (...) {
    }
  }
}

void App::on_popup_video_frame(void* ud, void* fh) {
  PopupWindow* pw = static_cast<PopupWindow*>(ud);
  if (!pw || !fh) return;
  TcrVideoFrameHandle h = static_cast<TcrVideoFrameHandle>(fh);
  const TcrVideoFrameBuffer* b = tcr_video_frame_get_buffer(h);
  if (!b) return;
  if (b->type == TCR_VIDEO_BUFFER_TYPE_GPU) {
    tcr_video_frame_add_ref(h);
    pw->frame_queue.push(VideoFrame(h, b->buffer.gpu, b->timestamp_us));
    return;
  }
  if (b->type != TCR_VIDEO_BUFFER_TYPE_I420) return;
  tcr_video_frame_add_ref(h);
  const TcrI420Buffer& i = b->buffer.i420;
  VideoFrame vf(h, i.data_y, i.data_u, i.data_v, i.stride_y, i.stride_u, i.stride_v, i.width, i.height,
                b->timestamp_us);
  pw->frame_queue.push(std::move(vf));
}

// =============================================================================
// Popup rendering
// =============================================================================

void App::render_popup_windows() {
  for (auto* pw : m_popups) {
    if (pw->close_requested || !pw->sdl_window || !pw->imgui_ctx) continue;
    render_single_popup(pw);
  }
}

void App::render_single_popup(PopupWindow* pw) {
#if !defined(RENDERER_D3D11)
  ImGui::SetCurrentContext(pw->imgui_ctx);
  SDL_GL_MakeCurrent(pw->sdl_window, m_gl_context);

  int win_w, win_h;
  SDL_GetWindowSize(pw->sdl_window, &win_w, &win_h);
  int draw_w, draw_h;
  SDL_GL_GetDrawableSize(pw->sdl_window, &draw_w, &draw_h);

  glViewport(0, 0, draw_w, draw_h);
  glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Replay buffered events
  Uint32 wid = SDL_GetWindowID(pw->sdl_window);
  auto it = m_popup_events_map.find(wid);
  if (it != m_popup_events_map.end()) {
    for (const auto& ev : it->second) ImGui_ImplSDL2_ProcessEvent(&ev);
    it->second.clear();
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  int w = win_w, h = win_h;
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  char win_id[32];
  snprintf(win_id, sizeof(win_id), "##popup_%d", pw->id);
  ImGui::Begin(win_id, nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

  // Top bar
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
  bool close_clicked = ImGui::Button("Close");
  ImGui::SameLine();

  // Info
  TcrSessionHandle popup_session = static_cast<TcrSessionHandle>(pw->session);
  if (pw->is_sync) {
    ImGui::Text("Sync: %zu instances", pw->sync_ids.size());
  } else {
    std::string sid =
        pw->instance_id.size() > 30 ? "..." + pw->instance_id.substr(pw->instance_id.size() - 27) : pw->instance_id;
    ImGui::Text("Instance: %s", sid.c_str());
  }
  ImGui::SameLine();
  const bool closed = !pw->close_reason.empty();
  ImGui::TextColored(
      pw->connected ? ImVec4(0.3f, 1, 0.3f, 1) : (closed ? ImVec4(1, 0.3f, 0.3f, 1) : ImVec4(1, 1, 0.3f, 1)),
      pw->connected ? "Connected" : (closed ? "Closed" : "Connecting..."));

  ImGui::SameLine(0, 20);
  auto sendKey = [&](const char* label, int code) {
    if (ImGui::Button(label) && popup_session) {
      tcr_session_send_keyboard_event(popup_session, code, true);
      tcr_session_send_keyboard_event(popup_session, code, false);
    }
  };
  sendKey("Back", 158);
  ImGui::SameLine();
  sendKey("Home", 172);
  ImGui::SameLine();
  sendKey("Menu", 139);
  ImGui::PopStyleVar();

  float bar_h = ImGui::GetCursorPosY();
  ImVec2 avail((float)w, (float)h - bar_h);

  if (pw->renderer && pw->renderer->has_frame()) {
    float tex_w = (float)pw->renderer->frame_width();
    float tex_h = (float)pw->renderer->frame_height();
    float rot = pw->rotation_angle;
    int vid_w = pw->video_width;
    int vid_h = pw->video_height;

    // If no screen config received yet, use texture size as video size
    if (vid_w <= 0 || vid_h <= 0) {
      vid_w = (int)tex_w;
      vid_h = (int)tex_h;
    }

    // Calculate display size considering rotation
    // For 90/270 rotation, the displayed aspect ratio is swapped (height/width)
    float display_w, display_h;
    bool is_rotated = (rot == 90 || rot == 270);
    if (is_rotated) {
      display_w = (float)vid_h;  // swapped
      display_h = (float)vid_w;
    } else {
      display_w = (float)vid_w;
      display_h = (float)vid_h;
    }

    // Fit to available area while maintaining aspect ratio
    float scale = std::min(avail.x / display_w, avail.y / display_h);
    ImVec2 isz(display_w * scale, display_h * scale);

    // Center in available area
    float px = (avail.x - isz.x) * 0.5f;
    float py = (avail.y - isz.y) * 0.5f;
    ImGui::SetCursorPos(ImVec2(px, bar_h + py));
    ImVec2 ip = ImGui::GetCursorScreenPos();

    // Render video (texture is always unrotated I420, rotation is display-only)
    // For now, render the texture as-is with aspect ratio correction
    // The GPU texture contains the raw frame; rotation would need shader support
    // So we render fitting the rotated aspect ratio
    ImGui::Image((ImTextureID)(intptr_t)pw->renderer->get_texture_id(), isz, ImVec2(0, 1), ImVec2(1, 0));

    // Touch input: transform view coordinates to cloud screen coordinates
    if (ImGui::IsItemHovered() && popup_session) {
      ImVec2 mouse = ImGui::GetMousePos();
      // Normalized coordinates in the view [0, 1]
      float nx = (mouse.x - ip.x) / isz.x;
      float ny = (mouse.y - ip.y) / isz.y;
      nx = std::max(0.0f, std::min(1.0f, nx));
      ny = std::max(0.0f, std::min(1.0f, ny));

      // Transform based on rotation (view coords -> cloud screen coords)
      // Cloud screen is always vid_w x vid_h (physical, unrotated)
      int cx, cy;
      if (rot == 90) {
        // Client rotated 90 CW to compensate cloud 270
        cx = (int)(ny * vid_w);
        cy = (int)((1.0f - nx) * vid_h);
      } else if (rot == 180) {
        cx = (int)((1.0f - nx) * vid_w);
        cy = (int)((1.0f - ny) * vid_h);
      } else if (rot == 270) {
        // Client rotated 270 CW to compensate cloud 90
        cx = (int)((1.0f - ny) * vid_w);
        cy = (int)(nx * vid_h);
      } else {
        cx = (int)(nx * vid_w);
        cy = (int)(ny * vid_h);
      }

      if (ImGui::IsMouseClicked(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 0, vid_w, vid_h, (int64_t)SDL_GetTicks());
      if (ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 1, vid_w, vid_h, (int64_t)SDL_GetTicks());
      if (ImGui::IsMouseReleased(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 2, vid_w, vid_h, (int64_t)SDL_GetTicks());
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.0f) tcr_session_send_mouse_scroll(popup_session, wheel > 0 ? 1.0f : -1.0f);
    }
  } else if (!pw->close_reason.empty()) {
    // 会话已被关闭（拉流/连接失败等）：显式展示 SDK 带回的关闭原因，避免只显示 "Waiting for video"
    ImGui::SetCursorPos(ImVec2(avail.x * 0.1f, bar_h + avail.y * 0.4f));
    ImGui::PushTextWrapPos(avail.x * 0.9f);
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "Session closed: %s", pw->close_reason.c_str());
    ImGui::TextWrapped("Instance: %s", pw->instance_id.c_str());
    ImGui::PopTextWrapPos();
  } else {
    ImGui::SetCursorPos(ImVec2(avail.x * 0.3f, bar_h + avail.y * 0.45f));
    ImGui::Text("Waiting for video...");
  }

  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(pw->sdl_window);

  ImGui::SetCurrentContext(m_main_imgui_ctx);
  SDL_GL_MakeCurrent(m_window, m_gl_context);

  if (close_clicked) pw->close_requested = true;
#endif
}

// =============================================================================
// Config selection
// =============================================================================

void App::select_config(int index) {
  if (index < 0 || index >= (int)m_config_names.size()) return;
  std::string path = m_config_dir + m_config_names[index] + ".json";
  if (m_config.load(path)) {
    m_selected_config = index;
    // 用配置中的 instanceIds 刷新可编辑输入框
    std::memset(m_instance_ids_buf, 0, sizeof(m_instance_ids_buf));
    std::strncpy(m_instance_ids_buf, m_config.instance_ids.c_str(), sizeof(m_instance_ids_buf) - 1);
    // 用配置中的 gridCellWidth 刷新可编辑输入框
    std::memset(m_grid_cell_width_buf, 0, sizeof(m_grid_cell_width_buf));
    snprintf(m_grid_cell_width_buf, sizeof(m_grid_cell_width_buf), "%d", m_config.grid_cell_width);
    LOG_INFO("App", "Selected config [%d] %s (instances=%zu)", index, m_config_names[index].c_str(),
             m_config.get_instance_id_list().size());
  } else {
    LOG_WARN("App", "Failed to load config %s", path.c_str());
  }
}

// =============================================================================
// Token
// =============================================================================

void App::request_token() {
  if (m_state == AppState::REQUESTING_TOKEN) return;
  m_state = AppState::REQUESTING_TOKEN;
  m_error_message.clear();
  // 将输入框中的 instanceIds 同步回配置（用户可能已修改）
  m_config.instance_ids = m_instance_ids_buf;
  // 将输入框中的 gridCellWidth 同步回配置（用户可能已覆盖 json 默认值）
  int cell_width = std::atoi(m_grid_cell_width_buf);
  if (cell_width > 0) m_config.grid_cell_width = cell_width;
  auto ids = m_config.get_instance_id_list();
  if (ids.empty()) {
    m_error_message = "No instance IDs";
    m_state = AppState::TOKEN_PAGE;
    return;
  }
  nlohmann::json req;
  req["RequestId"] = "imgui-" + std::to_string(SDL_GetTicks());
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& id : ids) arr.push_back(id);
  req["AndroidInstanceIds"] = arr;
  if (m_config.app_id != 0) req["AppId"] = m_config.app_id;
  std::string url = m_config.base_url + m_config.api_path;
  std::string body = req.dump();
  std::thread([this, url, body]() {
    HttpResponse r = http_post(url, body);
    if (!r.ok()) {
      m_error_message = "HTTP " + std::to_string(r.status_code);
      m_state = AppState::TOKEN_PAGE;
      return;
    }
    try {
      auto j = nlohmann::json::parse(r.body);
      if (j.contains("Error")) {
        m_error_message = j["Error"].value("Code", "") + ": " + j["Error"].value("Message", "");
        m_state = AppState::TOKEN_PAGE;
        return;
      }
      m_token = j.value("Token", "");
      m_access_info = j.value("AccessInfo", "");
      if (m_token.empty() || m_access_info.empty()) {
        m_error_message = "Missing Token/AccessInfo";
        m_state = AppState::TOKEN_PAGE;
        return;
      }
      start_multi_streaming();
    } catch (const std::exception& e) {
      m_error_message = e.what();
      m_state = AppState::TOKEN_PAGE;
    }
  }).detach();
}

// =============================================================================
// Multi-stream
// =============================================================================

void App::start_multi_streaming() {
  m_error_message.clear();
  m_tcr_client = tcr_client_get_instance();
  if (!m_tcr_client) {
    m_error_message = "No TcrClient";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  TcrConfig cfg = tcr_config_default();
  cfg.token = m_token.c_str();
  cfg.accessInfo = m_access_info.c_str();
  cfg.hardwareDecode = m_config.hardware_decode;
  LOG_INFO("App", "tcr_client_init hardwareDecode=%d", cfg.hardwareDecode ? 1 : 0);
  if (tcr_client_init(static_cast<TcrClientHandle>(m_tcr_client), &cfg) != TCR_SUCCESS) {
    m_error_message = "tcr_client_init failed";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  m_tcr_instance = tcr_client_get_android_instance(static_cast<TcrClientHandle>(m_tcr_client));
  m_all_instance_ids = m_config.get_instance_id_list();
  m_instance_states.clear();
  m_current_streaming_ids.clear();
  m_checked_instances.clear();
  for (const auto& id : m_all_instance_ids) m_instance_states[id] = InstanceState::Connecting;

  // 在创建 session 前，先按当前窗口尺寸算好动态并发上限（= 窗口可平铺的子流画面数量）。
  // grid 可用区域与 render_multi_stream_page 保持一致：顶栏 44，底栏 28。
  {
    int win_w = 0, win_h = 0;
    SDL_GetWindowSize(m_window, &win_w, &win_h);
    float aw = (float)win_w;
    float ah = (float)win_h - 44.0f - 28.0f;
    recompute_concurrent_streaming(aw, ah);
  }

  create_multi_session();
  access_all_instances();
  m_state = AppState::MULTI_STREAM;
}

void App::create_multi_session() {
  close_session();

  TcrSessionConfig c = tcr_session_config_default();
  c.stream_profile.video_width = m_config.video_width;
  c.stream_profile.fps = m_config.video_fps;
  c.stream_profile.min_bitrate = m_config.video_min_bitrate;
  c.stream_profile.max_bitrate = m_config.video_max_bitrate;
  // 并发数由窗口尺寸动态计算（recompute_concurrent_streaming 已在上游调用时算出）。
  // 保证 concurrentStreamingInstances 始终等于当前窗口可渲染的子流画面数量。
  c.concurrentStreamingInstances = m_computed_concurrent > 0 ? m_computed_concurrent : m_config.concurrent_streaming;
  m_session_concurrent_limit = c.concurrentStreamingInstances;

  m_tcr_session = tcr_client_create_session(static_cast<TcrClientHandle>(m_tcr_client), &c);
  if (!m_tcr_session) {
    m_error_message = "create_session failed";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  TcrSessionObserver* o = reinterpret_cast<TcrSessionObserver*>(m_session_observer_storage);
  memset(o, 0, sizeof(TcrSessionObserver));
  o->user_data = this;
  o->on_event = reinterpret_cast<decltype(o->on_event)>(&App::on_multi_session_event);
  tcr_session_set_observer(static_cast<TcrSessionHandle>(m_tcr_session), o);

  TcrVideoFrameObserver* v = reinterpret_cast<TcrVideoFrameObserver*>(m_video_frame_observer_storage);
  memset(v, 0, sizeof(TcrVideoFrameObserver));
  v->user_data = this;
  v->on_frame = reinterpret_cast<decltype(v->on_frame)>(&App::on_multi_video_frame);
  tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(m_tcr_session), v);
}

void App::access_all_instances() {
  if (!m_tcr_session || m_all_instance_ids.empty()) return;
  std::vector<const char*> p;
  for (const auto& id : m_all_instance_ids) p.push_back(id.c_str());

  // 首屏要出流的实例随接入请求一起下发，不再等连上后补一次 switch：
  // 刚 connected 时 uploader 未必已连上 edge，那一跳的 switch 可能被 edge 丢掉。
  // 滚动位置为 0 时的可见区就是列表前 concurrentStreamingInstances 个，无需再算几何。
  const int lim = m_computed_concurrent > 0 ? m_computed_concurrent : m_config.concurrent_streaming;
  std::vector<const char*> dp;
  m_current_streaming_ids.clear();
  for (size_t i = 0; i < m_all_instance_ids.size() && (int)i < lim; ++i) {
    dp.push_back(m_all_instance_ids[i].c_str());
    m_current_streaming_ids.insert(m_all_instance_ids[i]);
  }

  LOG_INFO("App", "access_multi_stream_ex: instances=%zu, default_streaming=%zu", p.size(), dp.size());
  tcr_session_access_multi_stream_ex(static_cast<TcrSessionHandle>(m_tcr_session), p.data(), (int32_t)p.size(),
                                     dp.data(), (int32_t)dp.size());
}

void App::close_session() {
  if (m_tcr_session) {
    tcr_session_set_observer(static_cast<TcrSessionHandle>(m_tcr_session), nullptr);
    tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(m_tcr_session), nullptr);
    if (m_tcr_client)
      tcr_client_destroy_session(static_cast<TcrClientHandle>(m_tcr_client),
                                 static_cast<TcrSessionHandle>(m_tcr_session));
    m_tcr_session = nullptr;
  }
  m_multi_frame_cache.clear();
  m_current_streaming_ids.clear();
}

// =============================================================================
// Multi-stream callbacks
// =============================================================================

void App::on_multi_session_event(void* ud, int ev, const char* d) {
  App* s = static_cast<App*>(ud);
  if (!s || s->m_is_destroying.load(std::memory_order_acquire)) return;
  if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_STATE_CONNECTED) {
    // 会话连上不代表所有实例都出流。这里只把「还未出流」的实例置为 Connecting，
    // 真正的 On 由视频帧回调和切换完成事件里的 actual 驱动。
    for (auto& kv : s->m_instance_states) {
      if (kv.second != InstanceState::Connected) kv.second = InstanceState::Connecting;
    }
  } else if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_STATE_CLOSED) {
    for (auto& kv : s->m_instance_states) kv.second = InstanceState::Offline;
    {
      std::lock_guard<std::mutex> lock(s->m_instance_offline_mutex);
      s->m_instance_fail_reason.clear();
    }
    s->m_state = AppState::DISCONNECTED;
    s->m_error_message = d ? d : "closed";
  } else if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_CLIENT_STATS && d) {
    s->m_client_stats = d;
  } else if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_STREAMING_SWITCH_COMPLETED && d) {
    // 每次 switch 恰好一次回执。status 只描述「云端往返是否完成」：
    //   success = 已答复，failed 里是服务端给出的实例级结果
    //   timeout / fail = 没完成往返，其余字段为空，状态保持原样（原因看日志）
    try {
      auto j = nlohmann::json::parse(d);
      const std::string status = j.value("status", "");
      LOG_INFO("App", "switch completed: %s", d);
      if (status != "success") {
        return;
      }

      std::lock_guard<std::mutex> lock(s->m_instance_offline_mutex);
      // 记录服务端给出的失败原因原文，渲染时直接展示，不做映射。
      if (j.contains("failed") && j["failed"].is_array()) {
        for (const auto& item : j["failed"]) {
          if (!item.contains("instance_id")) continue;
          s->m_instance_fail_reason[item["instance_id"].get<std::string>()] = item.value("reason", "");
        }
      }
      // 本次切换成功的实例说明已正常，清掉失败原因（切回来就恢复显示）
      if (j.contains("succeeded") && j["succeeded"].is_array()) {
        for (const auto& item : j["succeeded"]) {
          s->m_instance_fail_reason.erase(item.get<std::string>());
        }
      }
      // 2. actual 是 SDK 认为在出流的实例集合。不在其中的说明当前没有出流意图，
      //    清掉其 offline 标记即可（offline 标记只描述服务端答复，不代表出流意图）。
      //    文本状态的 On/Off 由渲染时依据是否有帧推导，这里不改。
    } catch (...) {
    }
  }
}

void App::on_multi_video_frame(void* ud, void* fh) {
  App* s = static_cast<App*>(ud);
  if (!s || !fh || s->m_is_destroying.load(std::memory_order_acquire)) return;
  TcrVideoFrameHandle h = static_cast<TcrVideoFrameHandle>(fh);
  const TcrVideoFrameBuffer* b = tcr_video_frame_get_buffer(h);
  if (!b) return;

  std::string id;
  if (b->instance_id) id = b->instance_id;
  if (id.empty()) return;

  // per-instance 帧统计：记录本 2s 统计段内每路收到的帧数（性能测试主指标）。
  // 此回调在 SDK 解码线程执行，须加锁保护跨线程读写。
  {
    std::lock_guard<std::mutex> lock(s->m_frame_stat_mutex);
    s->m_frame_count_window[id]++;
    s->m_frame_count_total[id]++;
  }

  // 该实例已有帧到达，说明它当前是在出流的，清掉 offline 标记。
  // 注意：帧回调只负责「清标记」，文本状态由渲染时依据 m_frame_count_total 推导，
  // 避免在解码线程频繁改写状态导致 UI 抖动。画面渲染不依赖这里。
  {
    std::lock_guard<std::mutex> lock(s->m_instance_offline_mutex);
    s->m_instance_fail_reason.erase(id);
  }

  if (b->type == TCR_VIDEO_BUFFER_TYPE_GPU) {
    // 硬件解码：GPU 纹理，零拷贝交给渲染线程
    s->m_frames_gpu.fetch_add(1, std::memory_order_relaxed);
    tcr_video_frame_add_ref(h);
    s->m_multi_frame_cache.push(id, VideoFrame(h, b->buffer.gpu, b->timestamp_us));
    return;
  }

  if (b->type != TCR_VIDEO_BUFFER_TYPE_I420) return;
  s->m_frames_i420.fetch_add(1, std::memory_order_relaxed);
  tcr_video_frame_add_ref(h);
  const TcrI420Buffer& i = b->buffer.i420;
  s->m_multi_frame_cache.push(id, VideoFrame(h, i.data_y, i.data_u, i.data_v, i.stride_y, i.stride_u, i.stride_v,
                                             i.width, i.height, b->timestamp_us));
}

// =============================================================================
// Batch render & stream switching
// =============================================================================

void App::batch_render_frames() {
  auto frames = m_multi_frame_cache.swap_all();
  // 上传耗时单独计时：它发生在 update() 内部，不扣掉会被算进 ui 段
  const Uint64 up_start = SDL_GetPerformanceCounter();
  for (auto& kv : frames) {
    if (!kv.second.valid()) continue;
    VideoRenderer* r = get_or_create_renderer(kv.first);
    if (!r) continue;
    if (kv.second.is_gpu) {
      // 硬解路径：失败时不回退（GPU 帧没有 I420 数据可用），仅告警一次
      if (!r->upload_gpu_frame(kv.second.gpu)) {
        static bool warned = false;
        if (!warned) {
          warned = true;
          LOG_WARN("App", "upload_gpu_frame failed (platform=%d format=%d), frames will not render",
                   (int)kv.second.gpu.platform, (int)kv.second.gpu.pixel_format);
        }
      }
    } else {
      r->upload_frame(kv.second.data_y, kv.second.data_u, kv.second.data_v, kv.second.stride_y, kv.second.stride_u,
                      kv.second.stride_v, kv.second.width, kv.second.height);
    }
  }
  if (!frames.empty()) {
    m_upload_ms_frame +=
        (double)(SDL_GetPerformanceCounter() - up_start) * 1000.0 / (double)SDL_GetPerformanceFrequency();
    m_window_uploads += frames.size();
  }
}

VideoRenderer* App::get_or_create_renderer(const std::string& id) {
  auto it = m_instance_renderers.find(id);
  if (it != m_instance_renderers.end()) return it->second;
  VideoRenderer* r = new VideoRenderer();
  if (!r->init()) {
    delete r;
    return nullptr;
  }
  m_instance_renderers[id] = r;
  return r;
}

std::vector<std::string> App::calculate_visible_instances(float sy, float vh, float ch) {
  if (ch <= 0 || m_all_instance_ids.empty()) return {};
  int c = m_grid_columns;
  int sr = std::max(0, (int)(sy / ch));
  int er = (int)((sy + vh) / ch) + 1;
  int si = std::max(0, sr * c);
  int ei = std::min(er * c, (int)m_all_instance_ids.size());
  std::vector<std::string> v;
  for (int i = si; i < ei; ++i) v.push_back(m_all_instance_ids[i]);
  return v;
}

void App::switch_streaming_instances(const std::vector<std::string>& ids) {
  if (!m_tcr_session || ids.empty()) return;
  // 并发上限由窗口尺寸动态计算得到；若尚未计算（理论上不会），回退到 config 默认值。
  int lim = m_computed_concurrent > 0 ? m_computed_concurrent : m_config.concurrent_streaming;
  std::vector<const char*> p;
  for (size_t i = 0; i < ids.size() && (int)i < lim; ++i) p.push_back(ids[i].c_str());
  tcr_session_switch_streaming_instances(static_cast<TcrSessionHandle>(m_tcr_session), p.data(), (int32_t)p.size());
  m_current_streaming_ids.clear();
  for (const auto& id : p) m_current_streaming_ids.insert(id);
}

void App::recompute_concurrent_streaming(float avail_w, float avail_h) {
  // 格子尺寸固定：单个子流画面的宽度由 config.grid_cell_width 决定，
  // 视频区 16:9，含左右内边距 12、底部状态条（显示实例 ID 时 30，否则 18）、格子间距 6
  // （常量与 render_multi_stream_page 保持一致）。
  const float cell_vw = (float)m_config.grid_cell_width;
  const float cell_vh = cell_vw * 16.0f / 9.0f;
  const float cw = cell_vw + 12.0f;
  const float ch = cell_vh + (m_config.show_instance_id ? 30.0f : 18.0f);
  const float sp = 6.0f;

  int cols = std::max(1, (int)((avail_w + sp) / (cw + sp)));
  int rows = std::max(1, (int)((avail_h + sp) / (ch + sp)));
  int n = cols * rows;

  // 每格在屏幕上的实际像素就是画质上限：按 2x（给 Retina / 放大留余量）限制每路
  // RGBA 纹理尺寸。数百路平铺时这是显存占用的决定性因素——不限制的话每路都会按
  // 解码分辨率留一张全尺寸 RGBA，500 路就是几百 MB 的纯浪费。
  VideoRenderer::set_max_output_size((int)(cell_vw * 2), (int)(cell_vh * 2));

  m_grid_columns = cols;
  m_computed_concurrent = n;
  m_last_grid_display_w = avail_w;
  m_last_grid_display_h = avail_h;

  LOG_INFO("App", "recompute_concurrent_streaming: avail=%.0fx%.0f cell=%.0f cols=%d rows=%d concurrent=%d", avail_w,
           avail_h, cell_vw, cols, rows, n);
}

void App::rebuild_session_with_new_limit() {
  if (!m_tcr_client || m_all_instance_ids.empty()) return;
  LOG_INFO("App", "rebuild_session_with_new_limit: concurrent %d -> %d", m_session_concurrent_limit,
           m_computed_concurrent);
  create_multi_session();  // 内部会 close_session 并用新的 m_computed_concurrent 重建
  access_all_instances();  // 新并发上限下的首屏出流实例随接入请求下发
}

// =============================================================================
// Page 1: Token
// =============================================================================

void App::render_token_page() {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 ws(640, 480);
  ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - ws.x) * 0.5f, (io.DisplaySize.y - ws.y) * 0.5f), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ws, ImGuiCond_Always);
  ImGui::Begin(
      "##token", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
  ImGui::TextColored(ImVec4(0.1f, 0.46f, 0.82f, 1), "TcrSDK ImGui Demo");
  ImGui::Separator();

  // 配置选择下拉框
  if (m_config_names.empty()) {
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "No config found in config/ directory");
  } else {
    ImGui::Text("Config:");
    ImGui::SameLine();
    if (ImGui::BeginCombo("##config_combo",
                          m_selected_config >= 0 ? m_config_names[m_selected_config].c_str() : "Select")) {
      for (int i = 0; i < (int)m_config_names.size(); ++i) {
        bool selected = (i == m_selected_config);
        if (ImGui::Selectable(m_config_names[i].c_str(), selected)) {
          select_config(i);
        }
        if (selected) ImGui::SetItemDefaultFocus();
      }
      ImGui::EndCombo();
    }

    // instanceIds 可编辑输入框（默认值来自选中配置，用户可修改，支持多行）
    ImGui::Text("Instance IDs (comma-separated):");
    ImGui::InputTextMultiline("##instance_ids", m_instance_ids_buf, sizeof(m_instance_ids_buf), ImVec2(-1, 140),
                              ImGuiInputTextFlags_AllowTabInput);

    // gridCellWidth 可编辑输入框（默认值来自选中配置，用户可覆盖）
    ImGui::Text("Grid Cell Width (px):");
    ImGui::InputText("##grid_cell_width", m_grid_cell_width_buf, sizeof(m_grid_cell_width_buf));

    ImGui::Text("BaseUrl: %s", m_config.base_url.c_str());
    ImGui::Text("ApiPath: %s", m_config.api_path.c_str());

    // 实时统计当前输入框中的实例数量
    size_t count = split_comma_separated(m_instance_ids_buf).size();
    ImGui::Text("Total instances: %zu", count);
  }

  if (!m_error_message.empty()) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", m_error_message.c_str());
  bool busy = (m_state == AppState::REQUESTING_TOKEN);
  if (busy) ImGui::BeginDisabled();
  if (ImGui::Button(busy ? "Requesting..." : "Create Access Token", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
    request_token();
  if (busy) ImGui::EndDisabled();
  ImGui::End();
}

// =============================================================================
// Page 2: Multi-instance grid
// =============================================================================

void App::render_multi_stream_page(float dt) {
  ImGuiIO& io = ImGui::GetIO();

  // Top bar
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 44));
  ImGui::Begin(
      "##top", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
  if (ImGui::Button("Back")) {
    close_all_popups();
    close_session();
    m_state = AppState::TOKEN_PAGE;
  }
  ImGui::SameLine();
  int conn = 0;
  {
    std::lock_guard<std::mutex> lock(m_instance_offline_mutex);
    for (const auto& kv : m_instance_states)
      if (kv.second == InstanceState::Connected) conn++;
  }
  // InstanceOffline 不算 connected，保持计数语义为"会话已连接"
  ImGui::Text("Connected: %d/%zu | Streaming: %zu", conn, m_all_instance_ids.size(), m_current_streaming_ids.size());
  ImGui::SameLine(0, 20);
  if (ImGui::Button(m_checked_instances.size() == m_all_instance_ids.size() ? "Deselect All" : "Select All")) {
    if (m_checked_instances.size() == m_all_instance_ids.size())
      m_checked_instances.clear();
    else
      for (const auto& id : m_all_instance_ids) m_checked_instances.insert(id);
  }
  ImGui::SameLine();
  ImGui::Text("Sel: %zu", m_checked_instances.size());
  ImGui::SameLine(0, 20);
  if (m_checked_instances.empty()) ImGui::BeginDisabled();
  if (ImGui::Button("Sync Ops")) open_sync_popup(m_checked_instances);
  if (m_checked_instances.empty()) ImGui::EndDisabled();
  ImGui::End();

  // Grid
  float gt = 44, gb = io.DisplaySize.y - 28, gh = gb - gt;
  ImGui::SetNextWindowPos(ImVec2(0, gt));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, gh));
  ImGui::Begin("##grid", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

  // 固定格子尺寸（视频区 16:9），格子尺寸由 config.grid_cell_width 决定，
  // 列数/行数按窗口可用宽高动态计算，concurrentStreaming = cols * rows。
  const float cell_vw = (float)m_config.grid_cell_width;  // 视频区固定宽度
  const float cell_vh = cell_vw * 16.0f / 9.0f;           // 视频区固定高度
  const float cw = cell_vw + 12.0f;                       // 格子总宽（含左右内边距）
  // 底部状态条高度：显示实例 ID 时需要更高以容纳文本，否则只保留复选框/状态标签。
  const float bar_h = m_config.show_instance_id ? 30.0f : 18.0f;
  const float ch = cell_vh + bar_h;  // 格子总高（含底部状态条）
  const float sp = 6.0f;             // 格子间距
  const float vw = cell_vw;
  const float vh = cell_vh;

  float aw = ImGui::GetContentRegionAvail().x;
  // 检测窗口尺寸变化（resize）：若可用宽高变化，则重算并发上限。
  // 若新上限超过当前 session 的 SSRC 池，需要重建 session（更大的 concurrentStreamingInstances）。
  float grid_gh = gh;
  if (aw != m_last_grid_display_w || grid_gh != m_last_grid_display_h) {
    recompute_concurrent_streaming(aw, grid_gh);
    if (m_computed_concurrent > m_session_concurrent_limit) {
      rebuild_session_with_new_limit();
    }
  }
  int cols = m_grid_columns;

  // 可见行裁剪：实例列表可达数千条，而窗口一次只放得下十几行。
  // 不裁剪的话每帧都要为列表里每个实例做 5~6 次 map 查找 + 控件构建，
  // UI 耗时随列表长度线性增长（实测 620 条 0.68ms / 2831 条 1.39ms），
  // 和「同时出流多少路」无关，纯属浪费。这里只遍历滚动可见区内的行。
  const size_t total_cells = m_all_instance_ids.size();
  const int total_rows = cols > 0 ? (int)((total_cells + cols - 1) / cols) : 0;
  const float row_h = ch + sp;
  const float scroll_y = ImGui::GetScrollY();
  // 上下各多画一行，避免滚动时边缘闪烁
  int first_row = std::max(0, (int)(scroll_y / row_h) - 1);
  int last_row = std::min(total_rows, (int)((scroll_y + gh) / row_h) + 2);

  // 用一个占位 Dummy 把被跳过的前置行高度补回来，保证滚动条范围和滚动位置不变
  if (first_row > 0) ImGui::Dummy(ImVec2(1, first_row * row_h));

  const size_t begin_idx = (size_t)first_row * cols;
  const size_t end_idx = std::min(total_cells, (size_t)last_row * cols);
  m_window_cells = (int)(end_idx - begin_idx);

  // 渲染可见区实例（带滚动），滚动时切流到可见区。concurrentStreaming 作为同时出流上限。
  for (size_t i = begin_idx; i < end_idx; ++i) {
    if (i > begin_idx && (i % cols) != 0) ImGui::SameLine(0, sp);

    const std::string& id = m_all_instance_ids[i];
    bool ck = m_checked_instances.count(id) > 0;

    ImGui::PushID((int)i);
    ImGui::BeginGroup();

    ImVec2 c0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRect(c0, ImVec2(c0.x + cw, c0.y + ch), IM_COL32(60, 60, 60, 255));

    VideoRenderer* r = m_instance_renderers.count(id) ? m_instance_renderers[id] : nullptr;
    if (r && r->has_frame()) {
      float tw = (float)r->frame_width(), th = (float)r->frame_height();
      float sc = std::min(vw / tw, vh / th);
      ImVec2 isz(tw * sc, th * sc);
      float px = (vw - isz.x) * 0.5f, py = (vh - isz.y) * 0.5f;
      ImGui::SetCursorScreenPos(ImVec2(c0.x + 6 + px, c0.y + py));
      ImGui::Image((ImTextureID)(intptr_t)r->get_texture_id(), isz, ImVec2(0, 1), ImVec2(1, 0));
      if (ImGui::IsItemClicked(0)) open_instance_popup(id);
    } else {
      ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(c0.x + 1, c0.y + 1), ImVec2(c0.x + vw, c0.y + vh),
                                                IM_COL32(30, 30, 30, 255));
      // 小流没画面不代表大流拉不到：占位区也要可点击，用于验证单实例大流是否正常
      ImGui::SetCursorScreenPos(ImVec2(c0.x + 1, c0.y + 1));
      ImGui::InvisibleButton("##cell_click", ImVec2(vw, vh));
      if (ImGui::IsItemClicked(0)) open_instance_popup(id);
    }

    ImGui::SetCursorScreenPos(ImVec2(c0.x + 4, c0.y + vh + 2));

    // 文本只用于展示，与画面渲染完全解耦：
    // 画面显示只看 VideoRenderer::has_frame()，不看这里推导出的任何状态。
    // 优先级：有帧 > 服务端给的失败原因 > 会话/出流状态。
    InstanceState st = InstanceState::Offline;
    std::string label;
    ImVec4 label_color(1, .3f, .3f, 1);
    {
      std::lock_guard<std::mutex> lock(m_instance_offline_mutex);
      bool timeline_has_frame = false;
      {
        std::lock_guard<std::mutex> fl(m_frame_stat_mutex);
        auto it = m_frame_count_total.find(id);
        timeline_has_frame = (it != m_frame_count_total.end() && it->second > 0);
      }
      auto reason_it = m_instance_fail_reason.find(id);
      if (timeline_has_frame) {
        // 有帧说明实际在出流，不管服务端之前回过什么，都显示 On
        st = InstanceState::Connected;
        label = "On";
        label_color = ImVec4(.3f, 1, .3f, 1);
      } else if (reason_it != m_instance_fail_reason.end() && !reason_it->second.empty()) {
        // 直接展示服务端返回的原因原文（如 "instance offline"），不在此做映射改写
        st = InstanceState::InstanceOffline;
        label = reason_it->second;
        label_color = ImVec4(.55f, .55f, .55f, 1);
      } else {
        auto sit = m_instance_states.find(id);
        if (sit != m_instance_states.end()) st = sit->second;
        switch (st) {
          case InstanceState::Connected:
            label = "On";
            label_color = ImVec4(.3f, 1, .3f, 1);
            break;
          case InstanceState::Connecting:
            label = "未出流";
            label_color = ImVec4(1, 1, .3f, 1);
            break;
          default:
            label = "Off";
            label_color = ImVec4(1, .3f, .3f, 1);
            break;
        }
      }
    }

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    bool c = ck;
    if (ImGui::Checkbox("##cb", &c)) {
      if (c)
        m_checked_instances.insert(id);
      else
        m_checked_instances.erase(id);
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();
    ImGui::TextColored(label_color, "%s", label.c_str());
    if (m_config.show_instance_id) {
      ImGui::SameLine();
      std::string sid = id.size() > 16 ? "..." + id.substr(id.size() - 13) : id;
      ImGui::TextColored(ImVec4(.5f, .5f, .5f, 1), "%s", sid.c_str());
    }

    ImGui::SetCursorScreenPos(ImVec2(c0.x, c0.y + ch));
    ImGui::Dummy(ImVec2(cw, 0));
    ImGui::EndGroup();
    ImGui::PopID();
  }

  // 补回被跳过的后置行高度，否则滚动条会认为内容只到可见区底部
  if (last_row < total_rows) ImGui::Dummy(ImVec2(1, (total_rows - last_row) * row_h));

  // Scroll debounce：滚动时切流到当前可见区（受 concurrentStreaming 上限截断）。
  float cs = ImGui::GetScrollY();
  if (cs != m_prev_scroll_y) {
    m_scroll_dirty = true;
    m_debounce_timer = 0;
    m_prev_scroll_y = cs;
  }
  if (m_scroll_dirty) {
    m_debounce_timer += dt;
    if (m_debounce_timer >= 0.5f) {
      auto vis = calculate_visible_instances(cs, gh, ch);
      std::set<std::string> vs(vis.begin(), vis.end());
      if (vs != m_current_streaming_ids && !vis.empty()) switch_streaming_instances(vis);
      m_scroll_dirty = false;
    }
  }
  // 兜底：首屏出流实例已随 access_multi_stream_ex 下发并预置到 m_current_streaming_ids，
  // 正常路径不会进这里；只在出流集合被意外清空时按可见区补一次。
  if (m_current_streaming_ids.empty() && !m_all_instance_ids.empty()) {
    auto vis = calculate_visible_instances(0, gh, ch);
    if (!vis.empty()) switch_streaming_instances(vis);
  }
  ImGui::End();

  // Status bar
  ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 28));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 28));
  ImGui::Begin(
      "##st", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
  ImGui::Text("Instances: %zu | Connected: %d | Streaming: %zu | Selected: %zu | Popups: %zu",
              m_all_instance_ids.size(), conn, m_current_streaming_ids.size(), m_checked_instances.size(),
              m_popups.size());
  ImGui::End();
}
