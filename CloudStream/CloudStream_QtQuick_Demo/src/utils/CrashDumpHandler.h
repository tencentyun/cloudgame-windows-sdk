#ifndef CRASHDUMPHANDLER_H
#define CRASHDUMPHANDLER_H

#include <QString>
#include <atomic>
#include <mutex>

// Prevent Windows macros from polluting the global namespace
#ifndef NOMINMAX
#define NOMINMAX
#endif

#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif

// Undefine problematic Windows macros that conflict with common identifiers
#ifdef ERROR
#undef ERROR
#endif

#include <windows.h>
#include <dbghelp.h>

/**
 * @brief 崩溃转储处理类
 * 
 * 负责在程序崩溃时自动生成完整的dump文件，
 * 包含程序状态、调用栈、内存信息等调试数据
 * 
 * 注意：此处理器可以捕获主程序和加载的DLL中的崩溃
 */
class CrashDumpHandler {
public:
    /**
     * @brief 初始化崩溃处理器
     * @param dumpPath dump文件保存路径（可选，默认为程序运行目录）
     * @return 是否初始化成功
     * 
     * 注意：必须在程序启动早期调用，在加载任何可能崩溃的DLL之前
     */
    static bool initialize(const QString& dumpPath = QString());

    /**
     * @brief 获取dump文件保存路径
     */
    static QString getDumpPath();

private:
    /**
     * @brief 未处理异常过滤器回调函数
     * @param exceptionInfo 异常信息指针
     * @return 异常处理结果
     */
    static LONG WINAPI unhandledExceptionFilter(EXCEPTION_POINTERS* exceptionInfo);

    /**
     * @brief 创建MiniDump文件
     * @param exceptionInfo 异常信息指针
     * @return 是否创建成功
     */
    static bool createMiniDump(EXCEPTION_POINTERS* exceptionInfo);

    static QString s_dumpPath;  ///< dump文件保存路径
    static std::atomic<bool> s_dumpInProgress;  ///< 标记是否正在创建dump
    static std::mutex s_dumpMutex;  ///< 保护dump创建过程的互斥锁
};

#endif // CRASHDUMPHANDLER_H