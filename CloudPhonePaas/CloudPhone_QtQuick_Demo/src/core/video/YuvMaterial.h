#pragma once

#include <QSGMaterial>
#include <QSize>
#include <QSGTexture>
#include <QQuickWindow>

class YuvMaterialShader;

// YUV材质类，管理YUV纹理及其上传
class YuvMaterial : public QSGMaterial
{
public:
    YuvMaterial();
    ~YuvMaterial() override;

    // 返回材质类型
    QSGMaterialType* type() const override;
    // 创建对应的shader
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;

    // 设置YUV数据并上传为纹理
    void setYuvData(
        const uint8_t* DataY,
        const uint8_t* DataU,
        const uint8_t* DataV,
        int StrideY, 
        int StrideU, 
        int StrideV,
        int width, int height, QQuickWindow* window);

    // 清除纹理和尺寸信息
    void clear();

    // 获取当前帧尺寸
    QSize frameSize() const { return m_size; }

    // 获取Y/U/V分量的纹理对象
    QSGTexture* textureYObject() const { return m_textureY; }
    QSGTexture* textureUObject() const { return m_textureU; }
    QSGTexture* textureVObject() const { return m_textureV; }

private:
    // 上传YUV数据到GPU纹理
    void uploadTextures(    
        const uint8_t* DataY,
        const uint8_t* DataU,
        const uint8_t* DataV,
        int StrideY, 
        int StrideU, 
        int StrideV,
        int width, int height, QQuickWindow* window);

    QSize m_size;             // 当前帧尺寸
    bool m_hasTexture = false;// 是否已有纹理

    QSGTexture* m_textureY = nullptr; // Y分量纹理
    QSGTexture* m_textureU = nullptr; // U分量纹理
    QSGTexture* m_textureV = nullptr; // V分量纹理
};