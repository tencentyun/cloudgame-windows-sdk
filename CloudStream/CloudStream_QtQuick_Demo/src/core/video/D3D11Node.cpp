#include "D3D11Node.h"
#include "D3D11Material.h"
#include "Frame.h"
#include <QSGGeometry>
#include <QRectF>

// ========== 构造与析构 ==========

D3D11Node::D3D11Node()
    : m_geometry(QSGGeometry::defaultAttributes_TexturedPoint2D(), 4)
{
    // 设置几何数据（4个顶点，带纹理坐标）
    setGeometry(&m_geometry);

    // 创建D3D11材质
    m_material = new D3D11Material();
    setMaterial(m_material);
    
    // 让Qt管理材质的生命周期
    setFlag(OwnsMaterial);
}

D3D11Node::~D3D11Node()
{
    // 材质由Qt自动管理释放（因为设置了OwnsMaterial标志）
}

// ========== 公共接口 ==========

void D3D11Node::setFrame(QQuickWindow* window, 
                        const VideoFrameData* frame, 
                        const QSizeF& itemSize, 
                        bool frameDirty)
{
    // 验证帧数据有效性和类型
    if (frame && 
        frame->frame_type == VideoFrameType::D3D11_GPU &&  // 仅处理D3D11格式
        frame->width > 0 && 
        frame->height > 0 &&
        frame->d3d11_data.texture && 
        frame->d3d11_data.device) 
    {
        // 更新材质（设置D3D11纹理）
        updateMaterial(window, 
                      frame->d3d11_data,
                      frame->width, 
                      frame->height, 
                      frameDirty);
    } 
    else 
    {
        // 清除材质（无效帧或不支持的类型）
        if (m_material) {
            m_material->clear();
        }
        m_frameWidth = 0;
        m_frameHeight = 0;
        m_lastTexture = nullptr;
    }
    
    // 更新几何信息（顶点位置和纹理坐标）
    updateGeometry(itemSize);
}

// ========== 私有方法 ==========

void D3D11Node::updateGeometry(const QSizeF& itemSize)
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

void D3D11Node::updateMaterial(QQuickWindow* window,
                              const D3D11TextureData& d3d11Data,
                              int width, 
                              int height, 
                              bool frameDirty)
{
    // 检查材质对象是否有效
    if (!m_material) {
        return;
    }

    // 检查是否需要更新纹理
    // 条件：帧数据已更新 或 帧参数发生变化
    bool needUpdate = frameDirty || 
                     width != m_frameWidth || 
                     height != m_frameHeight || 
                     d3d11Data.texture != m_lastTexture;
    
    if (needUpdate) 
    {
        // 设置D3D11纹理到材质
        m_material->setD3D11Texture(d3d11Data, width, height, window);
        
        // 缓存当前帧参数
        m_frameWidth = width;
        m_frameHeight = height;
        m_lastTexture = d3d11Data.texture;
        
        // 通知场景图材质已改变
        markDirty(QSGNode::DirtyMaterial);
    }
}