#include "YuvFrameDumper.h"
#include "Logger.h"
#include <QDebug>

// ==================== 构造与析构 ====================

YuvFrameDumper::YuvFrameDumper(int maxFrames)
    : m_enabled(true)
    , m_frameCount(0)
    , m_maxFrames(maxFrames)
    , m_lastWidth(0)
    , m_lastHeight(0)
    , m_yuvFile(nullptr)
{
    Logger::info(QString("[YuvFrameDumper] 创建实例，最大帧数: %1").arg(m_maxFrames));
}

YuvFrameDumper::~YuvFrameDumper()
{
    closeFile();
    Logger::info("[YuvFrameDumper] 销毁实例");
}

// ==================== 公共接口 ====================

void YuvFrameDumper::setEnabled(bool enabled)
{
    if (m_enabled == enabled) {
        return;
    }
    
    m_enabled = enabled;
    
    if (enabled) {
        Logger::info("[YuvFrameDumper] 启用帧保存功能");
    } else {
        Logger::info("[YuvFrameDumper] 禁用帧保存功能");
        closeFile();
    }
}

void YuvFrameDumper::setMaxFrames(int maxFrames)
{
    if (maxFrames <= 0) {
        Logger::warning(QString("[YuvFrameDumper] 无效的最大帧数: %1，保持原值: %2")
            .arg(maxFrames).arg(m_maxFrames));
        return;
    }
    
    m_maxFrames = maxFrames;
    Logger::info(QString("[YuvFrameDumper] 设置最大帧数: %1").arg(m_maxFrames));
    
    // 重置计数器
    reset();
}

void YuvFrameDumper::reset()
{
    Logger::info("[YuvFrameDumper] 重置状态");
    
    closeFile();
    m_frameCount = 0;
    m_lastWidth = 0;
    m_lastHeight = 0;
}

bool YuvFrameDumper::saveFrame(const TcrI420Buffer& i420Buffer)
{
    // 检查是否启用
    if (!m_enabled) {
        return false;
    }
    
    // 检查是否已达到最大帧数
    if (m_frameCount >= m_maxFrames) {
        return false;
    }
    
    // 检查数据有效性
    if (!i420Buffer.data_y || !i420Buffer.data_u || !i420Buffer.data_v) {
        Logger::error("[YuvFrameDumper] 无效的I420数据指针");
        return false;
    }
    
    if (i420Buffer.width <= 0 || i420Buffer.height <= 0) {
        Logger::error(QString("[YuvFrameDumper] 无效的分辨率: %1x%2")
            .arg(i420Buffer.width).arg(i420Buffer.height));
        return false;
    }
    
    // 检测分辨率变化
    bool resolutionChanged = (i420Buffer.width != m_lastWidth || 
                             i420Buffer.height != m_lastHeight);
    
    if (resolutionChanged) {
        Logger::warning(QString("[YuvFrameDumper] 检测到分辨率变化: %1x%2 -> %3x%4 (帧#%5)")
            .arg(m_lastWidth).arg(m_lastHeight)
            .arg(i420Buffer.width).arg(i420Buffer.height)
            .arg(m_frameCount));
        
        // 关闭旧文件
        closeFile();
        
        // 更新分辨率记录
        m_lastWidth = i420Buffer.width;
        m_lastHeight = i420Buffer.height;
    }
    
    // 打开或创建文件
    if (!m_yuvFile) {
        if (!openFile(i420Buffer.width, i420Buffer.height)) {
            return false;
        }
    }
    
    // 写入YUV数据
    if (!writeYuvData(i420Buffer)) {
        return false;
    }
    
    m_frameCount++;
    
    // 每30帧输出一次日志（避免刷屏）
    if (m_frameCount % 30 == 0) {
        Logger::debug(QString("[YuvFrameDumper] 已保存 %1 帧 (%2x%3)")
            .arg(m_frameCount)
            .arg(i420Buffer.width)
            .arg(i420Buffer.height));
    }
    
    // 达到最大帧数后自动停止
    if (m_frameCount >= m_maxFrames) {
        Logger::info(QString("[YuvFrameDumper] 已保存 %1 帧，达到最大帧数，停止录制")
            .arg(m_frameCount));
        closeFile();
    }
    
    return true;
}

// ==================== 私有方法 ====================

bool YuvFrameDumper::openFile(int width, int height)
{
    // 生成文件名
    QString filename = QString("debug_stream_%1x%2.yuv")
        .arg(width)
        .arg(height);
    
    // 创建文件对象
    m_yuvFile = new QFile(filename);
    
    // 以追加模式打开文件
    if (!m_yuvFile->open(QIODevice::WriteOnly | QIODevice::Append)) {
        Logger::error(QString("[YuvFrameDumper] 无法创建文件: %1").arg(filename));
        delete m_yuvFile;
        m_yuvFile = nullptr;
        return false;
    }
    
    Logger::info(QString("[YuvFrameDumper] 创建YUV文件: %1").arg(filename));
    Logger::info(QString("[YuvFrameDumper] 播放命令: ffplay -f rawvideo -pixel_format yuv420p -video_size %1x%2 %3")
        .arg(width)
        .arg(height)
        .arg(filename));
    
    return true;
}

void YuvFrameDumper::closeFile()
{
    if (m_yuvFile) {
        if (m_yuvFile->isOpen()) {
            m_yuvFile->flush();
            m_yuvFile->close();
            Logger::debug("[YuvFrameDumper] 关闭YUV文件");
        }
        delete m_yuvFile;
        m_yuvFile = nullptr;
    }
}

bool YuvFrameDumper::writeYuvData(const TcrI420Buffer& i420Buffer)
{
    if (!m_yuvFile || !m_yuvFile->isOpen()) {
        Logger::error("[YuvFrameDumper] 文件未打开");
        return false;
    }
    
    // 写入Y平面（逐行写入，跳过stride填充）
    for (int row = 0; row < i420Buffer.height; ++row) {
        qint64 bytesWritten = m_yuvFile->write(
            reinterpret_cast<const char*>(i420Buffer.data_y + row * i420Buffer.stride_y),
            i420Buffer.width
        );
        
        if (bytesWritten != i420Buffer.width) {
            Logger::error(QString("[YuvFrameDumper] Y平面写入失败，行: %1").arg(row));
            return false;
        }
    }
    
    // 写入U平面（YUV420格式，宽高减半）
    int uvWidth = i420Buffer.width / 2;
    int uvHeight = i420Buffer.height / 2;
    
    for (int row = 0; row < uvHeight; ++row) {
        qint64 bytesWritten = m_yuvFile->write(
            reinterpret_cast<const char*>(i420Buffer.data_u + row * i420Buffer.stride_u),
            uvWidth
        );
        
        if (bytesWritten != uvWidth) {
            Logger::error(QString("[YuvFrameDumper] U平面写入失败，行: %1").arg(row));
            return false;
        }
    }
    
    // 写入V平面
    for (int row = 0; row < uvHeight; ++row) {
        qint64 bytesWritten = m_yuvFile->write(
            reinterpret_cast<const char*>(i420Buffer.data_v + row * i420Buffer.stride_v),
            uvWidth
        );
        
        if (bytesWritten != uvWidth) {
            Logger::error(QString("[YuvFrameDumper] V平面写入失败，行: %1").arg(row));
            return false;
        }
    }
    
    // 刷新缓冲区
    m_yuvFile->flush();
    
    return true;
}
