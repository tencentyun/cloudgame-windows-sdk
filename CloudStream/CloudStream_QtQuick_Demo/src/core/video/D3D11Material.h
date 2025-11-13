#pragma once

#include <QSGMaterial>
#include <QSGTexture>
#include <QQuickWindow>
#include <QSGRendererInterface>
#include "Frame.h"

class QSGMaterialShader;
class QRhi;
class QRhiTexture;

// D3D11类型前向声明
struct ID3D11Device;
struct ID3D11Texture2D;

/**
 * @brief D3D11纹理材质类
 * 
 * 负责管理D3D11纹理与Qt场景图的集成。
 * 处理D3D11纹理到OpenGL纹理的转换和同步。
 */
class D3D11Material : public QSGMaterial
{
public:
    /**
     * @brief 构造函数
     */
    D3D11Material();
    
    /**
     * @brief 析构函数
     */
    ~D3D11Material() override;

    /**
     * @brief 创建材质着色器
     * 
     * Qt场景图框架调用此方法创建对应的着色器程序
     * 
     * @return 材质着色器指针
     */
    QSGMaterialType* type() const override;
    
    /**
     * @brief 创建着色器实例
     * 
     * @param renderMode 渲染模式
     * @return 着色器对象指针
     */
    QSGMaterialShader* createShader(QSGRendererInterface::RenderMode renderMode) const override;

    /**
     * @brief 设置D3D11纹理数据
     * 
     * 将D3D11纹理转换为Qt可用的纹理对象
     * 
     * @param d3d11Data D3D11纹理数据
     * @param width 纹理宽度
     * @param height 纹理高度
     * @param window 渲染窗口指针
     */
    void setD3D11Texture(const D3D11TextureData& d3d11Data,
                        int width,
                        int height,
                        QQuickWindow* window);
    
    /**
     * @brief 清除纹理数据
     */
    void clear();
    
    /**
     * @brief 获取Qt纹理对象
     * 
     * @return Qt纹理指针，可能为nullptr
     */
    QSGTexture* texture() const { return m_texture; }

private:
    /**
     * @brief 创建或更新Qt纹理对象
     * 
     * 从D3D11纹理创建Qt可用的纹理
     * 
     * @param d3d11Data D3D11纹理数据
     * @param width 纹理宽度
     * @param height 纹理高度
     * @param window 渲染窗口指针
     * @return 是否成功创建/更新纹理
     */
    bool createTextureFromD3D11(const D3D11TextureData& d3d11Data,
                               int width,
                               int height,
                               QQuickWindow* window);
    
    /**
     * @brief 创建Compute Shader
     * 
     * 加载并编译NV12到RGBA转换的Compute Shader
     * 
     * @param device D3D11设备指针
     * @return 是否成功创建Compute Shader
     */
    bool createComputeShader(ID3D11Device* device);
    
    /**
     * @brief 创建RGBA目标纹理
     * 
     * 创建用于存储转换结果的RGBA纹理
     * 
     * @param device D3D11设备指针
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 是否成功创建纹理
     */
    bool createRGBATexture(ID3D11Device* device, int width, int height);
    
    /**
     * @brief 执行NV12到RGBA的转换
     * 
     * 使用Compute Shader将NV12格式纹理转换为RGBA格式
     * 
     * @param device D3D11设备指针
     * @param nv12Texture NV12格式的源纹理
     * @param arrayIndex 纹理数组索引
     * @param width 纹理宽度
     * @param height 纹理高度
     * @return 是否成功执行转换
     */
    bool convertNV12ToRGBA(ID3D11Device* device,
                          ID3D11Texture2D* nv12Texture,
                          unsigned int arrayIndex,
                          int width,
                          int height);
    
    /**
     * @brief 清理Compute Shader相关资源
     */
    void clearComputeResources();

private:
    // 基础纹理资源
    QSGTexture* m_texture = nullptr;  ///< Qt纹理对象
    int m_textureWidth = 0;           ///< 纹理宽度
    int m_textureHeight = 0;          ///< 纹理高度
    
    // NV12转换相关资源
    void* m_computeShader = nullptr;           ///< Compute Shader对象 (ID3D11ComputeShader*)
    void* m_rgbaTexture = nullptr;             ///< RGBA目标纹理 (ID3D11Texture2D*)
    void* m_rgbaUAV = nullptr;                 ///< RGBA纹理的UAV (ID3D11UnorderedAccessView*)
    void* m_nv12SRV_Y = nullptr;               ///< NV12 Y平面的SRV (ID3D11ShaderResourceView*)
    void* m_nv12SRV_UV = nullptr;              ///< NV12 UV平面的SRV (ID3D11ShaderResourceView*)
    void* m_samplerState = nullptr;            ///< 采样器状态 (ID3D11SamplerState*)
    void* m_constantBuffer = nullptr;          ///< 常量缓冲区 (ID3D11Buffer*)
};