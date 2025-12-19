#pragma once

#include "Frame.h"
#include <QQuickItem>
#include <memory>
#include <vector>
#include <QVector>

class YuvNode;

/**
 * @brief 视频渲染QML Item组件
 * 
 * 负责接收视频帧数据并在QML场景中进行渲染。
 * 继承自QQuickItem，可直接在QML中使用。
 */
class VideoRenderItem : public QQuickItem
{
    Q_OBJECT
    
    // QML属性：当前是否有有效的帧数据
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY firstFrameArrived)
    
    // QML属性：视频宽度
    Q_PROPERTY(int videoWidth READ videoWidth NOTIFY videoSizeChanged)
    
    // QML属性：视频高度
    Q_PROPERTY(int videoHeight READ videoHeight NOTIFY videoSizeChanged)
    
public:
    /**
     * @brief 构造函数
     * @param parent 父QQuickItem对象
     */
    explicit VideoRenderItem(QQuickItem* parent = nullptr);
    
    /**
     * @brief 析构函数
     */
    ~VideoRenderItem();
    
    /**
     * @brief 判断当前是否有有效的帧数据
     * @return true表示有有效帧数据，false表示无数据或数据无效
     */
    bool hasFrame() const;
    
    /**
     * @brief 获取视频宽度
     * @return 视频宽度（像素）
     */
    int videoWidth() const { return m_videoWidth; }
    
    /**
     * @brief 获取视频高度
     * @return 视频高度（像素）
     */
    int videoHeight() const { return m_videoHeight; }

signals:
    /**
     * @brief 首帧到达信号
     * 
     * 当从无帧状态接收到第一个有效帧时发射此信号。
     */
    void firstFrameArrived();
    
    /**
     * @brief 视频尺寸变化信号
     * 
     * 当视频宽度或高度发生变化时发射此信号。
     */
    void videoSizeChanged();

protected:
    /**
     * @brief 更新渲染节点
     * 
     * QQuickItem的虚函数，在渲染线程中被调用，用于更新场景图节点。
     * 
     * @param oldNode 旧的场景图节点
     * @param data 更新数据（未使用）
     * @return 更新后的场景图节点
     */
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

private:
    VideoFrameDataPtr m_frame;        ///< 当前帧数据
    bool m_frameDirty = false;        ///< 帧数据脏标记，表示帧数据已更新需要重新渲染
    int m_videoWidth;
    int m_videoHeight; 
    
public slots:
    /**
     * @brief 设置新的视频帧数据
     * 
     * 接收新的视频帧并触发渲染更新。
     * 如果是首帧，会发射firstFrameArrived信号。
     * 
     * @param frame 新的视频帧数据智能指针
     */
    void setFrame(VideoFrameDataPtr frame);
};