#include "YuvMaterial.h"
#include "utils/Logger.h"
#include <QSGMaterialShader>
#include <QOpenGLExtraFunctions>
#include <QSGTexture>
#include <QQuickWindow>
#include <QSize>
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

    /**
     * @brief 更新采样纹理绑定
     * 
     * Qt 6.9推荐的纹理绑定方式，将YUV三个平面纹理绑定到对应的采样器
     * 
     * @param state 渲染状态对象
     * @param binding 绑定点索引（1=Y, 2=U, 3=V）
     * @param texture 输出纹理指针
     * @param newMaterial 新材质对象
     * @param oldMaterial 旧材质对象（未使用）
     */
    void updateSampledImage(RenderState& state, 
                           int binding, 
                           QSGTexture** texture, 
                           QSGMaterial* newMaterial, 
                           QSGMaterial*) override
    {
        YuvMaterial* mat = static_cast<YuvMaterial*>(newMaterial);
        
        // 根据绑定点索引选择对应的纹理平面
        switch (binding) 
        {
            case 1: // Y平面（亮度）
                if (mat->textureYObject()) 
                {
                    // 提交纹理操作到RHI（渲染硬件接口）
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
                qDebug("Unknown texture binding: %d", binding);
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
    /**
     * @brief Lambda辅助函数：创建并配置单个纹理平面
     * 
     * @param texture 纹理对象引用
     * @param data 平面数据指针
     * @param w 平面宽度
     * @param h 平面高度
     * @param stride 行步长（字节数）
     * @param planeName 平面名称（用于日志）
     */
    auto createTexture = [&](QSGTexture*& texture, 
                            const uint8_t* data, 
                            int w, int h, 
                            int stride, 
                            const char* planeName) 
    {
        // 释放旧纹理
        if (texture) 
        {
            texture->deleteLater();
            texture = nullptr;
        }
        
        // 创建灰度图像（8位单通道）
        QImage image(w, h, QImage::Format_Grayscale8);
        
        // 逐行复制数据（处理stride不等于width的情况）
        for (int row = 0; row < h; ++row) 
        {
            memcpy(image.scanLine(row), data + row * stride, w);
        }
        
        // 从图像创建GPU纹理
        texture = window->createTextureFromImage(image);
        if (!texture) 
        {
            qWarning("Failed to create %s texture", planeName);
            return;
        }
        
        // 配置纹理采样参数
        texture->setFiltering(QSGTexture::Linear);              // 线性插值
        texture->setHorizontalWrapMode(QSGTexture::ClampToEdge); // 水平边缘夹紧
        texture->setVerticalWrapMode(QSGTexture::ClampToEdge);   // 垂直边缘夹紧
    };

    // 创建Y平面纹理（全分辨率）
    createTexture(m_textureY, DataY, width, height, StrideY, "Y");
    
    // 创建U平面纹理（宽高减半，YUV420格式）
    const int uvWidth = width / 2;
    const int uvHeight = height / 2;
    createTexture(m_textureU, DataU, uvWidth, uvHeight, StrideU, "U");
    
    // 创建V平面纹理（宽高减半，YUV420格式）
    createTexture(m_textureV, DataV, uvWidth, uvHeight, StrideV, "V");

    // 调试输出：检查纹理尺寸（debug时开启）
    // if (m_textureY) 
    // {
    //     qDebug("m_textureY size: %dx%d", 
    //            m_textureY->textureSize().width(), 
    //            m_textureY->textureSize().height());
    // }
    // if (m_textureU) 
    // {
    //     qDebug("m_textureU size: %dx%d", 
    //            m_textureU->textureSize().width(), 
    //            m_textureU->textureSize().height());
    // }
    // if (m_textureV) 
    // {
    //     qDebug("m_textureV size: %dx%d", 
    //            m_textureV->textureSize().width(), 
    //            m_textureV->textureSize().height());
    // }
}
