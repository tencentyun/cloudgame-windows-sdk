#pragma once

// logger.h - 简单日志工具

#include <cstdarg>
#include <cstdio>

namespace Log {

enum class Level { Debug, Info, Warning, Error };

// Use a function to get/set level (C++14 compatible, no inline variable)
inline Level& g_level_ref() {
  static Level level = Level::Info;
  return level;
}

inline void set_level(Level level) { g_level_ref() = level; }

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

  fprintf(stderr, "%s [%s] ", prefix, tag);
  va_list args;
  va_start(args, fmt);
  vfprintf(stderr, fmt, args);
  va_end(args);
  fprintf(stderr, "\n");
  fflush(stderr);
}

#define LOG_DEBUG(tag, fmt, ...) Log::log(Log::Level::Debug, tag, fmt, ##__VA_ARGS__)
#define LOG_INFO(tag, fmt, ...) Log::log(Log::Level::Info, tag, fmt, ##__VA_ARGS__)
#define LOG_WARN(tag, fmt, ...) Log::log(Log::Level::Warning, tag, fmt, ##__VA_ARGS__)
#define LOG_ERROR(tag, fmt, ...) Log::log(Log::Level::Error, tag, fmt, ##__VA_ARGS__)

}  // namespace Log
