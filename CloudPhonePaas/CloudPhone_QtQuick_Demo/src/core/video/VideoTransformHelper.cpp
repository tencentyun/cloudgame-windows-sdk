#include "VideoTransformHelper.h"
#include "utils/Logger.h"
#include <QtMath>

VideoTransformHelper::VideoTransformHelper()
    : m_rotationAngle(0.0)
    , m_videoWidth(0)
    , m_videoHeight(0)
{
}

// ==================== 旋转角度管理 ====================

bool VideoTransformHelper::setRotation(qreal angle, int videoWidth, int videoHeight)
{
    if (!isValidRotationAngle(angle)) {
        Logger::warning(QString("[VideoTransformHelper] 无效的旋转角度: %1").arg(angle));
        return false;
    }
    
    m_rotationAngle = angle;
    m_videoWidth = videoWidth;
    m_videoHeight = videoHeight;
    
    return true;
}

bool VideoTransformHelper::isValidRotationAngle(qreal angle)
{
    return qFuzzyCompare(angle, 0.0) || 
           qFuzzyCompare(angle, 90.0) || 
           qFuzzyCompare(angle, 180.0) || 
           qFuzzyCompare(angle, 270.0);
}

// ==================== 尺寸计算 ====================

QSizeF VideoTransformHelper::calculateRenderSize(const QSizeF& targetSize, const QSizeF& imageSize) const
{
    // 优先使用设置的云端屏幕尺寸，否则使用图像尺寸
    QSizeF videoSize = (m_videoWidth > 0 && m_videoHeight > 0) 
                       ? QSizeF(m_videoWidth, m_videoHeight) 
                       : imageSize;
    
    // 90度/270度旋转：云端屏幕宽高互换显示，使用旋转后的宽高比
    if (qFuzzyCompare(m_rotationAngle, 90.0) || qFuzzyCompare(m_rotationAngle, 270.0)) {
        // 旋转后的宽高比 = 云端屏幕高度 / 云端屏幕宽度
        qreal rotatedAspectRatio = videoSize.height() / videoSize.width();
        qreal renderWidth = targetSize.width();
        qreal renderHeight = renderWidth * rotatedAspectRatio;
        
        // 如果高度超出目标区域，则以高度为基准重新计算
        if (renderHeight > targetSize.height()) {
            renderHeight = targetSize.height();
            renderWidth = renderHeight / rotatedAspectRatio;
        }
        
        return QSizeF(renderWidth, renderHeight);
    } else {
        // 0度/180度旋转：保持原始宽高比
        return videoSize.scaled(targetSize, Qt::KeepAspectRatio);
    }
}

QRectF VideoTransformHelper::calculateDrawRect(const QSizeF& renderSize) const
{
    // 根据旋转角度调整绘制矩形位置
    // 旋转中心点为(0,0)，需要调整矩形位置确保旋转后内容从左上角开始显示
    if (qFuzzyCompare(m_rotationAngle, 90.0)) {
        // 顺时针旋转90度：矩形需要向上偏移宽度
        return QRectF(0, -renderSize.width(), renderSize.height(), renderSize.width());
    } 
    else if (qFuzzyCompare(m_rotationAngle, 180.0)) {
        // 旋转180度：矩形需要向左上偏移宽度和高度
        return QRectF(-renderSize.width(), -renderSize.height(), renderSize.width(), renderSize.height());
    } 
    else if (qFuzzyCompare(m_rotationAngle, 270.0)) {
        // 逆时针旋转90度：矩形需要向左偏移高度
        return QRectF(-renderSize.height(), 0, renderSize.height(), renderSize.width());
    } 
    else {
        // 无旋转：矩形从(0,0)开始
        return QRectF(0, 0, renderSize.width(), renderSize.height());
    }
}

// ==================== 坐标转换 ====================

QPointF VideoTransformHelper::transformTouchCoordinates(int x, int y, 
                                                        int viewWidth, int viewHeight) const
{
    // 获取云端屏幕尺寸（物理尺寸，不随旋转改变）
    int videoW = (m_videoWidth > 0) ? m_videoWidth : viewWidth;
    int videoH = (m_videoHeight > 0) ? m_videoHeight : viewHeight;
    
    QPointF result;
    
    if (qFuzzyCompare(m_rotationAngle, 90.0)) {
        // 客户端顺时针旋转90度（云端逆时针旋转90度）
        // 视图的x轴映射到云端屏幕的y轴
        // 视图的y轴反向映射到云端屏幕的x轴
        result.setX(y * videoW / viewHeight);
        result.setY((viewWidth - x) * videoH / viewWidth);
    } 
    else if (qFuzzyCompare(m_rotationAngle, 180.0)) {
        // 旋转180度：x和y都反向映射
        result.setX((viewWidth - x) * videoW / viewWidth);
        result.setY((viewHeight - y) * videoH / viewHeight);
    } 
    else if (qFuzzyCompare(m_rotationAngle, 270.0)) {
        // 客户端逆时针旋转90度（云端顺时针旋转90度）
        // 视图的x轴反向映射到云端屏幕的y轴
        // 视图的y轴映射到云端屏幕的x轴
        result.setX((viewHeight - y) * videoW / viewHeight);
        result.setY(x * videoH / viewWidth);
    }
    else { // 0度：x和y都映射
        result.setX(x * videoW / viewWidth);
        result.setY(y * videoH / viewHeight);
    }
    
    return result;
}
