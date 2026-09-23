#include "config.h"

#include <algorithm>
#include <cstdlib>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>

#ifdef _WIN32
#  include <windows.h>
#else
#  include <dirent.h>
#endif

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

    // concurrentStreaming 不再从 json 读取，改为运行时按窗口尺寸动态计算。
    // 保留 gridCellWidth 用于控制单个子流画面的格子尺寸。
    if (j.contains("gridCellWidth") && j["gridCellWidth"].is_number_integer()) {
      grid_cell_width = j["gridCellWidth"].get<int>();
    }
    if (j.contains("showInstanceId") && j["showInstanceId"].is_boolean()) {
      show_instance_id = j["showInstanceId"].get<bool>();
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
    if (j.contains("autoSwitch") && j["autoSwitch"].is_boolean()) {
      auto_switch = j["autoSwitch"].get<bool>();
    }
    if (j.contains("autoSwitchIntervalSeconds") && j["autoSwitchIntervalSeconds"].is_number_integer()) {
      auto_switch_interval_seconds = j["autoSwitchIntervalSeconds"].get<int>();
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
  return split_comma_separated(instance_ids);
}

// ----------------------------------------------------------------------------
// 将逗号分隔的字符串拆分为数组（去除每项前后空格）
// ----------------------------------------------------------------------------
std::vector<std::string> split_comma_separated(const std::string& s) {
  std::vector<std::string> result;
  std::istringstream ss(s);
  std::string item;
  while (std::getline(ss, item, ',')) {
    size_t start = item.find_first_not_of(' ');
    size_t end = item.find_last_not_of(' ');
    if (start != std::string::npos) {
      result.push_back(item.substr(start, end - start + 1));
    }
  }
  return result;
}

// ----------------------------------------------------------------------------
// 扫描 <dir> 目录下所有 .json 文件，返回文件名列表（不含 .json 后缀）
// ----------------------------------------------------------------------------
std::vector<std::string> scan_config_files(const std::string& dir) {
  std::vector<std::string> names;

#ifdef _WIN32
  WIN32_FIND_DATAA find_data;
  std::string search_path = dir + "\\*.json";
  HANDLE handle = FindFirstFileA(search_path.c_str(), &find_data);
  if (handle != INVALID_HANDLE_VALUE) {
    do {
      if (!(find_data.dwFileAttributes & FILE_ATTRIBUTE_DIRECTORY)) {
        std::string name = find_data.cFileName;
        if (name.size() > 5 && name.substr(name.size() - 5) == ".json") {
          names.push_back(name.substr(0, name.size() - 5));
        }
      }
    } while (FindNextFileA(handle, &find_data));
    FindClose(handle);
  }
#else
  DIR* d = opendir(dir.c_str());
  if (d) {
    struct dirent* entry;
    while ((entry = readdir(d)) != nullptr) {
      std::string name = entry->d_name;
      if (name.size() > 5 && name.substr(name.size() - 5) == ".json") {
        names.push_back(name.substr(0, name.size() - 5));
      }
    }
    closedir(d);
  }
#endif

  std::sort(names.begin(), names.end());
  return names;
}
