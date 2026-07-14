#include "Logger.h"

#include <filesystem>

#include <spdlog/spdlog.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>

#include "tcr_c_api.h"
#include "tcr_types.h"

#ifdef _WIN32
#include <windows.h>
#endif

namespace {

std::string wideToUtf8(const std::wstring& wide) {
#ifdef _WIN32
    if (wide.empty())
        return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr, 0,
                                   nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string result(size, '\0');
    WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
#else
    return std::string(wide.begin(), wide.end());
#endif
}

std::string getExeDirectory() {
#ifdef _WIN32
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().string();
#else
    return ".";
#endif
}

// TcrSdk log callback bridge — routes TcrSdk logs into spdlog
void tcrSdkLogCallback(void* /*user_data*/, TcrLogLevel level, const char* tag, const char* log) {
    if (!log) return;
    std::string msg = std::string("[") + (tag ? tag : "TcrSdk") + "] " + log;
    switch (level) {
    case TCR_LOG_LEVEL_TRACE:
    case TCR_LOG_LEVEL_DEBUG:
        spdlog::debug(msg);
        break;
    case TCR_LOG_LEVEL_INFO:
        spdlog::info(msg);
        break;
    case TCR_LOG_LEVEL_WARN:
        spdlog::warn(msg);
        break;
    case TCR_LOG_LEVEL_ERROR:
        spdlog::error(msg);
        break;
    default:
        spdlog::info(msg);
        break;
    }
}

}  // anonymous namespace

void Logger::globalInit() {
    try {
        std::string logDir = getExeDirectory() + "/logs";
        std::filesystem::create_directories(logDir);
        std::string logFile = logDir + "/app.log";

        // Rotating file sink: 200MB max, 5 backup files
        auto fileSink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(logFile, 200 * 1024 * 1024, 5);

        // Console sink (colored output)
        auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();

        std::vector<spdlog::sink_ptr> sinks = {consoleSink, fileSink};
        auto logger = std::make_shared<spdlog::logger>("app", sinks.begin(), sinks.end());

        // Format: [timestamp] [thread] [level] message
        logger->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%t] [%^%l%$] %v");
        logger->set_level(spdlog::level::debug);
        logger->flush_on(spdlog::level::info);

        spdlog::set_default_logger(logger);
        spdlog::info("Logger initialized, log file: {}", logFile);

        // Bridge TcrSdk logging into spdlog
        static TcrLogCallback tcrLogCb;
        tcrLogCb.user_data = nullptr;
        tcrLogCb.on_log = tcrSdkLogCallback;
        tcr_set_log_callback(&tcrLogCb);
        tcr_set_log_level(TCR_LOG_LEVEL_INFO);
        spdlog::info("TcrSdk log callback registered");
    } catch (const spdlog::spdlog_ex& ex) {
#ifdef _WIN32
        OutputDebugStringA("Logger init failed: ");
        OutputDebugStringA(ex.what());
        OutputDebugStringA("\n");
#endif
    }
}

void Logger::debug(const std::string& message) { spdlog::debug(message); }
void Logger::info(const std::string& message) { spdlog::info(message); }
void Logger::warning(const std::string& message) { spdlog::warn(message); }
void Logger::error(const std::string& message) { spdlog::error(message); }

void Logger::debug(const std::wstring& message) { spdlog::debug(wideToUtf8(message)); }
void Logger::info(const std::wstring& message) { spdlog::info(wideToUtf8(message)); }
void Logger::warning(const std::wstring& message) { spdlog::warn(wideToUtf8(message)); }
void Logger::error(const std::wstring& message) { spdlog::error(wideToUtf8(message)); }
