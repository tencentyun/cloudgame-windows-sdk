#include "automation.h"

#include <imgui.h>
#include <SDL.h>

#include <algorithm>
#include <cmath>
#include <cstdio>
#include <cstdlib>
#include <sstream>

#include "app.h"
#include "logger.h"
#include "video_renderer.h"

// =============================================================================
// 环境变量覆盖
// =============================================================================

std::string Automation::initial_config_name() {
  const char* cfg = SDL_getenv("IMGUI_DEMO_CONFIG");
  return cfg ? cfg : "";
}

void Automation::apply_env_overrides() {
  const char* ids = SDL_getenv("IMGUI_DEMO_INSTANCE_IDS");
  const char* gcw = SDL_getenv("IMGUI_DEMO_GRID_CELL_WIDTH");
  m_app.set_token_page_inputs(ids && ids[0] ? ids : nullptr, gcw ? std::atoi(gcw) : 0);

  if (const char* as = SDL_getenv("IMGUI_DEMO_AUTO_START")) m_app.config().auto_start = (as[0] == '1');

  if (const char* cf = SDL_getenv("IMGUI_DEMO_CMD_FILE")) {
    m_cmd_path = cf;
    LOG_INFO("Ctl", "command file: %s", m_cmd_path.c_str());
  }
}

// =============================================================================
// 每帧驱动
// =============================================================================

void Automation::before_ui() {
  if (m_cmd_path.empty()) return;
  log_popup_first_frames();
  poll_commands(ImGui::GetIO().DeltaTime);
}

void Automation::after_ui(float dt) {
  AppConfig& cfg = m_app.config();
  const float elapsed = m_app.elapsed_seconds();

  // autoStart: 启动 1 秒后自动触发一次取 token + 串流
  if (cfg.auto_start && !m_auto_start_fired && elapsed > 1.0f && m_app.state() == AppState::TOKEN_PAGE) {
    m_auto_start_fired = true;
    LOG_INFO("App", "autoStart triggered");
    m_app.request_token();
  }

  if (cfg.auto_exit_seconds > 0 && elapsed >= (float)cfg.auto_exit_seconds) {
    Diagnostics& diag = m_app.diagnostics();
    diag.log_render_coverage(true);
    LOG_INFO("App", "autoExitSeconds reached, quitting. i420_frames=%llu gpu_frames=%llu",
             (unsigned long long)diag.i420_frames(), (unsigned long long)diag.gpu_frames());
    m_app.quit();
  }

  if (cfg.auto_switch) auto_switch(dt);
}

// autoSwitch: 模拟滚动窗口列表，周期性切换实例子集（触发 tcr_session_switch_streaming_instances）
void Automation::auto_switch(float dt) {
  const auto& all = m_app.instance_ids();
  if (m_app.state() != AppState::MULTI_STREAM || all.empty() || !m_app.has_session()) return;
  if (!m_auto_switch_started) {
    // 首次：从 instance_ids 头部取一个窗口，记录起点
    m_auto_switch_started = true;
    m_auto_switch_timer = 0;
    m_auto_switch_offset = 0;
    return;
  }
  m_auto_switch_timer += dt;
  if (m_auto_switch_timer < (float)m_app.config().auto_switch_interval_seconds) return;
  m_auto_switch_timer = 0;
  // 滑动窗口：每次起点 +1（模拟滚动一行），取当前动态并发数个实例
  size_t n = all.size();
  size_t lim = (size_t)m_app.concurrent_limit();
  std::vector<std::string> ids;
  ids.reserve(lim);
  for (size_t i = 0; i < lim && i < n; ++i) ids.push_back(all[(m_auto_switch_offset + i) % n]);
  m_auto_switch_offset = (m_auto_switch_offset + 1) % n;
  LOG_INFO("App", "autoSwitch: switching to window offset=%zu, count=%zu", m_auto_switch_offset, ids.size());
  m_app.switch_streaming_instances(ids);
}

// =============================================================================
// 命令文件
// =============================================================================

void Automation::log_popup_first_frames() {
  for (const PopupWindow* pw : m_app.popups()) {
    if (!pw->renderer || !pw->renderer->has_frame() || m_popups_first_frame_logged.count(pw->id)) continue;
    m_popups_first_frame_logged.insert(pw->id);
    LOG_INFO("Ctl", "popup #%d first frame rendered instance=%s size=%dx%d", pw->id,
             pw->is_sync ? "(sync)" : pw->instance_id.c_str(), pw->renderer->frame_width(),
             pw->renderer->frame_height());
  }
}

void Automation::poll_commands(float dt) {
  m_cmd_timer += dt;
  if (m_cmd_timer < 0.2f) return;
  m_cmd_timer = 0;
  FILE* f = fopen(m_cmd_path.c_str(), "rb");
  if (!f) return;
  fseek(f, 0, SEEK_END);
  long size = ftell(f);
  if (size < m_cmd_offset) m_cmd_offset = 0;  // 文件被截断/重建，从头读
  std::string data;
  if (size > m_cmd_offset) {
    data.resize((size_t)(size - m_cmd_offset));
    fseek(f, m_cmd_offset, SEEK_SET);
    data.resize(fread(&data[0], 1, data.size(), f));
  }
  fclose(f);
  // 只消费完整的行，半行留到下次
  size_t last_nl = data.rfind('\n');
  if (last_nl == std::string::npos) return;
  m_cmd_offset += (long)last_nl + 1;
  std::istringstream ss(data.substr(0, last_nl));
  std::string line;
  while (std::getline(ss, line)) {
    while (!line.empty() && (line.back() == '\r' || line.back() == ' ')) line.pop_back();
    if (!line.empty() && line[0] != '#') exec_command(line);
  }
}

std::vector<size_t> Automation::fully_visible_cells() const {
  const GridLayout& g = m_app.grid_layout();
  std::vector<size_t> v;
  if (g.row_h <= 0 || g.cols <= 0) return v;
  int first = (int)std::ceil(g.scroll_y / g.row_h - 0.01f);
  int last = (int)std::floor((g.scroll_y + g.view_h) / g.row_h + 0.01f);
  size_t end = std::min(m_app.instance_ids().size(), (size_t)std::max(0, last) * g.cols);
  for (size_t i = (size_t)std::max(0, first) * g.cols; i < end; ++i) v.push_back(i);
  return v;
}

void Automation::dump() {
  const GridLayout& g = m_app.grid_layout();
  const auto& all = m_app.instance_ids();
  const auto& streaming = m_app.streaming_ids();
  int row = g.row_h > 0 ? (int)(g.scroll_y / g.row_h + 0.5f) : 0;
  int total_rows = g.cols > 0 ? (int)((all.size() + g.cols - 1) / g.cols) : 0;
  auto vis = fully_visible_cells();
  LOG_INFO("Ctl", "dump state=%d cols=%d top_row=%d total_rows=%d visible=%zu streaming=%zu popups=%zu",
           (int)m_app.state(), g.cols, row, total_rows, vis.size(), streaming.size(), m_app.popups().size());
  const auto totals = m_app.frame_totals();
  for (size_t i : vis) {
    const std::string& id = all[i];
    auto fc = totals.find(id);
    LOG_INFO("Ctl", "cell idx=%zu row=%zu id=%s streaming=%d rendered=%d frames=%llu reason=%s", i, i / g.cols,
             id.c_str(), streaming.count(id) ? 1 : 0, m_app.is_rendered(id) ? 1 : 0,
             (unsigned long long)(fc != totals.end() ? fc->second : 0), m_app.fail_reason(id).c_str());
  }
  for (const PopupWindow* pw : m_app.popups()) {
    LOG_INFO("Ctl", "popup #%d instance=%s connected=%d frames=%llu rendered=%d close_reason=%s", pw->id,
             pw->is_sync ? "(sync)" : pw->instance_id.c_str(), pw->connected ? 1 : 0,
             (unsigned long long)pw->frame_count.load(), pw->renderer && pw->renderer->has_frame() ? 1 : 0,
             pw->close_reason.c_str());
  }
  LOG_INFO("Ctl", "dump end");
}

void Automation::exec_command(const std::string& line) {
  LOG_INFO("Ctl", "cmd: %s", line.c_str());
  std::istringstream ss(line);
  std::string cmd, arg;
  ss >> cmd >> arg;
  const GridLayout& g = m_app.grid_layout();
  const auto& all = m_app.instance_ids();
  const bool in_grid = m_app.state() == AppState::MULTI_STREAM && g.row_h > 0;

  if (cmd == "start") {
    if (m_app.state() == AppState::TOKEN_PAGE) m_app.request_token();
  } else if (cmd == "scroll_rows" || cmd == "scroll_to_row" || cmd == "scroll_to_instance") {
    if (!in_grid) {
      LOG_WARN("Ctl", "%s ignored: not on multi-stream page", cmd.c_str());
      return;
    }
    int cur = (int)(g.scroll_y / g.row_h + 0.5f);
    int target = 0;
    if (cmd == "scroll_rows") {
      target = cur + std::atoi(arg.c_str());
    } else if (cmd == "scroll_to_row") {
      target = std::atoi(arg.c_str());
    } else {
      auto it = std::find(all.begin(), all.end(), arg);
      if (it == all.end()) {
        LOG_WARN("Ctl", "scroll_to_instance: %s not in list", arg.c_str());
        return;
      }
      target = (int)((it - all.begin()) / g.cols);
    }
    target = std::max(0, target);
    // ImGui 会把超出范围的滚动值钳到 ScrollMax，所以末尾行可能停不到顶端
    m_app.scroll_grid_to(target * g.row_h);
    LOG_INFO("Ctl", "scroll from_row=%d to_row=%d", cur, target);
  } else if (cmd == "click" || cmd == "click_rendered" || cmd == "click_unrendered") {
    if (!in_grid) {
      LOG_WARN("Ctl", "%s ignored: not on multi-stream page", cmd.c_str());
      return;
    }
    std::string id;
    if (cmd == "click") {
      id = arg;
    } else {
      const bool want = cmd == "click_rendered";
      for (size_t i : fully_visible_cells()) {
        if (m_app.is_rendered(all[i]) == want) {
          id = all[i];
          break;
        }
      }
    }
    if (id.empty()) {
      LOG_WARN("Ctl", "%s: no matching instance in visible rows", cmd.c_str());
      return;
    }
    const bool rendered = m_app.is_rendered(id);
    PopupWindow* pw = m_app.open_instance_popup(id);
    LOG_INFO("Ctl", "open popup instance=%s grid_rendered=%d popup=#%d", id.c_str(), rendered ? 1 : 0,
             pw ? pw->id : -1);
  } else if (cmd == "close_popups") {
    m_app.request_close_popups();
  } else if (cmd == "dump") {
    dump();
  } else if (cmd == "quit") {
    m_app.quit();
  } else {
    LOG_WARN("Ctl", "unknown command: %s", cmd.c_str());
  }
}
