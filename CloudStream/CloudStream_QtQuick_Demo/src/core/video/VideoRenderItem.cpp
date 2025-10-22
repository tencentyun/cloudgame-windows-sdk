#include "VideoRenderItem.h"
#include "YuvNode.h"
#include "YuvTestPattern.h"
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
    
    // 更新帧数据
    m_frame = frame;
    m_frameDirty = true;
    
    // 如果之前没有帧，现在有了，发射首帧到达信号
    if (wasEmpty && hasFrame()) {
        emit firstFrameArrived();
    }
    
    // 触发重绘
    update();
}

/**
 * @brief 判断当前是否有有效的帧数据
 * 
 * 检查帧数据是否存在且有效（尺寸合法且YUV数据指针非空）。
 */
bool VideoRenderItem::hasFrame() const
{
    // 检查帧对象是否存在且尺寸有效
    if (!m_frame || m_frame->width <= 0 || m_frame->height <= 0) {
        return false;
    }
    
    // 检查YUV三个分量的数据指针是否都有效
    return m_frame->data_y != nullptr && 
           m_frame->data_u != nullptr && 
           m_frame->data_v != nullptr;
}

/**
 * @brief 更新渲染节点
 * 
 * 在渲染线程中被调用，负责创建或更新YUV渲染节点。
 * 如果没有有效帧数据，返回nullptr表示不渲染任何内容。
 */
QSGNode* VideoRenderItem::updatePaintNode(QSGNode* oldNode, UpdatePaintNodeData*)
{
    // 如果没有有效帧数据，删除旧节点并返回空
    if (!hasFrame()) {
        delete oldNode;
        return nullptr;
    }
    
    // 尝试复用旧节点
    YuvNode* node = dynamic_cast<YuvNode*>(oldNode);
    if (!node) {
        // 如果旧节点不是YuvNode类型，删除并创建新节点
        delete oldNode;
        node = new YuvNode();
    }
    
    // 更新节点的帧数据和渲染尺寸
    node->setFrame(window(), m_frame.data(), QSizeF(width(), height()), m_frameDirty);
    
    // 清除脏标记
    m_frameDirty = false;
    
    return node;
}
