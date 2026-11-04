#pragma once

// diagnostics.h - 性能与画面正确性的观测输出（不影响业务逻辑）
//
// 只做两件事：采集主循环各阶段耗时、每 2 秒往日志里打一批统计行。
// 日志行的 tag 与字段格式被性能脚本（TcrSdkProject/scripts/run_multistream_perf.py）解析，
// 改格式前先改脚本：
//   FrameStats        累计收到的软解 / 硬解帧数
//   PerInstanceFrames 本 2s 内每路帧数分布（active/low_fps/stalled）
//   RenderCoverage    出流集合里有多少格子已经出过画面
//   LoopStats         主循环 fps 与分阶段耗时分位数
//   LumaVerify        回读纹理亮度，确认不是黑屏/错位（verifyLuma 打开时）

#include <atomic>
#include <cstdint>
#include <vector>

class App;

class Diagnostics {
 public:
  explicit Diagnostics(App& app) : m_app(app) {}

  // === 主循环分阶段计时（main.cpp 在对应位置调用）===
  // 五段：evt（SDL 事件）/ upload（帧上传 GPU）/ ui（ImGui 控件构建）/ gpu（Render+Present）/ tail。
  // upload 发生在 update() 内部，必须从 ui 段里扣掉，否则会全部算到 ui 头上，归因直接错。
  void begin_frame();
  void mark_evt_begin();
  void mark_evt_end();
  void mark_gpu_begin();
  void mark_gpu_end();
  void end_frame();

  // === App 上报的原始数据 ===
  void on_decoded_frame(bool is_gpu);  // SDK 解码线程调用
  void on_frames_uploaded(double ms, size_t count);
  void on_grid_cells_drawn(int cells) { m_window_cells = cells; }

  // 每帧在 App::update 末尾调用，驱动周期性日志
  void tick(float dt);

  // 出流集合的渲染覆盖情况；list_all_missing 时列出全部缺画面的实例
  void log_render_coverage(bool list_all_missing);
  uint64_t i420_frames() const { return m_frames_i420.load(); }
  uint64_t gpu_frames() const { return m_frames_gpu.load(); }

 private:
  void log_frame_stats();
  void log_loop_stats();
  void verify_luma(bool all);

  App& m_app;

  std::atomic<uint64_t> m_frames_i420{0};
  std::atomic<uint64_t> m_frames_gpu{0};
  float m_stats_timer = 0;
  float m_stream_start_elapsed = -1;  // 首次进入多流页面时的 App::elapsed_seconds
  bool m_coverage_10s_logged = false;

  std::vector<double> m_stage_ms;       // 本帧各阶段耗时（毫秒）
  std::vector<double> m_loop_ms;        // 2s 窗口内每帧总耗时
  std::vector<double> m_stage_hist[5];  // 2s 窗口内各阶段耗时
  double m_upload_ms_frame = 0;         // 本帧帧上传累计
  double m_prev_stage_ms = 0;           // 上一段时间戳
  float m_last_loop_log = 0;
  float m_last_loop_log_start = 0;
  unsigned long long m_window_uploads = 0;  // 2s 窗口内上传的帧数
  int m_window_cells = 0;                   // 上一帧实际绘制的格子数
};
