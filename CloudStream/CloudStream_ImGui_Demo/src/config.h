#pragma once

// config.h - 应用配置管理（从 config.json 加载）

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

  // 多实例并发流数
  int concurrent_streaming = 4;

  // 启用硬件解码（GPU 解码），失败时 SDK 自动回退软解
  bool hardware_decode = false;

  // 启动后自动请求 token 并开始串流，无需点击按钮（便于自动化验证）
  bool auto_start = false;

  // 运行指定秒数后自动退出，0 表示不自动退出（便于自动化验证）
  int auto_exit_seconds = 0;

  // 从 config.json 加载（在可执行文件同目录下查找）
  bool load(const std::string& config_path);

  // 将 instance_ids 按逗号分割为数组
  std::vector<std::string> get_instance_id_list() const;
};
