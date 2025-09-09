#pragma once

#include "Frame.h"
#include <QQuickItem>
#include <memory>
#include <vector>
#include <QVector>

class YuvNode;

// 视频渲染QML Item，负责接收视频帧并触发渲染
class VideoRenderItem : public QQuickItem
{
    Q_OBJECT
    // QML属性：当前是否有帧数据
    Q_PROPERTY(bool hasFrame READ hasFrame NOTIFY firstFrameArrived)
public:
    explicit VideoRenderItem(QQuickItem* parent = nullptr); // 构造函数
    ~VideoRenderItem();                                     // 析构函数

    // 判断当前是否有帧数据
    bool hasFrame() const;

signals:
    // 首帧到达时发射的信号
    void firstFrameArrived();

protected:
    // QQuickItem接口，更新渲染节点
    QSGNode* updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*) override;

private:
    VideoFrameDataPtr m_frame;   // 当前帧数据
    bool m_frameDirty = false;   // 帧数据是否已更新

public slots:
    // 设置新的视频帧数据
    void setFrame(VideoFrameDataPtr frame);
};