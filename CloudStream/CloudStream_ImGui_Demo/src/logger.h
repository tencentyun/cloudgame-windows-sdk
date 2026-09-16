#pragma once

// logger.h - 简单日志工具
//
// 日志同时输出到 stderr 和一个按日期时间命名的文件。
// 文件由 Log::init() 创建（放在可执行文件同目录），Log::shutdown() 关闭。
// TcrSdk 的日志回调（App::init 中设置）也会转发到这里，因此 SDK 日志与
// Demo 自身日志会写入同一个文件。

#include <cstdarg>
#include <cstdio>
#include <ctime>
#include <mutex>
#include <string>
#include <vector>

namespace Log {

enum class Level { Debug, Info, Warning, Error };

// Use a function to get/set level (C++14 compatible, no inline variable)
inline Level& g_level_ref() {
  static Level level = Level::Info;
  return level;
}

inline void set_level(Level level) { g_level_ref() = level; }

// 日志文件句柄与目录路径，由 init() 填充。文件写入需加锁，
// 因为 TcrSdk 日志回调可能从多个线程并发触发。
inline FILE*& g_file_ref() {
  static FILE* file = nullptr;
  return file;
}

inline std::mutex& g_mutex_ref() {
  static std::mutex mu;
  return mu;
}

inline std::string& g_log_path_ref() {
  static std::string path;
  return path;
}

// 返回日志文件的完整路径（供 UI 展示 / 排查），未初始化时为空串。
inline const std::string& log_path() { return g_log_path_ref(); }

// 生成形如 "tcr_demo_YYYYMMDD_HHMMSS.log" 的文件名。
inline std::string make_log_filename() {
  std::time_t now = std::time(nullptr);
  std::tm tmv;
#if defined(_WIN32)
  localtime_s(&tmv, &now);
#else
  localtime_r(&now, &tmv);
#endif
  char buf[64];
  std::strftime(buf, sizeof(buf), "tcr_demo_%Y%m%d_%H%M%S.log", &tmv);
  return std::string(buf);
}

// 在可执行文件同目录下创建日志文件（按日期时间命名）。
// base_dir 由调用方传入（通常来自 SDL_GetBasePath），需以路径分隔符结尾。
// 返回 true 表示成功打开文件；失败时仍可继续运行，只是不写文件。
inline bool init(const std::string& base_dir) {
  std::string full = base_dir + make_log_filename();
  FILE* f = nullptr;
#if defined(_WIN32)
  fopen_s(&f, full.c_str(), "a");
#else
  f = fopen(full.c_str(), "a");
#endif
  if (!f) return false;
  g_file_ref() = f;
  g_log_path_ref() = full;
  return true;
}

inline void shutdown() {
  FILE* f = g_file_ref();
  if (f) {
    fflush(f);
    fclose(f);
    g_file_ref() = nullptr;
  }
}

inline void log(Level level, const char* tag, const char* fmt, ...) {
  if (level < g_level_ref()) return;

  const char* prefix = "";
  switch (level) {
    case Level::Debug:
      prefix = "[DEBUG]";
      break;
    case Level::Info:
      prefix = "[INFO]";
      break;
    case Level::Warning:
      prefix = "[WARN]";
      break;
    case Level::Error:
      prefix = "[ERROR]";
      break;
  }

  // 先格式化到缓冲区，再统一输出到 stderr 与文件，避免两次 va_start 消耗同一 va_list。
  // 消息可能很长（如包含大段请求/响应 JSON），不能用固定大小栈缓冲区，否则会截断。
  // 先探测所需长度，再动态分配，保证完整写入。
  va_list args;
  va_start(args, fmt);
  va_list args_copy;
  va_copy(args_copy, args);
  int need = vsnprintf(nullptr, 0, fmt, args_copy);
  va_end(args_copy);
  std::vector<char> buf(need >= 0 ? need + 1 : 2048);
  vsnprintf(buf.data(), buf.size(), fmt, args);
  va_end(args);
  const char* msg = buf.data();

  std::lock_guard<std::mutex> lock(g_mutex_ref());

  fprintf(stderr, "%s [%s] %s\n", prefix, tag, msg);
  fflush(stderr);

  FILE* f = g_file_ref();
  if (f) {
    // 带时间戳写入文件，便于跨线程/长时间运行排查时序。
    std::time_t now = std::time(nullptr);
    std::tm tmv;
#if defined(_WIN32)
    localtime_s(&tmv, &now);
#else
    localtime_r(&now, &tmv);
#endif
    char ts[32];
    std::strftime(ts, sizeof(ts), "%Y-%m-%d %H:%M:%S", &tmv);
    fprintf(f, "%s %s [%s] %s\n", ts, prefix, tag, msg);
    fflush(f);
  }
}

#define LOG_DEBUG(tag, fmt, ...) Log::log(Log::Level::Debug, tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...) Log::log(Log::Level::Info, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...) Log::log(Log::Level::Warning, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) Log::log(Log::Level::Error, tag, fmt, ##__VA_ARGS__)

}  // namespace Log
