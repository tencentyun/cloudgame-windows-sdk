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

class Logger : public QObject
{
    Q_OBJECT
public:
    enum LogLevel {
        DEBUG,
        INFO,
        WARNING,
        ERROR
    };

    static void globalInit();
    static Logger* instance();

    void setLogLevel(LogLevel level);
    void setLogToFile(bool enable, const QString& filePath = QString());
    void setMaxFileSize(qint64 bytes); // 设置单个日志文件最大字节数
    void setMaxBackupFiles(int count); // 设置轮转文件个数

    void log(LogLevel level, const QString& message);

    // 方便使用的静态函数
    static void debug(const QString& message);
    static void info(const QString& message);
    static void warning(const QString& message);
    static void error(const QString& message);

private:
    Logger();
    ~Logger();

    void startLogThread();
    void stopLogThread();
    void logWriterLoop();

    void rotateLogFileIfNeeded(); // 轮转日志文件
    void doRotate();              // 执行轮转

    struct LogMsg {
        LogLevel level;
        QString message;
        QString timeStr;
        QString threadIdStr;
    };

    static Logger* m_instance;
    LogLevel m_logLevel;
    bool m_logToFile;
    QString m_logFilePath;
    QFile m_logFile;
    QTextStream m_textStream;

    // 线程安全队列
    QMutex m_queueMutex;
    QWaitCondition m_queueNotEmpty;
    std::queue<LogMsg> m_logQueue;
    bool m_stopThread;
    QThread* m_logThread;

    // 批量写入相关
    static constexpr int BATCH_SIZE = 100;
    static constexpr int FLUSH_INTERVAL_MS = 100; // 100ms强制flush
    qint64 m_lastFlushTime = 0;

    // 日志轮转相关
    qint64 m_maxFileSize = 200 * 1024 * 1024; // 200MB
    int m_maxBackupFiles = 5;
};