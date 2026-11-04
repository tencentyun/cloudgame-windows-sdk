#include "diagnostics.h"

#include <imgui.h>
#include <SDL.h>

#include <algorithm>
#include <string>

#include "app.h"
#include "logger.h"
#include "video_renderer.h"

// 当前进程常驻内存（MB）。只用于在 LoopStats 里给个量级参考；
// 精确的 GPU 显存归因要看 DXGI QueryVideoMemoryInfo / PIX，不能拿 RSS 代替。
#if defined(_WIN32)
// NOMINMAX 必须在任何 windows.h 之前：windows.h 会把 min/max 定义成宏，
// 之后所有 std::min/std::max 都会解析失败（报 illegal token on right side of '::'）。
// TcrSdkProject 顶层 CMakeLists 里加的 NOMINMAX 不会传到 Demo，这里自己兜住。
#  ifndef NOMINMAX
#    define NOMINMAX
#  endif
// windows.h 必须在 psapi.h 之前：psapi.h 用到 WINAPI/DWORD 等但没有自己包含 windows.h，
// 先包含 psapi.h 会报"missing ';' before identifier 'WINAPI'"。
// clang-format off：否则按字母序会把 psapi.h 排到 windows.h 前面，抵消这里的顺序要求。
// clang-format off
#  include <windows.h>
#  include <psapi.h>
// clang-format on
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

double now_ms() { return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency(); }

const int kStageCount = 5;
enum { kStageEvt = 0, kStageUpload, kStageUi, kStageGpu, kStageTail };

}  // namespace

// =============================================================================
// 主循环分阶段计时
// =============================================================================

void Diagnostics::begin_frame() {
  m_stage_ms.assign(kStageCount, 0.0);
  m_upload_ms_frame = 0.0;
  m_prev_stage_ms = now_ms();
}

void Diagnostics::mark_evt_begin() { m_prev_stage_ms = now_ms(); }

void Diagnostics::mark_evt_end() {
  const double t = now_ms();
  m_stage_ms[kStageEvt] += t - m_prev_stage_ms;
  m_prev_stage_ms = t;
}

void Diagnostics::mark_gpu_begin() {
  const double t = now_ms();
  double ui = t - m_prev_stage_ms - m_upload_ms_frame;
  m_stage_ms[kStageUi] += ui > 0 ? ui : 0.0;
  m_stage_ms[kStageUpload] += m_upload_ms_frame;
  m_prev_stage_ms = t;
}

void Diagnostics::mark_gpu_end() {
  const double t = now_ms();
  m_stage_ms[kStageGpu] += t - m_prev_stage_ms;
  m_prev_stage_ms = t;
}

void Diagnostics::end_frame() {
  const double t = now_ms();
  m_stage_ms[kStageTail] += t - m_prev_stage_ms;

  double total = 0;
  for (double v : m_stage_ms) total += v;
  m_loop_ms.push_back(total);
  for (int i = 0; i < kStageCount; ++i) m_stage_hist[i].push_back(m_stage_ms[i]);

  // 与 PerInstanceFrames 同为 2s 周期，便于对照
  const float elapsed = m_app.elapsed_seconds();
  if (m_loop_ms.size() >= 2 && elapsed - m_last_loop_log >= 2.0f) {
    m_last_loop_log = elapsed;
    log_loop_stats();
  }
}

void Diagnostics::on_decoded_frame(bool is_gpu) {
  (is_gpu ? m_frames_gpu : m_frames_i420).fetch_add(1, std::memory_order_relaxed);
}

void Diagnostics::on_frames_uploaded(double ms, size_t count) {
  m_upload_ms_frame += ms;
  m_window_uploads += count;
}

// =============================================================================
// 周期性日志
// =============================================================================

void Diagnostics::tick(float dt) {
  const float elapsed = m_app.elapsed_seconds();
  if (m_app.state() == AppState::MULTI_STREAM && m_stream_start_elapsed < 0) m_stream_start_elapsed = elapsed;
  if (m_stream_start_elapsed >= 0 && !m_coverage_10s_logged && elapsed - m_stream_start_elapsed >= 10.0f) {
    m_coverage_10s_logged = true;
    log_render_coverage(true);
  }

  m_stats_timer += dt;
  if (m_stats_timer < 2.0f) return;
  m_stats_timer = 0;
  log_frame_stats();
  log_render_coverage(false);

  // 画面正确性：帧计数正常但画面黑屏/错位时只能靠回读发现。
  // 默认关闭（GPU→CPU 同步回读是观测开销），只在取证轮打开。
  const AppConfig& cfg = m_app.config();
  if ((cfg.verify_luma || cfg.verify_luma_all) && m_app.state() == AppState::MULTI_STREAM) {
    verify_luma(cfg.verify_luma_all);
  }
}

// 不调用 sample_average_luma()——那是 GPU→CPU 回读，在多路（300 路）下是明显的观测开销污染源。
// per-instance 帧计数（PerInstanceFrames）作为主指标。
void Diagnostics::log_frame_stats() {
  LOG_INFO("FrameStats", "elapsed=%.1fs i420_frames=%llu gpu_frames=%llu", m_app.elapsed_seconds(),
           (unsigned long long)m_frames_i420.load(), (unsigned long long)m_frames_gpu.load());

  int active = 0, low_fps = 0, stalled = 0;
  const auto window = m_app.take_window_frame_counts();
  for (const auto& kv : window) {
    if (kv.second >= 2)
      active++;
    else if (kv.second == 1)
      low_fps++;
    else
      stalled++;
  }
  LOG_INFO("PerInstanceFrames", "active=%d low_fps=%d stalled=%d total=%zu", active, low_fps, stalled, window.size());
}

// 统计当前出流（switch_streaming_instances 选中）的实例里，有多少格子已经渲染出过画面。
// PerInstanceFrames 只能看到收到帧的实例，看不到"从未出帧"的格子，所以单独统计。
void Diagnostics::log_render_coverage(bool list_all_missing) {
  if (m_app.state() != AppState::MULTI_STREAM && m_app.state() != AppState::DISCONNECTED) return;
  if (m_stream_start_elapsed < 0) return;
  const auto& streaming = m_app.streaming_ids();
  const auto totals = m_app.frame_totals();
  auto total_of = [&totals](const std::string& id) -> uint64_t {
    auto it = totals.find(id);
    return it == totals.end() ? 0 : it->second;
  };

  size_t rendered = 0;
  std::vector<std::string> missing;
  for (const auto& id : streaming) {
    if (m_app.is_rendered(id))
      rendered++;
    else
      missing.push_back(id);
  }

  // 区分：SDK 已回调过解码帧但格子没画面（本地渲染链路问题） vs SDK 从未回调（解码前/服务端）
  size_t cb_but_no_render = 0;
  std::string cb_ids;
  for (const auto& id : missing) {
    if (uint64_t n = total_of(id)) {
      cb_but_no_render++;
      cb_ids += id + "(" + std::to_string(n) + ") ";
    }
  }
  if (list_all_missing || cb_but_no_render)
    LOG_INFO("RenderCoverage", "missing_with_sdk_callback=%zu %s", cb_but_no_render, cb_ids.c_str());

  if (list_all_missing) {
    // 收到了帧回调、但不在 Demo 期望出流集合里的实例（说明服务端推的集合 ≠ Demo 期望的集合）
    std::string extra;
    size_t n_extra = 0;
    for (const auto& kv : totals) {
      if (!streaming.count(kv.first)) {
        n_extra++;
        extra += kv.first + "(" + std::to_string(kv.second) + ") ";
      }
    }
    LOG_INFO("RenderCoverage", "callback_instances=%zu not_in_streaming_set=%zu %s", totals.size(), n_extra,
             extra.c_str());
  }

  std::string ms;
  size_t n = list_all_missing ? missing.size() : std::min<size_t>(missing.size(), 5);
  for (size_t i = 0; i < n; ++i) ms += (i ? "," : "") + missing[i];
  LOG_INFO("RenderCoverage", "since_stream=%.1fs streaming=%zu rendered=%zu missing=%zu [%s]%s",
           m_app.elapsed_seconds() - m_stream_start_elapsed, streaming.size(), rendered, missing.size(), ms.c_str(),
           n < missing.size() ? "..." : "");
}

// 分位数按 2s 窗口统计后打印，只看平均值会被偶发卡顿稀释。
void Diagnostics::log_loop_stats() {
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
  const float elapsed = m_app.elapsed_seconds();
  const double secs = elapsed - m_last_loop_log_start;
  const double p50 = pct(loop, 0.50), p95 = pct(loop, 0.95), p99 = pct(loop, 0.99);
  const double maxv = loop.empty() ? 0.0 : loop.back();

  LOG_INFO("LoopStats",
           "fps=%.1f frame_ms avg=%.2f p50=%.2f p95=%.2f p99=%.2f max=%.2f | "
           "evt=%.2f upload=%.2f ui=%.2f gpu=%.2f tail=%.2f | "
           "upload_p95=%.2f gpu_p95=%.2f | uploads=%llu cells=%d draw_cmds=%d verts=%d rss_mb=%zu renderers=%d",
           secs > 0.01 ? (double)m_loop_ms.size() / secs : 0.0, mean(loop), p50, p95, p99, maxv,
           mean(m_stage_hist[kStageEvt]), mean(m_stage_hist[kStageUpload]), mean(m_stage_hist[kStageUi]),
           mean(m_stage_hist[kStageGpu]), mean(m_stage_hist[kStageTail]), pct(m_stage_hist[kStageUpload], 0.95),
           pct(m_stage_hist[kStageGpu], 0.95), m_window_uploads, m_window_cells,
           ImGui::GetDrawData() ? ImGui::GetDrawData()->CmdListsCount : 0, ImGui::GetIO().MetricsRenderVertices,
           (size_t)current_rss_mb(), (int)m_app.renderers().size());

  m_loop_ms.clear();
  for (int i = 0; i < kStageCount; ++i) m_stage_hist[i].clear();
  m_window_uploads = 0;
  m_window_cells = 0;
  m_last_loop_log_start = elapsed;
}

// 回读每路输出纹理的平均亮度。
//
// 这条统计和 PerInstanceFrames 互补：共享 scratch + uvScale 如果算错，或者上传完
// 没解绑 RTV/SRV，帧计数一切正常但画面是黑屏或错位。判据有两条 ——
//   1. nonzero 路数 == 有帧的路数（否则就是黑屏）
//   2. 亮度值不是全同一个数（全相同说明采样位置算错，所有格子采到了同一块像素）
void Diagnostics::verify_luma(bool all) {
  std::vector<VideoRenderer*> targets;
  std::vector<std::string> ids;
  for (const auto& kv : m_app.renderers()) {
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
