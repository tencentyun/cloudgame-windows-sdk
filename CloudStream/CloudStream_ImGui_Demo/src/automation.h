#pragma once

// automation.h - 无人值守验证用的驱动逻辑（与 Demo 的业务功能无关）
//
// 全部能力都要显式打开，默认运行时不产生任何行为：
//   - config.json：autoStart / autoExitSeconds / autoSwitch(+IntervalSeconds)
//   - 环境变量：IMGUI_DEMO_CONFIG / IMGUI_DEMO_INSTANCE_IDS / IMGUI_DEMO_GRID_CELL_WIDTH /
//     IMGUI_DEMO_AUTO_START，覆盖 token 页的默认输入
//   - IMGUI_DEMO_CMD_FILE：命令文件，每追加一行执行一条命令（滚动 / 点击 / dump），
//     走与鼠标操作相同的 App 接口，结果以 [Ctl] 写入日志。命令说明见
//     TcrSdkProject/Demo/CloudStream_ImGui/AGENTS.md。

#include <set>
#include <string>
#include <vector>

class App;

class Automation {
 public:
  explicit Automation(App& app) : m_app(app) {}

  // IMGUI_DEMO_CONFIG 指定的初始配置名（不含 .json），未设置时为空
  static std::string initial_config_name();

  // App::init 选完配置后调用：套用其余环境变量覆盖
  void apply_env_overrides();

  // 每帧在 App::update 里调用：before_ui 在构建界面前（命令要在本帧界面上生效），
  // after_ui 在界面之后（autoStart / autoExit / autoSwitch）
  void before_ui();
  void after_ui(float dt);

 private:
  void poll_commands(float dt);
  void exec_command(const std::string& line);
  void log_popup_first_frames();
  void dump();
  // 完整落在视口内的格子下标（半截露出的不算，避免点到看不见的格子）
  std::vector<size_t> fully_visible_cells() const;
  void auto_switch(float dt);

  App& m_app;

  std::string m_cmd_path;
  long m_cmd_offset = 0;
  float m_cmd_timer = 0;
  std::set<int> m_popups_first_frame_logged;

  bool m_auto_start_fired = false;
  float m_auto_switch_timer = 0;
  size_t m_auto_switch_offset = 0;
  bool m_auto_switch_started = false;
};
