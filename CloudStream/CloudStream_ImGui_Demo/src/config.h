#pragma once

// config.h - 应用配置管理（从 config/ 目录下的 json 加载）

#include <string>
#include <vector>

struct AppConfig {
  std::string base_url = "https://test-accelerator-biz-server.cai.crtrcloud.com";
  std::string api_path = "/CreateAndroidInstancesAccessToken";
  std::string instance_ids;  // 逗号分隔的实例 ID
  // 部分环境（如 Inner 接口）要求在请求体里带 AppId，为 0 表示不带
  long long app_id = 0;

  // 视频流参数
  int video_width = 720;
  int video_fps = 30;
  int video_min_bitrate = 300;
  int video_max_bitrate = 600;

  // 多实例并发流数。注意：此字段不再从 json 读取，运行时由「窗口尺寸 ÷ 固定格子尺寸」
  // 动态计算（见 App::recompute_concurrent_streaming），保证 concurrentStreaming 始终等于
  // 当前窗口可平铺渲染的子流画面数量。此处仅作为默认/兜底值保留。
  int concurrent_streaming = 4;

  // 单个子流画面的格子宽度（像素）。格子尺寸固定，同一分辨率下能展示的子流画面数量
  // 由「窗口可用宽高 ÷ 格子尺寸」决定，调整此值即可改变每行/每列能容纳的画面数。
  int grid_cell_width = 170;

  // 启用硬件解码（GPU 解码），失败时 SDK 自动回退软解
  bool hardware_decode = false;

  // 启动后自动请求 token 并开始串流，无需点击按钮（便于自动化验证）
  bool auto_start = false;

  // 运行指定秒数后自动退出，0 表示不自动退出（便于自动化验证）
  int auto_exit_seconds = 0;

  // 模拟滚动窗口列表：为 true 时，串流稳定后周期性调用 tcr_session_switch_streaming_instances
  // 切换实例子集（模拟用户上下滚动列表），用于验证多流切换逻辑（便于自动化验证）
  bool auto_switch = false;

  // 每次模拟滚动切换的间隔秒数（仅在 auto_switch=true 时生效）
  int auto_switch_interval_seconds = 5;

  // 从指定 json 文件加载配置
  bool load(const std::string& config_path);

  // 将 instance_ids 按逗号分割为数组
  std::vector<std::string> get_instance_id_list() const;
};

// 扫描 <dir> 目录下所有 .json 文件，返回文件名列表（不含 .json 后缀）。
// 返回值为空表示目录不存在或无 json 文件。
std::vector<std::string> scan_config_files(const std::string& dir);

// 将逗号分隔的字符串按逗号拆分为数组（去除每项前后空格）。
std::vector<std::string> split_comma_separated(const std::string& s);
