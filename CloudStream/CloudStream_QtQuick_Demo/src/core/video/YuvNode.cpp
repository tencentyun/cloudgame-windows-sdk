#include "YuvNode.h"
#include "YuvMaterial.h"
#include "Frame.h"
#include <QSGGeometry>
#include <QRectF>
#include <cstring>

// ========== 构造与析构 ==========

YuvNode::YuvNode()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    // 设置几何数据（4个顶点，带纹理坐标）
    setGeometry(&m_geometry);

    // 创建YUV材质
    m_material = new YuvMaterial();
    setMaterial(m_material);
    
    // 让Qt管理材质的生命周期
    setFlag(OwnsMaterial);
}

YuvNode::~YuvNode()
{
    // 材质由Qt自动管理释放（因为设置了OwnsMaterial标志）
}

// ========== 公共接口 ==========

void YuvNode::setFrame(QQuickWindow* window, 
                       const VideoFrameData* frame, 
                       const QSizeF& itemSize, 
                       bool frameDirty)
{
    // 验证帧数据有效性
    if (frame && 
        frame->width > 0 && 
        frame->height > 0 &&
        frame->data_y && 
        frame->data_u && 
        frame->data_v) 
    {
        // 更新材质（上传YUV数据到GPU）
        updateMaterial(window, 
                      frame->data_y, 
                      frame->data_u, 
                      frame->data_v,
                      frame->strideY, 
                      frame->strideU, 
                      frame->strideV,
                      frame->width, 
                      frame->height, 
                      frameDirty);
    } 
    else 
    {
        // 清除材质（无效帧）
        updateMaterial(window, 
                      nullptr, 
                      nullptr, 
                      nullptr, 
                      0, 0, 0, 
                      0, 0, 
                      frameDirty);
    }
    
    // 更新几何信息（顶点位置和纹理坐标）
    updateGeometry(itemSize);
}

// ========== 私有方法 ==========

void YuvNode::updateGeometry(const QSizeF& itemSize)
{
    // 获取顶点数据指针
    QSGGeometry::TexturedPoint2D* vertices = m_geometry.vertexDataAsTexturedPoint2D();
    
    // 定义渲染矩形区域
    QRectF rect(0, 0, itemSize.width(), itemSize.height());
    
    // 设置四个顶点的位置和纹理坐标
    // 顶点顺序: 左上, 右上, 左下, 右下
    // 纹理坐标: (0,0)左上角, (1,1)右下角
    vertices[0].set(rect.left(),  rect.top(),    0.0f, 0.0f); // 左上
    vertices[1].set(rect.right(), rect.top(),    1.0f, 0.0f); // 右上
    vertices[2].set(rect.left(),  rect.bottom(), 0.0f, 1.0f); // 左下
    vertices[3].set(rect.right(), rect.bottom(), 1.0f, 1.0f); // 右下
    
    // 标记顶点数据已修改
    m_geometry.markVertexDataDirty();
    
    // 通知场景图几何信息已改变
    markDirty(QSGNode::DirtyGeometry);
}

void YuvNode::updateMaterial(QQuickWindow* window,
                            const uint8_t* DataY,
                            const uint8_t* DataU,
                            const uint8_t* DataV,
                            int StrideY, 
                            int StrideU, 
                            int StrideV,
                            int width, 
                            int height, 
                            bool frameDirty)
{
    // 检查材质对象是否有效
    if (!m_material) {
        return;
    }

    // 处理有效的YUV数据
    if (DataY && DataU && DataV && width > 0 && height > 0) 
    {
        // 检查是否需要更新纹理
        // 条件：帧数据已更新 或 帧参数发生变化
        bool needUpdate = frameDirty || 
                         width != m_frameWidth || 
                         height != m_frameHeight || 
                         StrideY != m_strideY || 
                         StrideU != m_strideU || 
                         StrideV != m_strideV;
        
        if (needUpdate) 
        {
            // 上传YUV数据到GPU纹理
            m_material->setYuvData(DataY, DataU, DataV, 
                                  StrideY, StrideU, StrideV, 
                                  width, height, 
                                  window);
            
            // 缓存当前帧参数
            m_frameWidth = width;
            m_frameHeight = height;
            m_strideY = StrideY;
            m_strideU = StrideU;
            m_strideV = StrideV;
            
            // 通知场景图材质已改变
            markDirty(QSGNode::DirtyMaterial);
        }
    } 
    else 
    {
        // 清除材质和缓存参数
        m_material->clear();
        m_frameWidth = 0;
        m_frameHeight = 0;
        m_strideY = 0;
        m_strideU = 0;
        m_strideV = 0;
        
        // 通知场景图材质已改变
        markDirty(QSGNode::DirtyMaterial);
    }
}
