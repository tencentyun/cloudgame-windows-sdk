#include "YuvMaterial.h"
#include "YuvDynamicTexture.h"
#include "utils/Logger.h"
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QQuickWindow>
#include <QSize>
#include <QImage>
#include <cstring>

// ========================================
// YuvMaterialShader - YUV着色器实现
// ========================================

/**
 * @brief YUV材质着色器类
 * 
 * 负责加载和管理YUV到RGB转换的着色器程序，
 * 并处理uniform数据和纹理采样的绑定。
 */
class YuvMaterialShader : public QSGMaterialShader
{
public:
    /**
     * @brief 构造函数
     * 
     * 加载预编译的顶点和片段着色器（.qsb格式）
     */
    YuvMaterialShader()
    {
        // 设置顶点着色器
        setShaderFileName(VertexStage, QLatin1String(":/shaders/shaders/yuv.vert.qsb"));
        
        // 设置片段着色器
        setShaderFileName(FragmentStage, QLatin1String(":/shaders/shaders/yuv.frag.qsb"));
    }

    /**
     * @brief 更新uniform数据
     * 
     * 主要用于更新MVP（模型-视图-投影）矩阵
     * 
     * @param state 渲染状态对象
     * @param newMaterial 新材质对象
     * @param oldMaterial 旧材质对象
     * @return 如果uniform数据发生变化返回true
     */
    bool updateUniformData(RenderState& state, 
                          QSGMaterial* newMaterial, 
                          QSGMaterial* oldMaterial) override
    {
        bool changed = false;
        QByteArray* buf = state.uniformData();
        
        // 检查变换矩阵是否需要更新
        if (state.isMatrixDirty()) 
        {
            // 获取组合变换矩阵（模型-视图-投影）
            const QMatrix4x4 m = state.combinedMatrix();
            
            // 将矩阵数据复制到uniform缓冲区（64字节 = 16个float）
            memcpy(buf->data(), m.constData(), 64);
            changed = true;
        }
        
        return changed;
    }

    void updateSampledImage(RenderState& state, 
                           int binding, 
                           QSGTexture** texture, 
                           QSGMaterial* newMaterial, 
                           QSGMaterial* oldMaterial) override
    {
        YuvMaterial* mat = static_cast<YuvMaterial*>(newMaterial);
        
        // 根据绑定点索引选择对应的纹理平面
        switch (binding) 
        {
            case 1: // Y平面（亮度）
                if (mat->textureYObject()) 
                {
                    // YuvDynamicTexture会自动处理纹理更新
                    mat->textureYObject()->commitTextureOperations(
                        state.rhi(), 
                        state.resourceUpdateBatch());
                }
                *texture = mat->textureYObject();
                break;
                
            case 2: // U平面（色度蓝）
                if (mat->textureUObject()) 
                {
                    mat->textureUObject()->commitTextureOperations(
                        state.rhi(), 
                        state.resourceUpdateBatch());
                }
                *texture = mat->textureUObject();
                break;
                
            case 3: // V平面（色度红）
                if (mat->textureVObject()) 
                {
                    mat->textureVObject()->commitTextureOperations(
                        state.rhi(), 
                        state.resourceUpdateBatch());
                }
                *texture = mat->textureVObject();
                break;
                
            default: // 未知绑定点
                *texture = nullptr;
                Logger::warning(QString("Unknown texture binding: %1").arg(binding));
                break;
        }
    }
};

// ========================================
// YuvMaterial - YUV材质实现
// ========================================

YuvMaterial::YuvMaterial() 
{
    // 默认构造
}

YuvMaterial::~YuvMaterial() 
{ 
    // 清理所有纹理资源
    clear(); 
}

QSGMaterialType* YuvMaterial::type() const
{
    // 返回静态类型标识符
    // Qt场景图使用此标识来区分不同的材质类型
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader* YuvMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    // 创建YUV着色器实例
    return new YuvMaterialShader();
}

void YuvMaterial::setYuvData(const uint8_t* DataY,
                            const uint8_t* DataU,
                            const uint8_t* DataV,
                            int StrideY, 
                            int StrideU, 
                            int StrideV,
                            int width, 
                            int height, 
                            QQuickWindow* window)
{
    // 验证输入参数
    if (!DataY || !DataU || !DataV || width <= 0 || height <= 0) {
        return;
    }

    // 调试输出：打印YUV数据的前几个字节（debug时开启）
    // qDebug("Y plane first 3 bytes: %d, %d, %d", DataY[0], DataY[1], DataY[2]);
    // qDebug("U plane first 3 bytes: %d, %d, %d", DataU[0], DataU[1], DataU[2]);
    // qDebug("V plane first 3 bytes: %d, %d, %d", DataV[0], DataV[1], DataV[2]);

    // 上传YUV数据到GPU纹理
    uploadTextures(DataY, DataU, DataV, 
                  StrideY, StrideU, StrideV, 
                  width, height, 
                  window);
    
    // 更新帧尺寸和状态
    m_size = QSize(width, height);
    m_hasTexture = true;
}

void YuvMaterial::clear()
{
    // 释放Y平面纹理
    if (m_textureY) 
    { 
        m_textureY->deleteLater(); 
        m_textureY = nullptr; 
    }
    
    // 释放U平面纹理
    if (m_textureU) 
    { 
        m_textureU->deleteLater(); 
        m_textureU = nullptr; 
    }
    
    // 释放V平面纹理
    if (m_textureV) 
    { 
        m_textureV->deleteLater(); 
        m_textureV = nullptr; 
    }
    
    // 清空QImage缓冲
    m_imageY = QImage();
    m_imageU = QImage();
    m_imageV = QImage();
    
    // 重置状态
    m_size = QSize();
    m_hasTexture = false;
}

void YuvMaterial::uploadTextures(const uint8_t* DataY,
                                const uint8_t* DataU,
                                const uint8_t* DataV,
                                int StrideY, 
                                int StrideU, 
                                int StrideV,
                                int width, 
                                int height, 
                                QQuickWindow* window)
{
    // 获取RHI接口
    QRhi* rhi = window->rhi();
    if (!rhi) {
        Logger::warning("RHI is not available");
        return;
    }
    
    // 检测尺寸变化，只有尺寸改变时才重建纹理
    bool needRecreate = !m_hasTexture || 
                        width != m_size.width() || 
                        height != m_size.height();

    if (needRecreate) {
        Logger::info(QString("Frame size changed or first frame, recreating dynamic textures: %1x%2")
                     .arg(width).arg(height));
        clear();  // 清理旧纹理
        
        // 创建动态纹理对象（只在尺寸变化时创建一次）
        const int uvWidth = width / 2;
        const int uvHeight = height / 2;
        
        m_textureY = new YuvDynamicTexture(QSize(width, height), rhi);
        m_textureU = new YuvDynamicTexture(QSize(uvWidth, uvHeight), rhi);
        m_textureV = new YuvDynamicTexture(QSize(uvWidth, uvHeight), rhi);
        
        if (!m_textureY->isValid() || !m_textureU->isValid() || !m_textureV->isValid()) {
            Logger::warning("Failed to create one or more dynamic textures");
            clear();
            return;
        }
    }

    // 更新纹理数据（不重建纹理对象，只更新数据）
    if (m_textureY && m_textureU && m_textureV) {
        // 更新Y平面数据（全分辨率）
        m_textureY->setTextureData(DataY, width, height, StrideY);
        
        // 更新U平面数据（宽高减半，YUV420格式）
        const int uvWidth = width / 2;
        const int uvHeight = height / 2;
        m_textureU->setTextureData(DataU, uvWidth, uvHeight, StrideU);
        
        // 更新V平面数据（宽高减半，YUV420格式）
        m_textureV->setTextureData(DataV, uvWidth, uvHeight, StrideV);
    }
}


