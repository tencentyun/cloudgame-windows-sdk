#include "CrashDumpHandler.h"
#include "Logger.h"
#include <QDir>
#include <QDateTime>
#include <QCoreApplication>

QString CrashDumpHandler::s_dumpPath;
std::atomic<bool> CrashDumpHandler::s_dumpInProgress(false);
std::mutex CrashDumpHandler::s_dumpMutex;

bool CrashDumpHandler::initialize(const QString& dumpPath) {
    // 设置dump文件保存路径
    if (dumpPath.isEmpty()) {
        s_dumpPath = QCoreApplication::applicationDirPath() + "/CrashDumps";
    } else {
        s_dumpPath = dumpPath;
    }

    // 确保目录存在
    QDir dir;
    if (!dir.exists(s_dumpPath)) {
        if (!dir.mkpath(s_dumpPath)) {
            qCritical() << "Failed to create dump directory:" << s_dumpPath;
            return false;
        }
    }

    // 设置未处理异常过滤器（返回旧的过滤器）
    LPTOP_LEVEL_EXCEPTION_FILTER oldFilter = SetUnhandledExceptionFilter(unhandledExceptionFilter);
    if (oldFilter != nullptr) {
        qWarning() << "Previous exception filter was replaced";
    }
    
    // 禁用 Windows 错误报告对话框，确保我们的处理器能够执行
    SetErrorMode(SEM_FAILCRITICALERRORS | SEM_NOGPFAULTERRORBOX | SEM_NOOPENFILEERRORBOX);
    
    qInfo() << "CrashDumpHandler initialized. Dump path:" << s_dumpPath;
    qInfo() << "Error mode set to suppress system error dialogs";
    return true;
}

QString CrashDumpHandler::getDumpPath() {
    return s_dumpPath;
}

LONG WINAPI CrashDumpHandler::unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo) {
    // 使用原子操作检查是否已经有线程在处理崩溃
    bool expected = false;
    if (!s_dumpInProgress.compare_exchange_strong(expected, true)) {
        // 已经有其他线程在创建dump，当前线程直接等待并退出
        qWarning() << "Another thread is already creating crash dump, current thread will wait...";
        qWarning() << "Current Thread ID:" << GetCurrentThreadId();
        
        // 等待第一个线程完成dump创建（最多等待30秒）
        for (int i = 0; i < 300 && s_dumpInProgress.load(); ++i) {
            Sleep(100);
        }
        
        return EXCEPTION_EXECUTE_HANDLER;
    }
    
    // 使用互斥锁保护整个dump创建过程
    std::lock_guard<std::mutex> lock(s_dumpMutex);
    
    qCritical() << "=== Unhandled exception detected! ===";
    qCritical() << "Current Thread ID:" << GetCurrentThreadId();
    qCritical() << "Exception Code:" << QString::number(exceptionInfo->ExceptionRecord->ExceptionCode, 16);
    qCritical() << "Exception Address:" << exceptionInfo->ExceptionRecord->ExceptionAddress;
    
    // 创建dump文件
    bool success = createMiniDump(exceptionInfo);
    
    if (success) {
        qCritical() << "Crash dump created successfully in:" << s_dumpPath;
        
        // 显示消息框通知用户（可选）
        QString message = QString("Application crashed!\nDump file saved to:\n%1").arg(s_dumpPath);
        MessageBoxW(NULL, 
                   reinterpret_cast<LPCWSTR>(message.utf16()),
                   L"Crash Detected",
                   MB_OK | MB_ICONERROR);
    } else {
        qCritical() << "Failed to create crash dump!";
        MessageBoxW(NULL, 
                   L"Failed to create crash dump file!",
                   L"Crash Handler Error",
                   MB_OK | MB_ICONERROR);
    }

    // 返回EXCEPTION_EXECUTE_HANDLER让系统终止程序
    return EXCEPTION_EXECUTE_HANDLER;
}

bool CrashDumpHandler::createMiniDump(EXCEPTION_POINTERS* exceptionInfo) {
    // 生成带时间戳、进程ID和线程ID的dump文件名，避免冲突
    QString timestamp = QDateTime::currentDateTime().toString("yyyyMMdd_HHmmss_zzz");  // 添加毫秒
    DWORD processId = GetCurrentProcessId();
    DWORD threadId = GetCurrentThreadId();
    QString dumpFileName = QString("%1/crash_%2_pid%3_tid%4.dmp")
                              .arg(s_dumpPath)
                              .arg(timestamp)
                              .arg(processId)
                              .arg(threadId);

    qCritical() << "Creating dump file:" << dumpFileName;

    // 创建dump文件
    HANDLE hFile = CreateFileW(
        reinterpret_cast<LPCWSTR>(dumpFileName.utf16()),
        GENERIC_WRITE,
        0,  // 不共享
        NULL,
        CREATE_ALWAYS,
        FILE_ATTRIBUTE_NORMAL,
        NULL
    );

    if (hFile == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        qCritical() << "Failed to create dump file:" << dumpFileName;
        qCritical() << "Error code:" << error;
        
        // 检查目录是否存在和可写
        QDir dir(s_dumpPath);
        qCritical() << "Dump directory exists:" << dir.exists();
        qCritical() << "Dump directory path:" << s_dumpPath;
        
        return false;
    }

    qCritical() << "Dump file handle created successfully";

    // 设置dump信息
    MINIDUMP_EXCEPTION_INFORMATION dumpInfo;
    dumpInfo.ThreadId = GetCurrentThreadId();
    dumpInfo.ExceptionPointers = exceptionInfo;
    dumpInfo.ClientPointers = FALSE;

    qCritical() << "Current Thread ID:" << dumpInfo.ThreadId;

    // 写入完整的dump文件（包含所有内存、线程、模块信息）
    MINIDUMP_TYPE dumpType = static_cast<MINIDUMP_TYPE>(
        MiniDumpWithFullMemory |           // 包含完整内存
        MiniDumpWithHandleData |           // 包含句柄信息
        MiniDumpWithThreadInfo |           // 包含线程信息
        MiniDumpWithUnloadedModules |      // 包含已卸载模块
        MiniDumpWithFullMemoryInfo |       // 包含完整内存信息
        MiniDumpWithProcessThreadData |    // 包含进程线程数据
        MiniDumpWithModuleHeaders          // 包含模块头信息
    );

    qCritical() << "Calling MiniDumpWriteDump...";

    BOOL result = MiniDumpWriteDump(
        GetCurrentProcess(),
        GetCurrentProcessId(),
        hFile,
        dumpType,
        exceptionInfo ? &dumpInfo : NULL,
        NULL,
        NULL
    );

    if (!result) {
        DWORD error = GetLastError();
        qCritical() << "MiniDumpWriteDump failed. Error:" << error;
        CloseHandle(hFile);
        
        // 尝试删除不完整的dump文件
        DeleteFileW(reinterpret_cast<LPCWSTR>(dumpFileName.utf16()));
        return false;
    }

    qCritical() << "MiniDumpWriteDump succeeded";
    
    // 获取文件大小
    LARGE_INTEGER fileSize;
    if (GetFileSizeEx(hFile, &fileSize)) {
        qCritical() << "Dump file size:" << fileSize.QuadPart << "bytes";
    }
    
    CloseHandle(hFile);

    qInfo() << "Dump file created successfully:" << dumpFileName;
    return true;
}