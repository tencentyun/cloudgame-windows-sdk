#pragma once

#include <QDebug>
#include <QSysInfo>
#include <QCoreApplication>
#include "Logger.h"

/**
 * @brief 环境信息打印工具类
 * 
 * 提供静态方法用于打印应用程序运行环境信息，包括：
 * - Qt相关环境变量（QPA平台、Quick后端、RHI后端等）
 * - Qt版本信息
 * - 操作系统信息
 * 
 * 该类还会自动设置一些调试相关的环境变量，用于输出Qt Scene Graph和RHI的详细信息。
 */
class EnvInfoPrinter
{
public:
    /**
     * @brief 打印环境信息
     * 
     * 功能说明：
     * 1. 打印当前Qt相关环境变量的值
     * 2. 设置调试环境变量以输出更多运行时信息
     * 3. 打印Qt版本和操作系统信息
     * 
     * 打印的环境变量包括：
     * - QT_QPA_PLATFORM: Qt平台抽象层配置
     * - QT_QUICK_BACKEND: Qt Quick渲染后端
     * - QSG_RHI_BACKEND: Qt Scene Graph RHI后端
     * - QSG_INFO: Scene Graph调试信息开关
     */
    static void printEnvironmentInfo()
    {
        // 打印Qt平台相关环境变量
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QT_QPA_PLATFORM: %1")
                          .arg(qgetenv("QT_QPA_PLATFORM").constData()));
        
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QT_QUICK_BACKEND: %1")
                          .arg(qgetenv("QT_QUICK_BACKEND").constData()));
        
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QSG_RHI_BACKEND: %1")
                          .arg(qgetenv("QSG_RHI_BACKEND").constData()));
        
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QSG_INFO: %1")
                          .arg(qgetenv("QSG_INFO").constData()));
        
        // 设置调试环境变量，用于输出Qt Scene Graph和RHI的详细信息
        qputenv("QSG_INFO", "1");
        qputenv("QT_LOGGING_RULES", "qt.scenegraph.general=true;qt.rhi.general=true");
        
        // 打印Qt版本信息
        Logger::debug(QString("[TRACE][EnvInfoPrinter] Qt version: %1")
                          .arg(QT_VERSION_STR));
        
        // 打印操作系统信息
        Logger::debug(QString("[TRACE][EnvInfoPrinter] OS: %1")
                          .arg(QSysInfo::prettyProductName()));
    }
};