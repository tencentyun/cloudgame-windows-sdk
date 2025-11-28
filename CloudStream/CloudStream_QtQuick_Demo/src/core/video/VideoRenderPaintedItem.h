#pragma once

#include <QQuickPaintedItem>
#include <QImage>
#include <QPointer>
#include "Frame.h"
#include "VideoTransformHelper.h"

/**
 * @brief 视频渲染组件（基于QPainter软件渲染）
 * 
 * 核心功能：
 * 1. 接收I420格式视频帧，转换为RGB并绘制到屏幕
 * 2. 支持0/90/180/270度旋转（用于适配云端手机屏幕旋转）
 * 3. 处理触摸事件并转换坐标（从客户端视图坐标系转换到云端屏幕坐标系）
 * 
 * 工作流程：
 * 1. 接收视频帧 → setFrame()
 * 2. 转换格式 → convertI420ToRGB()
 * 3. 应用旋转 → setRotationAngle()
 * 4. 绘制画面 → paint()
 * 5. 处理触摸 → handleTouchEvent() → 坐标转换 → 发送到云端
 * 
 * 旋转处理：
 * - 当云端手机屏幕旋转时，通过 setRotationAngle() 设置客户端旋转角度
 * - 旋转角度和云端屏幕尺寸由 StreamingViewModel 从 SDK 事件中获取
 * - VideoTransformHelper 负责具体的旋转变换和坐标转换计算
 */
class VideoRenderPaintedItem : public QQuickPaintedItem
{
    Q_OBJECT

public:
    explicit VideoRenderPaintedItem(QQuickItem* parent = nullptr);
    ~VideoRenderPaintedItem() override;

    /**
     * @brief 判断是否有有效的视频帧
     */
    Q_INVOKABLE bool hasFrame() const;
    
    /**
     * @brief 设置 StreamingViewModel（用于连接触摸事件信号）
     * @param viewModel StreamingViewModel 对象指针
     */
    Q_INVOKABLE void setStreamingViewModel(QObject* viewModel);
    
    /**
     * @brief 设置旋转角度和云端屏幕尺寸
     * @param angle 客户端旋转角度（0, 90, 180, 270）
     * @param videoWidth 云端手机屏幕宽度（像素）
     * @param videoHeight 云端手机屏幕高度（像素）
     * 
     * 此方法由 StreamingViewModel 在接收到 TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE 事件时调用
     */
    void setRotationAngle(qreal angle, int videoWidth = 0, int videoHeight = 0);

public slots:
    /**
     * @brief 设置新的视频帧
     * @param frame 视频帧数据（智能指针）
     */
    void setFrame(VideoFrameDataPtr frame);
    
    /**
     * @brief 处理触摸事件
     * @param x 触摸点X坐标（客户端视图坐标系）
     * @param y 触摸点Y坐标（客户端视图坐标系）
     * @param width 视图宽度
     * @param height 视图高度
     * @param eventType 事件类型（0=按下, 1=移动, 2=抬起）
     * @param timestamp 时间戳
     * 
     * 此方法会将坐标从客户端视图坐标系转换到云端屏幕坐标系，然后发送到云端
     */
    void handleTouchEvent(int x, int y, int width, int height, int eventType, qint64 timestamp);

signals:
    /**
     * @brief 触摸事件信号（转发给 StreamingViewModel 处理）
     * @param x 触摸点X坐标（云端屏幕坐标系）
     * @param y 触摸点Y坐标（云端屏幕坐标系）
     * @param width 云端屏幕宽度
     * @param height 云端屏幕高度
     * @param eventType 事件类型
     * @param timestamp 时间戳
     */
    void touchEventOccurred(int x, int y, int width, int height, int eventType, qint64 timestamp);

protected:
    /**
     * @brief 绘制视频帧（QPainter渲染）
     * @param painter QPainter对象
     */
    void paint(QPainter* painter) override;

private:
    /**
     * @brief 将I420格式转换为RGB格式
     * @param frame 视频帧数据
     */
    void convertI420ToRGB(const VideoFrameData* frame);

private:
    VideoFrameDataPtr m_frame;                   // 当前视频帧数据
    QImage m_image;                              // 转换后的RGB图像（用于绘制）
    bool m_needConvert = false;                  // 是否需要格式转换标志
    VideoTransformHelper m_transformHelper;      // 视频变换辅助类（处理旋转、尺寸、坐标转换）
    QPointer<QObject> m_streamingViewModel;      // StreamingViewModel 指针（使用 QPointer 自动处理生命周期）
};
