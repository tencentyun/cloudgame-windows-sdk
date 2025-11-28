#pragma once

#include <QFile>
#include <QString>
#include <QObject>
#include "tcr_c_api.h"

/**
 * @brief YUV视频帧保存工具类
 * 
 * 用于将I420格式的视频帧保存到YUV文件，便于调试和问题排查。
 * 主要用途：
 *   - 验证视频帧数据的完整性
 *   - 分析分辨率变化时的帧数据
 * 
 * 使用示例：
 * @code
 * // 1. 创建实例（可选：指定最大帧数）
 * YuvFrameDumper dumper(300);  // 最多保存300帧
 * 
 * // 2. 启用保存功能
 * dumper.setEnabled(true);
 * 
 * // 3. 在视频帧回调中保存帧数据
 * dumper.saveFrame(i420Buffer);
 * 
 * // 4. 停止保存（可选，达到最大帧数会自动停止）
 * dumper.setEnabled(false);
 * @endcode
 * 
 * 输出文件格式：
 *   - 文件名：debug_stream_{width}x{height}.yuv
 *   - 格式：YUV420P (I420)
 *   - 播放命令：ffplay -f rawvideo -pixel_format yuv420p -video_size {width}x{height} {filename}
 */
class YuvFrameDumper
{
public:
    /**
     * @brief 构造函数
     * @param maxFrames 最大保存帧数（默认300帧，约10秒@30fps）
     */
    explicit YuvFrameDumper(int maxFrames = 300);
    
    /**
     * @brief 析构函数，自动关闭文件
     */
    ~YuvFrameDumper();

    /**
     * @brief 启用或禁用帧保存功能
     * @param enabled true=启用，false=禁用
     * 
     * 注意：禁用后会关闭当前文件，重新启用会创建新文件
     */
    void setEnabled(bool enabled);
    
    /**
     * @brief 检查是否已启用
     * @return true=已启用，false=未启用
     */
    bool isEnabled() const { return m_enabled; }
    
    /**
     * @brief 设置最大保存帧数
     * @param maxFrames 最大帧数
     * 
     * 注意：修改此值会重置帧计数器
     */
    void setMaxFrames(int maxFrames);
    
    /**
     * @brief 获取当前已保存的帧数
     * @return 已保存的帧数
     */
    int getFrameCount() const { return m_frameCount; }
    
    /**
     * @brief 重置帧计数器和文件
     * 
     * 调用此方法后：
     *   - 关闭当前文件
     *   - 重置帧计数器为0
     *   - 清除分辨率记录
     */
    void reset();
    
    /**
     * @brief 保存I420格式的视频帧
     * @param i420Buffer I420格式的视频帧缓冲区
     * @return true=保存成功，false=保存失败或已达到最大帧数
     * 
     * 功能说明：
     *   - 自动检测分辨率变化，变化时创建新文件
     *   - 逐行写入YUV数据，跳过stride填充
     *   - 达到最大帧数后自动停止保存
     *   - 每30帧输出一次日志（避免刷屏）
     */
    bool saveFrame(const TcrI420Buffer& i420Buffer);

private:
    /**
     * @brief 创建或打开YUV文件
     * @param width 视频宽度
     * @param height 视频高度
     * @return true=成功，false=失败
     */
    bool openFile(int width, int height);
    
    /**
     * @brief 关闭当前文件
     */
    void closeFile();
    
    /**
     * @brief 写入YUV数据到文件
     * @param i420Buffer I420格式的视频帧缓冲区
     * @return true=成功，false=失败
     */
    bool writeYuvData(const TcrI420Buffer& i420Buffer);

private:
    bool m_enabled;          ///< 是否启用保存功能
    int m_frameCount;        ///< 当前已保存的帧数
    int m_maxFrames;         ///< 最大保存帧数
    int m_lastWidth;         ///< 上一帧的宽度（用于检测分辨率变化）
    int m_lastHeight;        ///< 上一帧的高度（用于检测分辨率变化）
    QFile* m_yuvFile;        ///< YUV文件句柄
};
