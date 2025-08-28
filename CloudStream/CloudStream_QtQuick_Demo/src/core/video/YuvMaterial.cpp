#include "YuvMaterial.h"
#include "utils/Logger.h"
#include <QSGMaterialShader>
#include <QOpenGLExtraFunctions>
#include <QSGTexture>
#include <QQuickWindow>
#include <QSize>
#include <cstring>

// ================= YuvMaterialShader =================

class YuvMaterialShader : public QSGMaterialShader
{
public:
    YuvMaterialShader()
    {
        setShaderFileName(VertexStage, QLatin1String(":/shaders/shaders/yuv.vert.qsb"));
        setShaderFileName(FragmentStage, QLatin1String(":/shaders/shaders/yuv.frag.qsb"));
    }

    // 如果你的 shader 有 uniform block（比如 MVP 矩阵），需要实现
    bool updateUniformData(RenderState &state, QSGMaterial *newMaterial, QSGMaterial *oldMaterial) override
    {
        bool changed = false;
        QByteArray *buf = state.uniformData();
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data(), m.constData(), 64);
            changed = true;
        }
        return changed;
    }

    // Qt 6.9 推荐的采样纹理方式
    void updateSampledImage(RenderState &state, int binding, QSGTexture **texture, QSGMaterial *newMaterial, QSGMaterial *) override
    {
        YuvMaterial *mat = static_cast<YuvMaterial *>(newMaterial);
        switch (binding) {
            case 1: 
                if (mat->textureYObject()) {
                    mat->textureYObject()->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                }
                *texture = mat->textureYObject(); 
                break;
            case 2: 
                if (mat->textureUObject()) {
                    mat->textureUObject()->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                }
                *texture = mat->textureUObject(); 
                break;
            case 3: 
                if (mat->textureVObject()) {
                    mat->textureVObject()->commitTextureOperations(state.rhi(), state.resourceUpdateBatch());
                }
                *texture = mat->textureVObject(); 
                break;
            default: 
                *texture = nullptr; 
                qDebug("Bind unknown: %d", binding); 
                break;
        }
    }
};

// ================= YuvMaterial =================

YuvMaterial::YuvMaterial() {}
YuvMaterial::~YuvMaterial() { clear(); }

QSGMaterialType* YuvMaterial::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader* YuvMaterial::createShader(QSGRendererInterface::RenderMode) const
{
    return new YuvMaterialShader();
}

void YuvMaterial::setYuvData(
    const uint8_t* DataY,
    const uint8_t* DataU,
    const uint8_t* DataV,
    int StrideY, 
    int StrideU, 
    int StrideV,
    int width, int height, QQuickWindow* window)
{
    if (!DataY || !DataU || !DataV || width <= 0 || height <= 0)
        return;

    // qDebug("Y plane first 3 bytes: %d, %d, %d", DataY[0], DataY[1], DataY[2]);
    // qDebug("U plane first 3 bytes: %d, %d, %d", DataU[0], DataU[1], DataU[2]);
    // qDebug("V plane first 3 bytes: %d, %d, %d", DataV[0], DataV[1], DataV[2]);

    uploadTextures(DataY, DataU, DataV, StrideY, StrideU, StrideV, width, height, window);
    m_size = QSize(width, height);
    m_hasTexture = true;
}

void YuvMaterial::clear()
{
    if (m_textureY) { m_textureY->deleteLater(); m_textureY = nullptr; }
    if (m_textureU) { m_textureU->deleteLater(); m_textureU = nullptr; }
    if (m_textureV) { m_textureV->deleteLater(); m_textureV = nullptr; }
    m_size = QSize();
    m_hasTexture = false;
}

void YuvMaterial::uploadTextures(
    const uint8_t* DataY,
    const uint8_t* DataU,
    const uint8_t* DataV,
    int StrideY, 
    int StrideU, 
    int StrideV,
    int width, int height, QQuickWindow* window)
{
    // 辅助函数：创建并配置纹理
    auto createTexture = [&](QSGTexture*& texture, const uint8_t* data, int w, int h, int stride, const char* planeName) {
        if (texture) {
            texture->deleteLater();
            texture = nullptr;
        }
        QImage image(w, h, QImage::Format_Grayscale8);
        for (int row = 0; row < h; ++row) {
            memcpy(image.scanLine(row), data + row * stride, w);
        }
        texture = window->createTextureFromImage(image);
        if (!texture) {
            qWarning("Failed to create %s texture", planeName);
            return;
        }
        texture->setFiltering(QSGTexture::Linear);
        texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
        texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
    };

    // 创建Y平面纹理
    createTexture(m_textureY, DataY, width, height, StrideY, "Y");
    
    // 创建U平面纹理（宽高减半）
    const int uvWidth = width / 2;
    const int uvHeight = height / 2;
    createTexture(m_textureU, DataU, uvWidth, uvHeight, StrideU, "U");
    
    // 创建V平面纹理（宽高减半）
    createTexture(m_textureV, DataV, uvWidth, uvHeight, StrideV, "V");

    // 检查纹理尺寸
    // if (m_textureY) qDebug("m_textureY size: %dx%d", m_textureY->textureSize().width(), m_textureY->textureSize().height());
    // if (m_textureU) qDebug("m_textureU size: %dx%d", m_textureU->textureSize().width(), m_textureU->textureSize().height());
    // if (m_textureV) qDebug("m_textureV size: %dx%d", m_textureV->textureSize().width(), m_textureV->textureSize().height());
}
