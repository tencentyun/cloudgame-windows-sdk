#pragma once

#include <string>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief String conversion utilities for DuiLib (wstring/TCHAR) <-> network layer (UTF-8).
 */
namespace StringUtils {

inline std::wstring Utf8ToWide(const std::string& utf8) {
#ifdef _WIN32
    if (utf8.empty())
        return {};
    int size =
        MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), nullptr, 0);
    if (size <= 0)
        return {};
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_UTF8, 0, utf8.data(), static_cast<int>(utf8.size()), result.data(),
                        size);
    return result;
#else
    return std::wstring(utf8.begin(), utf8.end());
#endif
}

inline std::string WideToUtf8(const std::wstring& wide) {
#ifdef _WIN32
    if (wide.empty())
        return {};
    int size = WideCharToMultiByte(CP_UTF8, 0, wide.data(), static_cast<int>(wide.size()), nullptr,
                                   0, nullptr, nullptr);
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

inline std::wstring AnsiToWide(const std::string& ansi) {
#ifdef _WIN32
    if (ansi.empty())
        return {};
    int size =
        MultiByteToWideChar(CP_ACP, 0, ansi.data(), static_cast<int>(ansi.size()), nullptr, 0);
    if (size <= 0)
        return {};
    std::wstring result(size, L'\0');
    MultiByteToWideChar(CP_ACP, 0, ansi.data(), static_cast<int>(ansi.size()), result.data(),
                        size);
    return result;
#else
    return std::wstring(ansi.begin(), ansi.end());
#endif
}

inline std::string WideToAnsi(const std::wstring& wide) {
#ifdef _WIN32
    if (wide.empty())
        return {};
    int size = WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), nullptr,
                                   0, nullptr, nullptr);
    if (size <= 0)
        return {};
    std::string result(size, '\0');
    WideCharToMultiByte(CP_ACP, 0, wide.data(), static_cast<int>(wide.size()), result.data(), size,
                        nullptr, nullptr);
    return result;
#else
    return std::string(wide.begin(), wide.end());
#endif
}

}  // namespace StringUtils
