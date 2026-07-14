#pragma once

#include <string>

/**
 * @brief Logger utility class wrapping spdlog.
 *
 * Provides a simple static interface matching the original Qt Quick Demo's Logger.
 * Uses spdlog's rotating file sink (200MB, 5 backups) + console output.
 * Thread-safe by default (spdlog handles synchronization internally).
 */
class Logger {
public:
    static void globalInit();

    static void debug(const std::string& message);
    static void info(const std::string& message);
    static void warning(const std::string& message);
    static void error(const std::string& message);

    // Wide string overloads for DuiLib compatibility
    static void debug(const std::wstring& message);
    static void info(const std::wstring& message);
    static void warning(const std::wstring& message);
    static void error(const std::wstring& message);
};
