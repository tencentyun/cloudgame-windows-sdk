#include "Logger.h"
#include "tcr_c_api.h"
#include "tcr_types.h"
#include "utils/EnvInfoPrinter.h"
#include <QCoreApplication>
#include <QDir>
#include <QFileInfo>
#include <QElapsedTimer>
#include <QDebug>

// 静态成员初始化
Logger* Logger::m_instance = nullptr;

/**
 * @brief 获取Logger单例实例
 * @return Logger* 单例指针
 */
Logger* Logger::instance()
{
    if (!m_instance) {
        m_instance = new Logger();
    }
    return m_instance;
}

/**
 * @brief 构造函数
 * 
 * 初始化日志系统并启动日志写入线程
 */
Logger::Logger()
    : m_logLevel(DEBUG)
    , m_logToFile(false)
    , m_stopThread(false)
    , m_logThread(nullptr)
{
    startLogThread();
}

/**
 * @brief 析构函数
 * 
 * 停止日志写入线程并关闭日志文件
 */
Logger::~Logger()
{
    stopLogThread();
    
    if (m_logFile.isOpen()) {
        m_logFile.close();
    }
}

/**
 * @brief 全局初始化日志系统
 * 
 * 执行以下操作：
 * 1. 设置TCR SDK的日志回调函数
 * 2. 配置TCR SDK的日志级别
 * 3. 创建日志目录
 * 4. 配置日志文件路径
 * 5. 打印环境信息
 */
void Logger::globalInit()
{
    // 1. 初始化TCR SDK日志系统（使用C API）
    static TcrLogCallback logCallback;
    logCallback.user_data = nullptr;
    
    // 设置日志回调函数，将TCR SDK的日志转发到Logger系统
    logCallback.on_log = [](void* /*user_data*/, TcrLogLevel level, const char* tag, const char* log) {
        QString msg = QString("[%1] %2").arg(tag).arg(log);
        
        // 根据TCR日志级别映射到Logger日志级别
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

    // 2. 设置TCR SDK日志等级为INFO
    tcr_set_log_level(TCR_LOG_LEVEL_INFO);

    // 3. 创建日志目录
    QString logDirPath = QCoreApplication::applicationDirPath() + "/logs";
    QDir logDir(logDirPath);
    if (!logDir.exists()) {
        logDir.mkpath(".");
    }
    
    // 4. 配置日志文件路径并启用文件日志
    Logger::instance()->setLogToFile(true, logDir.filePath("app.log"));

    // 5. 打印运行环境信息
    EnvInfoPrinter::printEnvironmentInfo();
}

/**
 * @brief 设置日志级别
 * @param level 日志级别
 */
void Logger::setLogLevel(LogLevel level)
{
    m_logLevel = level;
}

/**
 * @brief 设置是否写入日志文件
 * @param enable 是否启用文件日志
 * @param filePath 日志文件路径
 */
void Logger::setLogToFile(bool enable, const QString& filePath)
{
    QMutexLocker locker(&m_queueMutex);
    
    m_logToFile = enable;
    
    if (enable) {
        // 关闭已打开的日志文件
        if (m_logFile.isOpen()) {
            m_logFile.close();
        }
        
        // 打开新的日志文件
        m_logFilePath = filePath;
        m_logFile.setFileName(m_logFilePath);
        
        if (m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text)) {
            m_textStream.setDevice(&m_logFile);
        }
    }
}

/**
 * @brief 设置单个日志文件的最大字节数
 * @param bytes 最大字节数
 */
void Logger::setMaxFileSize(qint64 bytes)
{
    m_maxFileSize = bytes;
}

/**
 * @brief 设置日志轮转保留的备份文件个数
 * @param count 备份文件个数
 */
void Logger::setMaxBackupFiles(int count)
{
    m_maxBackupFiles = count;
}

/**
 * @brief 记录日志
 * 
 * 将日志消息添加到队列中，由后台线程异步写入
 * 
 * @param level 日志级别
 * @param message 日志消息
 */
void Logger::log(LogLevel level, const QString& message)
{
    // 过滤低于当前日志级别的消息
    if (level < m_logLevel) {
        return;
    }

    // 获取线程ID和时间戳
    QString threadIdStr = QString::number(reinterpret_cast<quintptr>(QThread::currentThreadId()));
    QString timeStr = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

    // 构造日志消息
    LogMsg logMsg{level, message, timeStr, threadIdStr};

    // 将日志消息添加到队列
    {
        QMutexLocker locker(&m_queueMutex);
        m_logQueue.push(logMsg);
        m_queueNotEmpty.wakeOne();  // 唤醒日志写入线程
    }
}

/**
 * @brief 检查并在需要时轮转日志文件
 * 
 * 当日志文件大小超过设定的最大值时，执行轮转操作
 */
void Logger::rotateLogFileIfNeeded()
{
    // 检查是否需要轮转
    if (!m_logToFile || !m_logFile.isOpen()) {
        return;
    }
    
    if (m_logFile.size() < m_maxFileSize) {
        return;
    }

    // 执行轮转操作
    m_textStream.flush();
    m_logFile.close();
    
    doRotate();
    
    // 重新打开日志文件
    m_logFile.setFileName(m_logFilePath);
    m_logFile.open(QIODevice::WriteOnly | QIODevice::Append | QIODevice::Text);
    m_textStream.setDevice(&m_logFile);
}

/**
 * @brief 执行日志文件轮转操作
 * 
 * 轮转策略：
 * - app.log -> app.log.1
 * - app.log.1 -> app.log.2
 * - ...
 * - app.log.N-1 -> app.log.N
 * - 删除最老的备份文件（如果超过最大备份数）
 */
void Logger::doRotate()
{
    QString base = m_logFilePath;
    
    // 从后往前重命名备份文件
    for (int i = m_maxBackupFiles - 1; i >= 1; --i) {
        QString oldName = QString("%1.%2").arg(base).arg(i);
        QString newName = QString("%1.%2").arg(base).arg(i + 1);
        
        if (QFile::exists(oldName)) {
            QFile::remove(newName);         // 删除目标文件（如果存在）
            QFile::rename(oldName, newName); // 重命名
        }
    }
    
    // 将当前日志文件重命名为 .1
    QString firstBackup = QString("%1.1").arg(base);
    QFile::remove(firstBackup);
    QFile::rename(base, firstBackup);
}

/**
 * @brief 日志写入线程的主循环
 * 
 * 功能：
 * 1. 从队列中批量获取日志消息
 * 2. 批量写入文件和控制台
 * 3. 定期刷新缓冲区
 * 4. 检查并执行日志轮转
 */
void Logger::logWriterLoop()
{
    std::vector<QString> batch;
    batch.reserve(BATCH_SIZE);
    
    QElapsedTimer timer;
    timer.start();

    while (true) {
        // 从队列中获取日志消息
        {
            QMutexLocker locker(&m_queueMutex);
            
            // 等待队列非空或超时
            while (m_logQueue.empty() && !m_stopThread) {
                m_queueNotEmpty.wait(&m_queueMutex, FLUSH_INTERVAL_MS);
                break; // 超时后也要检查是否需要flush
            }
            
            // 批量获取日志消息
            while (!m_logQueue.empty() && batch.size() < BATCH_SIZE) {
                const LogMsg& msg = m_logQueue.front();
                
                // 格式化日志级别字符串
                QString levelStr;
                switch(msg.level) {
                    case DEBUG:   levelStr = "DEBUG"; break;
                    case INFO:    levelStr = "INFO "; break;
                    case WARNING: levelStr = "WARN "; break;
                    case ERROR:   levelStr = "ERROR"; break;
                }
                
                // 构造完整的日志消息
                QString logMsg = QString("[%1] [%2] [%3] %4")
                    .arg(msg.timeStr)
                    .arg(msg.threadIdStr)
                    .arg(levelStr)
                    .arg(msg.message);
                
                batch.push_back(logMsg);
                m_logQueue.pop();
            }
            
            // 检查是否需要退出线程
            if (m_stopThread && m_logQueue.empty() && batch.empty()) {
                break;
            }
        }

        // 批量写入日志
        if (!batch.empty()) {
            if (m_logToFile && m_logFile.isOpen()) {
                for (const auto& logMsg : batch) {
                    m_textStream << logMsg << "\n";
                    qDebug() << logMsg;   // 同时输出到控制台
                }
                
                m_textStream.flush();
                rotateLogFileIfNeeded();  // 检查是否需要轮转
            }
            
            batch.clear();
            timer.restart();
        }
        // 定时刷新缓冲区
        else if (timer.elapsed() > FLUSH_INTERVAL_MS) {
            if (m_logToFile && m_logFile.isOpen()) {
                m_textStream.flush();
                rotateLogFileIfNeeded();
            }
            timer.restart();
        }
    }
}

/**
 * @brief 启动日志写入线程
 */
void Logger::startLogThread()
{
    m_stopThread = false;
    m_logThread = QThread::create([this]() {
        this->logWriterLoop();
    });
    m_logThread->start();
}

/**
 * @brief 停止日志写入线程
 */
void Logger::stopLogThread()
{
    // 设置停止标志并唤醒线程
    {
        QMutexLocker locker(&m_queueMutex);
        m_stopThread = true;
        m_queueNotEmpty.wakeAll();
    }
    
    // 等待线程结束
    if (m_logThread) {
        m_logThread->wait();
        delete m_logThread;
        m_logThread = nullptr;
    }
}

// 便捷的静态日志记录方法实现
void Logger::debug(const QString& message)
{
    instance()->log(DEBUG, message);
}

void Logger::info(const QString& message)
{
    instance()->log(INFO, message);
}

void Logger::warning(const QString& message)
{
    instance()->log(WARNING, message);
}

void Logger::error(const QString& message)
{
    instance()->log(ERROR, message);
}
