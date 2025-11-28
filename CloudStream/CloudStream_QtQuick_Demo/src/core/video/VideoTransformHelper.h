#pragma once

#include <QPointF>
#include <QSizeF>
#include <QRectF>

/**
 * @brief 视频变换辅助类
 * 
 * 核心职责：
 * 1. 旋转管理：支持0/90/180/270度旋转
 * 2. 尺寸计算：根据旋转角度计算渲染尺寸（保持宽高比）
 * 3. 坐标转换：将触摸坐标从客户端视图坐标系转换到云端屏幕坐标系
 * 
 * ==================== 坐标系说明 ====================
 * 
 * 【客户端视图坐标系】
 * - 用户在客户端屏幕上看到的坐标系统
 * - 原点(0,0)在左上角，x向右递增，y向下递增
 * - 坐标范围：[0, viewWidth) × [0, viewHeight)
 * 
 * 【云端屏幕坐标系】
 * - 云端手机的实际屏幕坐标系统
 * - 原点(0,0)在左上角，x向右递增，y向下递增
 * - 坐标范围：[0, videoWidth) × [0, videoHeight)
 * - videoWidth/videoHeight 是云端手机屏幕的物理尺寸，不随旋转改变
 * 
 * ==================== 旋转处理逻辑 ====================
 * 
 * 当云端手机屏幕旋转时（TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE事件）：
 * 1. SDK 下发云端旋转角度 degree（"0_degree", "90_degree", "180_degree", "270_degree"）
 * 2. 客户端需要反向旋转来抵消云端旋转，保证用户看到正确的画面方向
 * 3. 映射关系：
 *    - 云端 0度   → 客户端旋转 0度   （无旋转）
 *    - 云端 90度  → 客户端旋转 270度 （逆时针90度）
 *    - 云端 180度 → 客户端旋转 180度 （旋转180度）
 *    - 云端 270度 → 客户端旋转 90度  （顺时针90度）
 * 
 * ==================== 渲染变换说明 ====================
 * 
 * 【0度旋转】
 * - 视图与云端屏幕方向一致
 * - 视图宽度对应云端屏幕宽度，视图高度对应云端屏幕高度
 * 
 * 【90度旋转】（客户端顺时针旋转90度）
 * - 云端屏幕逆时针旋转了90度，客户端需要顺时针旋转90度来抵消
 * - 视图宽度对应云端屏幕高度，视图高度对应云端屏幕宽度（宽高互换）
 * - 绘制时需要调整矩形位置，确保旋转后内容从左上角开始显示
 * 
 * 【180度旋转】
 * - 云端屏幕旋转了180度，客户端也旋转180度来抵消
 * - 视图宽度对应云端屏幕宽度，视图高度对应云端屏幕高度
 * 
 * 【270度旋转】（客户端逆时针旋转90度）
 * - 云端屏幕顺时针旋转了90度，客户端需要逆时针旋转90度来抵消
 * - 视图宽度对应云端屏幕高度，视图高度对应云端屏幕宽度（宽高互换）
 * - 绘制时需要调整矩形位置，确保旋转后内容从左上角开始显示
 */
class VideoTransformHelper
{
public:
    VideoTransformHelper();
    ~VideoTransformHelper() = default;

    // ==================== 旋转角度管理 ====================
    
    /**
     * @brief 设置旋转角度和云端屏幕尺寸
     * @param angle 客户端旋转角度（0, 90, 180, 270）
     * @param videoWidth 云端手机屏幕宽度（像素）
     * @param videoHeight 云端手机屏幕高度（像素）
     * @return 是否有变化
     * 
     * 注意：videoWidth/videoHeight 是云端手机屏幕的物理尺寸，不随旋转改变
     */
    bool setRotation(qreal angle, int videoWidth, int videoHeight);
    
    qreal rotationAngle() const { return m_rotationAngle; }
    int videoWidth() const { return m_videoWidth; }
    int videoHeight() const { return m_videoHeight; }
    
    static bool isValidRotationAngle(qreal angle);

    // ==================== 尺寸计算 ====================
    
    /**
     * @brief 计算渲染尺寸（保持宽高比）
     * @param targetSize 目标区域尺寸（客户端视图尺寸）
     * @param imageSize 图像原始尺寸
     * @return 计算后的渲染尺寸
     * 
     * 90度/270度旋转时：
     * - 云端屏幕宽高互换显示
     * - 使用旋转后的宽高比（videoHeight/videoWidth）进行缩放
     */
    QSizeF calculateRenderSize(const QSizeF& targetSize, const QSizeF& imageSize) const;
    
    /**
     * @brief 计算绘制矩形（考虑旋转后的位置调整）
     * @param renderSize 渲染尺寸
     * @return 绘制矩形（相对于旋转中心点的位置）
     * 
     * 旋转中心点为(0,0)，根据旋转角度调整矩形位置：
     * - 0度：矩形从(0,0)开始
     * - 90度：矩形从(0, -renderSize.width())开始
     * - 180度：矩形从(-renderSize.width(), -renderSize.height())开始
     * - 270度：矩形从(-renderSize.height(), 0)开始
     */
    QRectF calculateDrawRect(const QSizeF& renderSize) const;

    // ==================== 坐标转换 ====================
    
    /**
     * @brief 转换触摸坐标（从客户端视图坐标系到云端屏幕坐标系）
     * @param x 客户端视图X坐标
     * @param y 客户端视图Y坐标
     * @param viewWidth 客户端视图宽度
     * @param viewHeight 客户端视图高度
     * @return 云端屏幕坐标系中的坐标
     * 
     * 转换原理：
     * 1. 先将视图坐标归一化到[0,1]范围
     * 2. 根据旋转角度进行坐标轴映射
     * 3. 再映射到云端屏幕坐标系
     * 
     * 转换公式（videoW = m_videoWidth, videoH = m_videoHeight）：
     * - 0度：  (x,y) → (x * videoW / viewWidth, y * videoH / viewHeight)
     * - 90度： (x,y) → (y * videoW / viewHeight, (viewWidth - x) * videoH / viewWidth)
     * - 180度：(x,y) → ((viewWidth - x) * videoW / viewWidth, (viewHeight - y) * videoH / viewHeight)
     * - 270度：(x,y) → ((viewHeight - y) * videoW / viewHeight, x * videoH / viewWidth)
     * 
     * 注意：
     * - 90度/270度旋转时，viewWidth对应videoHeight，viewHeight对应videoWidth
     * - 公式中的videoW和videoH始终是云端屏幕的物理尺寸，不随旋转改变
     */
    QPointF transformTouchCoordinates(int x, int y, int viewWidth, int viewHeight) const;

private:
    qreal m_rotationAngle = 0.0;    // 客户端旋转角度（用于抵消云端旋转）
    int m_videoWidth = 0;           // 云端手机屏幕宽度（物理尺寸，不随旋转改变）
    int m_videoHeight = 0;          // 云端手机屏幕高度（物理尺寸，不随旋转改变）
};
