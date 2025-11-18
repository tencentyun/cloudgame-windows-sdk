#pragma once

#include <QObject>
#include <QFile>
#include <QTextStream>
#include <QDateTime>
#include <QThread>
#include <QMutex>
#include <QWaitCondition>
#include <QTimer>
#include <queue>

/**
 * @brief 日志管理类
 * 
 * 功能特性：
 * - 支持多种日志级别（DEBUG、INFO、WARNING、ERROR）
 * - 异步写入日志文件，避免阻塞主线程
 * - 支持日志文件自动轮转（按文件大小）
 * - 批量写入优化，提高I/O性能
 * - 线程安全的日志队列
 * - 同时输出到控制台和文件
 * 
 * 使用方式：
 * 1. 调用globalInit()进行全局初始化
 * 2. 使用静态方法debug/info/warning/error记录日志
 */
class Logger : public QObject
{
    Q_OBJECT
    
public:
    /**
     * @brief 日志级别枚举
     */
    enum LogLevel {
        DEBUG,      // 调试信息
        INFO,       // 一般信息
        WARNING,    // 警告信息
        ERROR       // 错误信息
    };

    /**
     * @brief 全局初始化日志系统
     * 
     * 执行以下操作：
     * - 初始化TCR SDK日志回调
     * - 设置TCR SDK日志级别
     * - 创建日志目录
     * - 配置日志文件路径
     * - 打印环境信息
     */
    static void globalInit();
    
    /**
     * @brief 获取Logger单例
     * @return Logger* 单例指针
     */
    static Logger* instance();

    /**
     * @brief 设置日志级别
     * @param level 日志级别，低于此级别的日志将被忽略
     */
    void setLogLevel(LogLevel level);
    
    /**
     * @brief 设置是否写入日志文件
     * @param enable 是否启用文件日志
     * @param filePath 日志文件路径（可选）
     */
    void setLogToFile(bool enable, const QString& filePath = QString());
    
    /**
     * @brief 设置单个日志文件的最大字节数
     * @param bytes 最大字节数，超过此大小将触发日志轮转
     */
    void setMaxFileSize(qint64 bytes);
    
    /**
     * @brief 设置日志轮转保留的备份文件个数
     * @param count 备份文件个数
     */
    void setMaxBackupFiles(int count);

    /**
     * @brief 记录日志
     * @param level 日志级别
     * @param message 日志消息
     */
    void log(LogLevel level, const QString& message);

    // 便捷的静态日志记录方法
    static void debug(const QString& message);      // 记录调试日志
    static void info(const QString& message);       // 记录信息日志
    static void warning(const QString& message);    // 记录警告日志
    static void error(const QString& message);      // 记录错误日志

private:
    /**
     * @brief 私有构造函数（单例模式）
     */
    Logger();
    
    /**
     * @brief 析构函数
     */
    ~Logger();

    /**
     * @brief 启动日志写入线程
     */
    void startLogThread();
    
    /**
     * @brief 停止日志写入线程
     */
    void stopLogThread();
    
    /**
     * @brief 日志写入线程的主循环
     */
    void logWriterLoop();

    /**
     * @brief 检查并在需要时轮转日志文件
     */
    void rotateLogFileIfNeeded();
    
    /**
     * @brief 执行日志文件轮转操作
     */
    void doRotate();

    /**
     * @brief 日志消息结构体
     */
    struct LogMsg {
        LogLevel level;         // 日志级别
        QString message;        // 日志内容
        QString timeStr;        // 时间戳字符串
        QString threadIdStr;    // 线程ID字符串
    };

    // 单例实例
    static Logger* m_instance;
    
    // 日志配置
    LogLevel m_logLevel;            // 当前日志级别
    bool m_logToFile;               // 是否写入文件
    QString m_logFilePath;          // 日志文件路径
    QFile m_logFile;                // 日志文件对象
    QTextStream m_textStream;       // 文本流对象

    // 线程安全队列相关
    QMutex m_queueMutex;                    // 队列互斥锁
    QWaitCondition m_queueNotEmpty;         // 队列非空条件变量
    std::queue<LogMsg> m_logQueue;          // 日志消息队列
    bool m_stopThread;                      // 停止线程标志
    QThread* m_logThread;                   // 日志写入线程

    // 批量写入优化相关
    static constexpr int BATCH_SIZE = 100;              // 批量写入大小
    static constexpr int FLUSH_INTERVAL_MS = 100;       // 强制刷新间隔（毫秒）
    qint64 m_lastFlushTime = 0;                         // 上次刷新时间

    // 日志轮转配置
    qint64 m_maxFileSize = 200 * 1024 * 1024;   // 单个日志文件最大大小（默认200MB）
    int m_maxBackupFiles = 50;                  // 最大备份文件数（默认50个）
    QString m_baseLogFileName;                  // 基础日志文件名（不含序号），格式：YYYYMMDD_HHMMSS_PID.log
    int m_currentFileIndex = 0;                 // 当前日志文件序号（0表示无序号）
};