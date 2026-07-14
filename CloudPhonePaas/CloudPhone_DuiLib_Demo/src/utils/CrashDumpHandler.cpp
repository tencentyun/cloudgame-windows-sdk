#include "CrashDumpHandler.h"

#ifdef _WIN32

#include <windows.h>
#include <dbghelp.h>
#include <filesystem>
#include <string>

#pragma comment(lib, "dbghelp.lib")

namespace {

std::wstring getExeDirectoryW() {
    wchar_t path[MAX_PATH] = {};
    GetModuleFileNameW(nullptr, path, MAX_PATH);
    std::filesystem::path exePath(path);
    return exePath.parent_path().wstring();
}

std::wstring generateDumpFilename() {
    std::wstring dumpDir = getExeDirectoryW() + L"\\dumps";
    std::filesystem::create_directories(dumpDir);

    SYSTEMTIME st;
    GetLocalTime(&st);

    wchar_t filename[MAX_PATH] = {};
    swprintf_s(filename, MAX_PATH, L"%s\\crash_%04d%02d%02d_%02d%02d%02d.dmp", dumpDir.c_str(), st.wYear,
               st.wMonth, st.wDay, st.wHour, st.wMinute, st.wSecond);
    return filename;
}

LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    std::wstring dumpPath = generateDumpFilename();

    HANDLE hFile =
        CreateFileW(dumpPath.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, nullptr);

    if (hFile != INVALID_HANDLE_VALUE) {
        MINIDUMP_EXCEPTION_INFORMATION mei;
        mei.ThreadId = GetCurrentThreadId();
        mei.ExceptionPointers = exceptionInfo;
        mei.ClientPointers = FALSE;

        MiniDumpWriteDump(GetCurrentProcess(), GetCurrentProcessId(), hFile, MiniDumpNormal, &mei, nullptr,
                          nullptr);
        CloseHandle(hFile);
    }

    return EXCEPTION_EXECUTE_HANDLER;
}

}  // anonymous namespace

void CrashDumpHandler::initialize() {
    std::wstring dumpDir = getExeDirectoryW() + L"\\dumps";
    std::filesystem::create_directories(dumpDir);
    SetUnhandledExceptionFilter(unhandledExceptionFilter);
}

#else

void CrashDumpHandler::initialize() {}

#endif
