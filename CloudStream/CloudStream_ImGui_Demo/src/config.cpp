#include "config.h"

#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#include "logger.h"

bool AppConfig::load(const std::string& config_path) {
  std::ifstream file(config_path);
  if (!file.is_open()) {
    LOG_WARN("Config", "Cannot open config file: %s, using defaults", config_path.c_str());
    return false;
  }

  try {
    nlohmann::json j;
    file >> j;

    if (j.contains("baseUrl") && j["baseUrl"].is_string()) {
      base_url = j["baseUrl"].get<std::string>();
    }
    if (j.contains("apiPath") && j["apiPath"].is_string()) {
      api_path = j["apiPath"].get<std::string>();
    }
    if (j.contains("instanceIds") && j["instanceIds"].is_string()) {
      instance_ids = j["instanceIds"].get<std::string>();
    }
    // appId 允许写成数字或字符串
    if (j.contains("appId")) {
      if (j["appId"].is_number_integer()) {
        app_id = j["appId"].get<long long>();
      } else if (j["appId"].is_string()) {
        const std::string s = j["appId"].get<std::string>();
        app_id = s.empty() ? 0 : std::strtoll(s.c_str(), nullptr, 10);
      }
    }
    if (j.contains("videoProfile") && j["videoProfile"].is_object()) {
      auto& vp = j["videoProfile"];
      if (vp.contains("width")) video_width = vp["width"].get<int>();
      if (vp.contains("fps")) video_fps = vp["fps"].get<int>();
      if (vp.contains("minBitrate")) video_min_bitrate = vp["minBitrate"].get<int>();
      if (vp.contains("maxBitrate")) video_max_bitrate = vp["maxBitrate"].get<int>();
    }

    if (j.contains("concurrentStreaming") && j["concurrentStreaming"].is_number_integer()) {
      concurrent_streaming = j["concurrentStreaming"].get<int>();
    }
    if (j.contains("hardwareDecode") && j["hardwareDecode"].is_boolean()) {
      hardware_decode = j["hardwareDecode"].get<bool>();
    }
    if (j.contains("autoStart") && j["autoStart"].is_boolean()) {
      auto_start = j["autoStart"].get<bool>();
    }
    if (j.contains("autoExitSeconds") && j["autoExitSeconds"].is_number_integer()) {
      auto_exit_seconds = j["autoExitSeconds"].get<int>();
    }

    LOG_INFO("Config", "Loaded config: baseUrl=%s, instanceIds=%s, concurrent=%d, hardwareDecode=%d", base_url.c_str(),
             instance_ids.c_str(), concurrent_streaming, hardware_decode ? 1 : 0);
    return true;
  } catch (const std::exception& e) {
    LOG_ERROR("Config", "Failed to parse config: %s", e.what());
    return false;
  }
}

std::vector<std::string> AppConfig::get_instance_id_list() const {
  std::vector<std::string> result;
  std::istringstream ss(instance_ids);
  std::string item;
  while (std::getline(ss, item, ',')) {
    // 去除前后空格
    size_t start = item.find_first_not_of(' ');
    size_t end = item.find_last_not_of(' ');
    if (start != std::string::npos) {
      result.push_back(item.substr(start, end - start + 1));
    }
  }
  return result;
}
