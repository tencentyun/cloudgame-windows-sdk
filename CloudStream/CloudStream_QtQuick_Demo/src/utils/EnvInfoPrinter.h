#pragma once
#include <QDebug>
#include <QSysInfo>
#include <QCoreApplication>

/**
 * @brief 环境信息打印工具类
 * 
 * 提供静态方法用于打印应用程序运行环境信息
 */
class EnvInfoPrinter {
public:
    /**
     * @brief 打印环境信息
     * 
     * 打印包括QT环境变量、QT版本和操作系统信息
     */
    static void printEnvironmentInfo() {
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QT_QPA_PLATFORM:%1").arg(qgetenv("QT_QPA_PLATFORM").constData()));
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QT_QUICK_BACKEND:%1").arg(qgetenv("QT_QUICK_BACKEND").constData()));
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QSG_RHI_BACKEND:%1").arg(qgetenv("QSG_RHI_BACKEND").constData()));
        Logger::debug(QString("[TRACE][EnvInfoPrinter] QSG_INFO:%1").arg(qgetenv("QSG_INFO").constData()));
        
        // 设置环境变量
        qputenv("QSG_INFO", "1"); // 输出QSG相关信息
        qputenv("QT_LOGGING_RULES", "qt.scenegraph.general=true;qt.rhi.general=true");
        
        Logger::debug(QString("[TRACE][EnvInfoPrinter] Qt version:%1").arg(QT_VERSION_STR));
        Logger::debug(QString("[TRACE][EnvInfoPrinter] OS:%1").arg(QSysInfo::prettyProductName()));
    }
};