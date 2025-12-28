#include "VideoRenderItem.h"
#include "YuvNode.h"
#include "D3D11Node.h"
#include "YuvTestPattern.h"
#include "utils/Logger.h"
#include <QSGNode>
#include <QSGGeometryNode>
#include <QSGTexture>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include <QOpenGLContext>
#include <QThread>

/**
 * @brief 构造函数
 * 
 * 初始化VideoRenderItem，设置ItemHasContents标志以启用自定义渲染。
 */
VideoRenderItem::VideoRenderItem(QQuickItem* parent)
    : QQuickItem(parent)
    , m_videoWidth(0)
    , m_videoHeight(0)
{
    // 设置标志，表示此Item有自定义渲染内容
    setFlag(ItemHasContents, true);
}

/**
 * @brief 析构函数
 */
VideoRenderItem::~VideoRenderItem()
{
    // 智能指针会自动释放资源
}

/**
 * @brief 设置新的视频帧数据
 * 
 * 接收新帧数据，更新内部状态，并触发重绘。
 * 如果是从无帧到有帧的转变，会发射firstFrameArrived信号。
 */
void VideoRenderItem::setFrame(VideoFrameDataPtr frame)
{
    // 记录设置前是否有帧
    bool wasEmpty = !hasFrame();
    
    // 重要：立即释放旧帧，避免窗口最小化时帧累积
    // 因为窗口最小化时updatePaintNode不会被调用，旧帧会一直保留
    if (m_frame) {
        // Logger::info(QString("[VideoRenderItem::setFrame] Releasing old frame before setting new one, "
        //                    "this=%1, old_frame_ptr=%2, old_timestamp_us=%3")
        //            .arg(reinterpret_cast<quintptr>(this))
        //            .arg(reinterpret_cast<quintptr>(m_frame->frame_handle))
        //            .arg(m_frame->timestamp_us));
        m_frame.reset();
    }
    
    // 更新帧数据
    m_frame = frame;
    m_frameDirty = true;
    
    // 更新视频宽高
    if (m_frame && (m_frame->width != m_videoWidth || m_frame->height != m_videoHeight)) {
        m_videoWidth = m_frame->width;
        m_videoHeight = m_frame->height;
        emit videoSizeChanged();
    }
    
    // 如果帧状态发生变化，发射信号
    if (wasEmpty && hasFrame()) {
        emit hasFrameChanged();
    }
    
    // 触发重绘
    update();
}

/**
 * @brief 判断当前是否有有效的帧数据
 * 
 * 检查帧数据是否存在且有效：
 * - 对于I420_CPU类型：检查尺寸和YUV数据指针
 * - 对于D3D11_GPU类型：检查尺寸和纹理指针
 */
bool VideoRenderItem::hasFrame() const
{
    // 检查帧对象是否存在且尺寸有效
    if (!m_frame || m_frame->width <= 0 || m_frame->height <= 0) {
        return false;
    }
    
    // 根据帧类型检查数据有效性
    if (m_frame->frame_type == VideoFrameType::I420_CPU) {
        // YUV420格式：检查三个分量的数据指针
        return m_frame->data_y != nullptr && 
               m_frame->data_u != nullptr && 
               m_frame->data_v != nullptr;
    } 
    else if (m_frame->frame_type == VideoFrameType::D3D11_GPU) {
        // D3D11纹理格式：检查纹理和设备指针
        return m_frame->d3d11_data.texture != nullptr && 
               m_frame->d3d11_data.device != nullptr;
    }
    
    return false;
}

/**
 * @brief 更新渲染节点
 * 
 * 在渲染线程中被调用，负责根据帧类型创建或更新对应的渲染节点。
 * - I420_CPU类型使用YuvNode渲染
 * - D3D11_GPU类型使用D3D11Node渲染
 * 如果没有有效帧数据，返回nullptr表示不渲染任何内容。
 */
QSGNode* VideoRenderItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    // 如果没有有效帧数据，删除旧节点并返回空
    if (!hasFrame()) {
        // Logger::info(QString("[VideoRenderItem::updatePaintNode] No frame data, not rendering, "
        //                    "this=%1, thread_id=%2")
        //            .arg(reinterpret_cast<quintptr>(this))
        //            .arg(reinterpret_cast<quintptr>(QThread::currentThreadId())));
        delete oldNode;
        return nullptr;
    }

    QSGNode* resultNode = nullptr;
    
    // 根据帧类型选择对应的渲染节点
    if (m_frame->frame_type == VideoFrameType::I420_CPU) 
    {
        // YUV420格式：使用YuvNode渲染
        YuvNode* yuvNode = dynamic_cast<YuvNode*>(oldNode);
        
        // 如果旧节点类型不匹配，删除并创建新节点
        if (!yuvNode) {
            delete oldNode;
            yuvNode = new YuvNode();
        }
        
        // 更新节点的帧数据和渲染尺寸
        yuvNode->setFrame(window(), m_frame.data(), QSizeF(width(), height()), m_frameDirty);
        resultNode = yuvNode;
    }
    else if (m_frame->frame_type == VideoFrameType::D3D11_GPU)
    {
        // D3D11纹理格式：使用D3D11Node渲染
        D3D11Node* d3d11Node = dynamic_cast<D3D11Node*>(oldNode);
        
        // 如果旧节点类型不匹配，删除并创建新节点
        if (!d3d11Node) {
            delete oldNode;
            d3d11Node = new D3D11Node();
            Logger::info(QString("[VideoRenderItem::updatePaintNode] Created new D3D11Node, "
                               "this=%1, thread_id=%2")
                       .arg(reinterpret_cast<quintptr>(this))
                       .arg(reinterpret_cast<quintptr>(QThread::currentThreadId())));
        }
        
        // 更新节点的帧数据和渲染尺寸
        d3d11Node->setFrame(window(), m_frame.data(), QSizeF(width(), height()), m_frameDirty);
        resultNode = d3d11Node;
    }
    else
    {
        // 未知类型，删除旧节点
        Logger::info(QString("[VideoRenderItem::updatePaintNode] Unknown frame type, "
                           "this=%1, frame_type=%2, thread_id=%3")
                   .arg(reinterpret_cast<quintptr>(this))
                   .arg(static_cast<int>(m_frame->frame_type))
                   .arg(reinterpret_cast<quintptr>(QThread::currentThreadId())));
        delete oldNode;
    }
    
    // 清除脏标记
    m_frameDirty = false;
    
    // 重要：渲染完成后立即释放帧引用
    // 此时纹理数据已经上传到GPU，不再需要持有CPU端的帧数据
    m_frame.reset();
    
    return resultNode;
}
