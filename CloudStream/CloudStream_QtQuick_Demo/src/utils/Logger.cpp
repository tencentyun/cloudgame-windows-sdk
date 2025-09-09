#include "Logger.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/EnvInfoPrinter.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

Logger* Logger::m_instance = nullptr;

Logger* Logger::instance() {
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

Logger::Logger()
    : m_logLevel(DEBUG), m_logToFile(false), m_stopThread(false), m_logThread(nullptr)
{
    startLogThread();
}

Logger::~Logger() {
    stopLogThread();
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

void Logger::globalInit() {
    // 1. 初始化TCR SDK日志系统（C API方式）
    static TcrLogCallback logCallback;
    logCallback.user_data = nullptr;
    logCallback.on_log = [](void* /*user_data*/, TcrLogLevel level, const char* tag, const char* log) {
        QString msg = QString("[%1] %2").arg(tag).arg(log);
        switch (level) {
        case TCR_LOG_LEVEL_TRACE:
        case TCR_LOG_LEVEL_DEBUG:
            Logger::debug(msg);
            break;
        case TCR_LOG_LEVEL_INFO:
            Logger::info(msg);
            break;
        case TCR_LOG_LEVEL_WARN:
            Logger::warning(msg);
            break;
        case TCR_LOG_LEVEL_ERROR:
            Logger::error(msg);
            break;
        }
    };
    tcr_set_log_callback(&logCallback);

    // 2. 设置TCR SDK日志等级
    tcr_set_log_level(TCR_LOG_LEVEL_INFO);

    // 3. 创建日志目录并配置日志文件
    QString logDirPath = QCoreApplication::applicationDirPath() + "/logs";
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    Logger::instance()->setLogToFile(true, logDir.filePath("app.log"));

    // 打印运行环境信息
    EnvInfoPrinter::printEnvironmentInfo();
}

void Logger::setLogLevel(LogLevel level) {
    m_logLevel = level;
}

void Logger::setLogToFile(bool enable, const QString& filePath) {
    QMutexLocker locker(&m_queueMutex);
    m_logToFile = enable;
    if (enable) {
        if (m_logFile.isOpen()) {
            m_logFile.close();
        }
        m_logFilePath = filePath;
        m_logFile.setFileName(m_logFilePath);
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_textStream.setDevice(&m_logFile);
        }
    }
}

void Logger::setMaxFileSize(qint64 bytes) {
    m_maxFileSize = bytes;
}
void Logger::setMaxBackupFiles(int count) {
    m_maxBackupFiles = count;
}

void Logger::log(LogLevel level, const QString& message) {
    if (level < m_logLevel) return;

    QString threadIdStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    LogMsg logMsg{level, message, timeStr, threadIdStr};

    {
        QMutexLocker locker(&m_queueMutex);
        m_logQueue.push(logMsg);
        m_queueNotEmpty.wakeOne();
    }
}

void Logger::rotateLogFileIfNeeded() {
    if (!m_logToFile || !m_logFile.isOpen()) return;
    if (m_logFile.size() < m_maxFileSize) return;

    m_textStream.flush();
    m_logFile.close();
    doRotate();
    m_logFile.setFileName(m_logFilePath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_textStream.setDevice(&m_logFile);
}

void Logger::doRotate() {
    // 删除最老的
    QString base = m_logFilePath;
    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString oldName = QString("%1.%2").arg(base).arg(i);
        QString newName = QString("%1.%2").arg(base).arg(i + 1);
        if (QFile::exists(oldName)) {
            QFile::remove(newName); // 保证不会rename失败
            QFile::rename(oldName, newName);
        }
    }
    // 当前日志文件 -> .1
    QString firstBackup = QString("%1.1").arg(base);
    QFile::remove(firstBackup);
    QFile::rename(base, firstBackup);
}

void Logger::logWriterLoop() {
    std::vector<QString> batch;
    batch.reserve(BATCH_SIZE);
    QElapsedTimer timer;
    timer.start();

    while (true) {
        {
            QMutexLocker locker(&m_queueMutex);
            while (m_logQueue.empty() && !m_stopThread) {
                m_queueNotEmpty.wait(&m_queueMutex, FLUSH_INTERVAL_MS);
                break; // 超时后也要检查flush
            }
            while (!m_logQueue.empty() && batch.size() < BATCH_SIZE) {
                const LogMsg& msg = m_logQueue.front();
                QString levelStr;
                switch(msg.level) {
                    case DEBUG: levelStr = "DEBUG"; break;
                    case INFO: levelStr = "INFO "; break;
                    case WARNING: levelStr = "WARN "; break;
                    case ERROR: levelStr = "ERROR"; break;
                }
                QString logMsg = QString("[%1] [%2] [%3] %4")
                    .arg(msg.timeStr)
                    .arg(msg.threadIdStr)
                    .arg(levelStr)
                    .arg(msg.message);
                batch.push_back(logMsg);
                m_logQueue.pop();
            }
            if (m_stopThread && m_logQueue.empty() && batch.empty())
                break;
        }

        if (!batch.empty()) {
            // 文件
            if (m_logToFile && m_logFile.isOpen()) {
                for (const auto& logMsg : batch) {
                    m_textStream << logMsg << "\n";
                    qDebug() << logMsg;   // 同时输出到控制台
                }
                m_textStream.flush();
                rotateLogFileIfNeeded();
            }
            batch.clear();
            timer.restart();
        } else if (timer.elapsed() > FLUSH_INTERVAL_MS) {
            // 定时flush
            if (m_logToFile && m_logFile.isOpen()) {
                m_textStream.flush();
                rotateLogFileIfNeeded();
            }
            timer.restart();
        }
    }
}

void Logger::startLogThread() {
    m_stopThread = false;
    m_logThread = QThread::create([this]() { this->logWriterLoop(); });
    m_logThread->start();
}

void Logger::stopLogThread() {
    {
        QMutexLocker locker(&m_queueMutex);
        m_stopThread = true;
        m_queueNotEmpty.wakeAll();
    }
    if (m_logThread) {
        m_logThread->wait();
        delete m_logThread;
        m_logThread = nullptr;
    }
}

void Logger::debug(const QString& message) { instance()->log(DEBUG, message); }
void Logger::info(const QString& message) { instance()->log(INFO, message); }
void Logger::warning(const QString& message) { instance()->log(WARNING, message); }
void Logger::error(const QString& message) { instance()->log(ERROR, message); }
