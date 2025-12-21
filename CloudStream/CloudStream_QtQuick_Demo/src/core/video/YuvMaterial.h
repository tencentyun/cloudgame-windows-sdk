#pragma once

#include <QSGMaterial>
#include <QSize>
#include <QSGTexture>
#include <QQuickWindow>

class YuvMaterialShader;
class YuvDynamicTexture;

/**
 * @brief YUV材质类
 * 
 * 管理YUV格式视频的三个纹理平面（Y、U、V），
 * 负责将CPU端的YUV数据上传到GPU纹理，并提供给着色器使用。
 */
class YuvMaterial : public QSGMaterial
{
public:
    /**
     * @brief 构造函数
     */
    YuvMaterial();
    
    /**
     * @brief 析构函数
     * 
     * 清理所有纹理资源
     */
    ~YuvMaterial() override;

    /**
     * @brief 返回材质类型标识
     * 
     * Qt场景图使用此标识来区分不同的材质类型
     * 
     * @return 材质类型指针
     */
    QSGMaterialType* type() const override;
    
    /**
     * @brief 创建对应的着色器对象
     * 
     * @param renderMode 渲染模式（未使用）
     * @return 着色器对象指针
     */
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode) const override;

    /**
     * @brief 设置YUV数据并上传为GPU纹理
     * 
     * 将CPU端的YUV平面数据转换为QImage，然后创建GPU纹理
     * 
     * @param DataY Y分量数据指针（亮度平面）
     * @param DataU U分量数据指针（色度蓝平面）
     * @param DataV V分量数据指针（色度红平面）
     * @param StrideY Y平面的行步长（字节数）
     * @param StrideU U平面的行步长（字节数）
     * @param StrideV V平面的行步长（字节数）
     * @param width 视频帧宽度（Y平面）
     * @param height 视频帧高度（Y平面）
     * @param window 渲染窗口指针，用于创建纹理
     */
    void setYuvData(const uint8_t* DataY,
                    const uint8_t* DataU,
                    const uint8_t* DataV,
                    int StrideY, 
                    int StrideU, 
                    int StrideV,
                    int width, 
                    int height, 
                    QQuickWindow* window);

    /**
     * @brief 清除所有纹理和尺寸信息
     * 
     * 释放GPU纹理资源，重置状态
     */
    void clear();

    // ========== 访问器方法 ==========
    
    /**
     * @brief 获取当前视频帧尺寸
     * 
     * @return 帧尺寸（宽度和高度）
     */
    QSize frameSize() const { return m_size; }

    /**
     * @brief 获取Y分量纹理对象
     * 
     * @return Y纹理指针
     */
    YuvDynamicTexture* textureYObject() const { return m_textureY; }
    
    /**
     * @brief 获取U分量纹理对象
     * 
     * @return U纹理指针
     */
    YuvDynamicTexture* textureUObject() const { return m_textureU; }
    
    /**
     * @brief 获取V分量纹理对象
     * 
     * @return V纹理指针
     */
    YuvDynamicTexture* textureVObject() const { return m_textureV; }

private:
    /**
     * @brief 上传YUV数据到GPU纹理
     * 
     * 内部实现函数，将YUV平面数据转换为灰度图像并创建纹理
     * 
     * @param DataY Y分量数据指针
     * @param DataU U分量数据指针
     * @param DataV V分量数据指针
     * @param StrideY Y平面行步长
     * @param StrideU U平面行步长
     * @param StrideV V平面行步长
     * @param width 视频帧宽度
     * @param height 视频帧高度
     * @param window 渲染窗口指针
     */
    void uploadTextures(const uint8_t* DataY,
                       const uint8_t* DataU,
                       const uint8_t* DataV,
                       int StrideY, 
                       int StrideU, 
                       int StrideV,
                       int width, 
                       int height, 
                       QQuickWindow* window);

private:
    // 状态信息
    QSize m_size;              ///< 当前视频帧尺寸
    bool m_hasTexture = false; ///< 标识是否已创建纹理
    bool m_texturesDirty = false; ///< 标识纹理是否需要提交更新操作

    // GPU纹理对象（使用动态纹理实现复用）
    YuvDynamicTexture* m_textureY = nullptr; ///< Y分量纹理（亮度）
    YuvDynamicTexture* m_textureU = nullptr; ///< U分量纹理（色度蓝）
    YuvDynamicTexture* m_textureV = nullptr; ///< V分量纹理（色度红）
    
    // 复用的QImage对象（避免每帧重新分配）
    QImage m_imageY; ///< Y平面图像缓冲
    QImage m_imageU; ///< U平面图像缓冲
    QImage m_imageV; ///< V平面图像缓冲
};