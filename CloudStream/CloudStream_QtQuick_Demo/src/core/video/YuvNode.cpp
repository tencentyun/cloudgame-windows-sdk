#include "YuvNode.h"
#include "YuvMaterial.h"
#include "Frame.h"
#include "utils/Logger.h"
#include <QSGGeometry>
#include <QRectF>
#include <cstring>

// ========== YuvNode 构造与析构 ==========

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
    // [PERF] 记录开始时间
    qint64 startTime = QDateTime::currentMSecsSinceEpoch();
    qint64 updateMaterialTime = 0;
    qint64 updateGeometryTime = 0;
    
    // 验证帧数据有效性和类型
    if (frame && 
        frame->frame_type == VideoFrameType::I420_CPU &&  // 仅处理YUV420格式
        frame->width > 0 && 
        frame->height > 0 &&
        frame->data_y && 
        frame->data_u && 
        frame->data_v) 
    {
        // [PERF] 记录updateMaterial前的时间
        qint64 beforeUpdateMaterial = QDateTime::currentMSecsSinceEpoch();
        
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
        
        // [PERF] 计算updateMaterial耗时
        updateMaterialTime = QDateTime::currentMSecsSinceEpoch() - beforeUpdateMaterial;
    } 
    else 
    {
        // 清除材质（无效帧或不支持的类型）
        // TODO: 未来在此处添加D3D11_GPU类型的处理分支
        qint64 beforeUpdateMaterial = QDateTime::currentMSecsSinceEpoch();
        
        updateMaterial(window, 
                      nullptr, 
                      nullptr, 
                      nullptr, 
                      0, 0, 0, 
                      0, 0, 
                      frameDirty);
        
        updateMaterialTime = QDateTime::currentMSecsSinceEpoch() - beforeUpdateMaterial;
    }
    
    // [PERF] 记录updateGeometry前的时间
    qint64 beforeUpdateGeometry = QDateTime::currentMSecsSinceEpoch();
    
    // 更新几何信息（顶点位置和纹理坐标）
    updateGeometry(itemSize);
    
    // [PERF] 计算updateGeometry耗时
    updateGeometryTime = QDateTime::currentMSecsSinceEpoch() - beforeUpdateGeometry;
    
    // [PERF] 计算总耗时
    qint64 totalTime = QDateTime::currentMSecsSinceEpoch() - startTime;
    
    // [PERF] 只打印超过20ms的异常情况
    if (totalTime > 20) {
        Logger::warning(QString("[YuvNode::setFrame] !!! SLOW OPERATION !!! "
                              "Total=%1ms (updateMaterial=%2ms, updateGeometry=%3ms), "
                              "frame_size=%4x%5, frameDirty=%6, this=%7")
                      .arg(totalTime)
                      .arg(updateMaterialTime)
                      .arg(updateGeometryTime)
                      .arg(frame ? frame->width : 0)
                      .arg(frame ? frame->height : 0)
                      .arg(frameDirty)
                      .arg(reinterpret_cast<quintptr>(this)));
    }
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
            // [PERF] 记录setYuvData前的时间
            qint64 beforeSetYuvData = QDateTime::currentMSecsSinceEpoch();
            
            // 上传YUV数据到GPU纹理
            m_material->setYuvData(DataY, DataU, DataV, 
                                  StrideY, StrideU, StrideV, 
                                  width, height, 
                                  window);
            
            // [PERF] 计算setYuvData耗时
            qint64 setYuvDataTime = QDateTime::currentMSecsSinceEpoch() - beforeSetYuvData;
            
            // [PERF] 只打印超过20ms的异常情况
            if (setYuvDataTime > 20) {
                Logger::warning(QString("[YuvNode::updateMaterial] !!! SLOW TEXTURE UPLOAD !!! "
                                      "setYuvData took %1ms, frame_size=%2x%3, "
                                      "strides=(%4,%5,%6), this=%7")
                              .arg(setYuvDataTime)
                              .arg(width)
                              .arg(height)
                              .arg(StrideY)
                              .arg(StrideU)
                              .arg(StrideV)
                              .arg(reinterpret_cast<quintptr>(this)));
            }
            
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
