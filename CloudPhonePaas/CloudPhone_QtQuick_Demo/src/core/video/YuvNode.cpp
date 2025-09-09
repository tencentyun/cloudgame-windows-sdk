#include "YuvNode.h"
#include "YuvMaterial.h"
#include "Frame.h"
#include <QSGGeometry>
#include <QRectF>
#include <cstring>

YuvNode::YuvNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    setGeometry(&m_geometry);

    m_material = new YuvMaterial();
    setMaterial(m_material);
    // 让Qt管理 m_material 的生命周期
    setFlag(OwnsMaterial); 
}

YuvNode::~YuvNode(){}

void YuvNode::setFrame(QQuickWindow* window, const VideoFrameData* frame, const QSizeF& itemSize, bool frameDirty)
{
    if (frame && frame->width > 0 && frame->height > 0 &&
        !frame->y.empty() && !frame->u.empty() && !frame->v.empty()) {
        updateMaterial(window, frame->y.data(), frame->u.data(), frame->v.data(),
                       frame->strideY, frame->strideU, frame->strideV,
                       frame->width, frame->height, frameDirty);
    } else {
        updateMaterial(window, nullptr, nullptr, nullptr, 0, 0, 0, 0, 0, frameDirty);
    }
    updateGeometry(itemSize);
}

void YuvNode::updateGeometry(const QSizeF& itemSize)
{
    QSGGeometry::TexturedPoint2D *vertices = m_geometry.vertexDataAsTexturedPoint2D();
    QRectF rect(0, 0, itemSize.width(), itemSize.height());
    
    // 顶点顺序: 左上, 右上, 左下, 右下
    vertices[0].set(rect.left(),  rect.top(),    0, 0);
    vertices[1].set(rect.right(), rect.top(),    1, 0);
    vertices[2].set(rect.left(),  rect.bottom(), 0, 1);
    vertices[3].set(rect.right(), rect.bottom(), 1, 1);
    m_geometry.markVertexDataDirty();
    markDirty(QSGNode::DirtyGeometry);
}

void YuvNode::updateMaterial(
    QQuickWindow* window,
    const uint8_t* DataY,
    const uint8_t* DataU,
    const uint8_t* DataV,
    int StrideY, 
    int StrideU, 
    int StrideV,
    int width, int height, 
    bool frameDirty)
{
    if (!m_material)
        return;

    if (DataY && DataU && DataV && width > 0 && height > 0) {
        if (frameDirty || width != m_frameWidth || height != m_frameHeight || 
            StrideY != m_strideY || StrideU != m_strideU || StrideV != m_strideV) {
            // 假设YuvMaterial已更新为支持分离平面
            m_material->setYuvData(DataY, DataU, DataV, StrideY, StrideU, StrideV, width, height, window);
            m_frameWidth = width;
            m_frameHeight = height;
            m_strideY = StrideY;
            m_strideU = StrideU;
            m_strideV = StrideV;
            markDirty(QSGNode::DirtyMaterial);
        }
    } else {
        m_material->clear();
        m_frameWidth = 0;
        m_frameHeight = 0;
        m_strideY = 0;
        m_strideU = 0;
        m_strideV = 0;
        markDirty(QSGNode::DirtyMaterial);
    }
}
