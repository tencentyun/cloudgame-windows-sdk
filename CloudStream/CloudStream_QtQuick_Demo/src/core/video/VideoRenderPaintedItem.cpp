#include "VideoRenderPaintedItem.h"
#include "viewmodels/StreamingViewModel.h"
#include "utils/Logger.h"
#include <QPainter>
#include <QDebug>

VideoRenderPaintedItem::VideoRenderPaintedItem(QQuickItem* parent)
    : QQuickPaintedItem(parent)
{
}

VideoRenderPaintedItem::~VideoRenderPaintedItem()
{
    if (!m_streamingViewModel.isNull()) {
        disconnect(this, nullptr, m_streamingViewModel.data(), nullptr);
    }
}

void VideoRenderPaintedItem::setFrame(VideoFrameDataPtr frame)
{
    m_frame = frame;
    
    if (m_frame && m_frame->frame_type == VideoFrameType::I420_CPU) {
        m_needConvert = true;
    } else if (m_frame && m_frame->frame_type != VideoFrameType::I420_CPU) {
        qWarning() << "VideoRenderPaintedItem只支持I420_CPU格式";
        m_frame.reset();
    }
    
    update();
}

bool VideoRenderPaintedItem::hasFrame() const
{
    if (!m_frame || m_frame->width <= 0 || m_frame->height <= 0) {
        return false;
    }
    
    if (m_frame->frame_type == VideoFrameType::I420_CPU) {
        return m_frame->data_y != nullptr && 
               m_frame->data_u != nullptr && 
               m_frame->data_v != nullptr;
    }
    
    return false;
}

void VideoRenderPaintedItem::paint(QPainter* painter)
{
    if (!painter || !hasFrame()) {
        return;
    }
    
    // 如果需要格式转换，先将I420转换为RGB
    if (m_needConvert) {
        convertI420ToRGB(m_frame.data());
        m_needConvert = false;
    }
    
    if (!m_image.isNull()) {
        QRectF targetRect = boundingRect();
        painter->save();
        
        // 计算渲染尺寸（保持宽高比）
        QSizeF renderSize = m_transformHelper.calculateRenderSize(targetRect.size(), m_image.size());
        
        // 如果有旋转，需要应用旋转变换
        if (m_transformHelper.rotationAngle() != 0.0) {
            // 设置旋转中心点为(0,0)
            painter->translate(0, 0);
            // 应用旋转
            painter->rotate(m_transformHelper.rotationAngle());
            // 计算旋转后的绘制矩形位置
            QRectF drawRect = m_transformHelper.calculateDrawRect(renderSize);
            // 绘制图像
            painter->drawImage(drawRect, m_image);
        } else {
            // 无旋转：直接绘制
            QRectF drawRect(0, 0, renderSize.width(), renderSize.height());
            painter->drawImage(drawRect, m_image);
        }
        
        painter->restore();
    }
}

void VideoRenderPaintedItem::convertI420ToRGB(const VideoFrameData* frame)
{
    if (!frame || frame->frame_type != VideoFrameType::I420_CPU) {
        return;
    }
    
    const int width = frame->width;
    const int height = frame->height;
    
    m_image = QImage(width, height, QImage::Format_RGB888);
    
    const uint8_t* yData = frame->data_y;
    const uint8_t* uData = frame->data_u;
    const uint8_t* vData = frame->data_v;
    
    const int strideY = frame->strideY;
    const int strideU = frame->strideU;
    const int strideV = frame->strideV;
    
    // YUV420转RGB：R = Y + 1.402*(V-128), G = Y - 0.344*(U-128) - 0.714*(V-128), B = Y + 1.772*(U-128)
    for (int y = 0; y < height; ++y) {
        uint8_t* rgbLine = m_image.scanLine(y);
        
        for (int x = 0; x < width; ++x) {
            int yValue = yData[y * strideY + x];
            int uValue = uData[(y / 2) * strideU + (x / 2)] - 128;
            int vValue = vData[(y / 2) * strideV + (x / 2)] - 128;
            
            int r = yValue + static_cast<int>(1.402 * vValue);
            int g = yValue - static_cast<int>(0.344 * uValue + 0.714 * vValue);
            int b = yValue + static_cast<int>(1.772 * uValue);
            
            r = qBound(0, r, 255);
            g = qBound(0, g, 255);
            b = qBound(0, b, 255);
            
            rgbLine[x * 3 + 0] = static_cast<uint8_t>(r);
            rgbLine[x * 3 + 1] = static_cast<uint8_t>(g);
            rgbLine[x * 3 + 2] = static_cast<uint8_t>(b);
        }
    }
}

void VideoRenderPaintedItem::setRotationAngle(qreal angle, int videoWidth, int videoHeight)
{
    if (m_transformHelper.setRotation(angle, videoWidth, videoHeight)) {
        update();
    }
}

void VideoRenderPaintedItem::setStreamingViewModel(QObject* viewModel)
{
    if (!viewModel) {
        Logger::warning("[VideoRenderPaintedItem] setStreamingViewModel 接收到空指针");
        return;
    }
    
    if (!m_streamingViewModel.isNull()) {
        disconnect(this, SIGNAL(touchEventOccurred(int,int,int,int,int,qint64)),
                   m_streamingViewModel.data(), SLOT(sendTouchEvent(int,int,int,int,int,qint64)));
    }
    
    StreamingViewModel* streamingVM = qobject_cast<StreamingViewModel*>(viewModel);
    if (!streamingVM) {
        Logger::error(QString("[VideoRenderPaintedItem] 无法转换为 StreamingViewModel，实际类型: %1")
                          .arg(viewModel->metaObject()->className()));
        return;
    }
    
    m_streamingViewModel = viewModel;
    
    bool connected = connect(this, &VideoRenderPaintedItem::touchEventOccurred,
                             streamingVM, &StreamingViewModel::sendTouchEvent,
                             Qt::QueuedConnection);
    
    if (!connected) {
        Logger::error("[VideoRenderPaintedItem] 信号槽连接失败");
        m_streamingViewModel = nullptr;
    }
}

void VideoRenderPaintedItem::handleTouchEvent(int x, int y, int width, int height, 
                                              int eventType, qint64 timestamp)
{
    if (m_streamingViewModel.isNull()) {
        Logger::warning("[VideoRenderPaintedItem] 未连接到 StreamingViewModel");
        return;
    }
    
    // 将触摸坐标从客户端视图坐标系转换到云端屏幕坐标系
    QPointF transformedPos = m_transformHelper.transformTouchCoordinates(x, y, width, height);
    
    // 获取云端屏幕尺寸
    int videoWidth = m_transformHelper.videoWidth();
    int videoHeight = m_transformHelper.videoHeight();
    
    // 如果未设置云端屏幕尺寸，使用视图尺寸作为默认值
    if (videoWidth <= 0 || videoHeight <= 0) {
        videoWidth = width;
        videoHeight = height;
    }
    
    // 发送转换后的坐标和云端屏幕尺寸到 StreamingViewModel
    emit touchEventOccurred(
        static_cast<int>(transformedPos.x()),
        static_cast<int>(transformedPos.y()),
        videoWidth,
        videoHeight,
        eventType,
        timestamp
    );
}
