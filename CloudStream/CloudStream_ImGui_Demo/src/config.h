#pragma once

// config.h - 应用配置管理（从 config.json 加载）

#include <string>
#include <vector>

struct AppConfig {
  std::string base_url = "https://test-accelerator-biz-server.cai.crtrcloud.com";
  std::string api_path = "/CreateAndroidInstancesAccessToken";
  std::string instance_ids;  // 逗号分隔的实例 ID

  // 视频流参数
  int video_width = 720;
  int video_fps = 30;
  int video_min_bitrate = 300;
  int video_max_bitrate = 600;

  // 多实例并发流数
  int concurrent_streaming = 4;

  // 从 config.json 加载（在可执行文件同目录下查找）
  bool load(const std::string& config_path);

  // 将 instance_ids 按逗号分割为数组
  std::vector<std::string> get_instance_id_list() const;
};
