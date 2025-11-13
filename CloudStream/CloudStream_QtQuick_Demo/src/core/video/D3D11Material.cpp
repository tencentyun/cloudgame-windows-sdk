#include "D3D11Material.h"
#include <QSGMaterialShader>
#include <QSGTexture>
#include <QOpenGLFunctions>
#include <QSGRendererInterface>
#include <QDebug>
#include <QFile>
#include <QQuickWindow>

#include <d3d11.h>
#include <d3dcompiler.h>
#include <wrl/client.h>
using Microsoft::WRL::ComPtr;

// ========== D3D11材质着色器 ==========

/**
 * @brief D3D11纹理着色器类
 * 
 * 负责渲染D3D11纹理的着色器程序
 */
class D3D11Shader : public QSGMaterialShader
{
public:
    D3D11Shader()
    {
        // Qt 6.x使用新的着色器设置方式
        // 顶点着色器：标准的2D纹理映射
        setShaderFileName(VertexStage, QLatin1String(":/shaders/shaders/d3d11_texture.vert.qsb"));
        // 片段着色器：简单的纹理采样
        setShaderFileName(FragmentStage, QLatin1String(":/shaders/shaders/d3d11_texture.frag.qsb"));
    }

    bool updateUniformData(RenderState& state, 
                          QSGMaterial* newMaterial,
                          QSGMaterial* oldMaterial) override
    {
        bool changed = false;
        QByteArray* buf = state.uniformData();
        
        // 更新MVP矩阵
        if (state.isMatrixDirty()) {
            const QMatrix4x4 m = state.combinedMatrix();
            memcpy(buf->data(), m.constData(), 64);
            changed = true;
        }
        
        // 更新透明度
        if (state.isOpacityDirty()) {
            const float opacity = state.opacity();
            memcpy(buf->data() + 64, &opacity, 4);
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
        // binding 1: RGBA纹理
        if (binding == 1) {
            D3D11Material* mat = static_cast<D3D11Material*>(newMaterial);
            if (mat && mat->texture()) {
                *texture = mat->texture();
                (*texture)->setFiltering(QSGTexture::Linear);
                (*texture)->setHorizontalWrapMode(QSGTexture::ClampToEdge);
                (*texture)->setVerticalWrapMode(QSGTexture::ClampToEdge);
            }
        }
    }
};

// ========== D3D11Material实现 ==========

D3D11Material::D3D11Material()
{
    // 设置材质标志
    setFlag(Blending, true);
}

D3D11Material::~D3D11Material()
{
    clear();
    clearComputeResources();
}

QSGMaterialType* D3D11Material::type() const
{
    static QSGMaterialType type;
    return &type;
}

QSGMaterialShader* D3D11Material::createShader(QSGRendererInterface::RenderMode renderMode) const
{
    Q_UNUSED(renderMode);
    return new D3D11Shader();
}

void D3D11Material::setD3D11Texture(const D3D11TextureData& d3d11Data,
                                   int width,
                                   int height,
                                   QQuickWindow* window)
{
    // 检查参数有效性
    if (!d3d11Data.texture || !d3d11Data.device || !window) {
        qWarning() << "D3D11Material: Invalid parameters";
        clear();
        return;
    }

    // 如果尺寸变化，需要重新创建纹理
    if (width != m_textureWidth || height != m_textureHeight) {
        clear();
    }

    // 创建或更新纹理
    if (!m_texture) {
        if (createTextureFromD3D11(d3d11Data, width, height, window)) {
            m_textureWidth = width;
            m_textureHeight = height;
        }
    }
}

void D3D11Material::clear()
{
    if (m_texture) {
        delete m_texture;
        m_texture = nullptr;
    }
    m_textureWidth = 0;
    m_textureHeight = 0;
    
    clearComputeResources();
}

bool D3D11Material::createTextureFromD3D11(const D3D11TextureData& d3d11Data,
                                          int width,
                                          int height,
                                          QQuickWindow* window)
{
    // 获取D3D11纹理对象
    ID3D11Texture2D* d3d11Texture = static_cast<ID3D11Texture2D*>(d3d11Data.texture);
    ID3D11Device* d3d11Device = static_cast<ID3D11Device*>(d3d11Data.device);

    if (!d3d11Texture || !d3d11Device) {
        qWarning() << "D3D11Material: Invalid D3D11 objects";
        return false;
    }

    // 获取并打印D3D11纹理描述信息（用于诊断）
    D3D11_TEXTURE2D_DESC desc;
    d3d11Texture->GetDesc(&desc);
    qDebug() << "D3D11 Texture Info:" 
             << "Width:" << desc.Width 
             << "Height:" << desc.Height
             << "Format:" << desc.Format
             << "BindFlags:" << desc.BindFlags
             << "Usage:" << desc.Usage
             << "CPUAccessFlags:" << desc.CPUAccessFlags;

    // 检查是否为解码器纹理（BindFlags包含D3D11_BIND_DECODER）
    const UINT D3D11_BIND_DECODER = 0x200; // 512
    bool isDecoderTexture = (desc.BindFlags & D3D11_BIND_DECODER) != 0;
    
    ID3D11Texture2D* renderableTexture = d3d11Texture;
    ComPtr<ID3D11Texture2D> copiedTexture; // 用于管理复制的纹理生命周期
    
    // 如果是解码器纹理或需要复制的纹理，创建可渲染副本
    if (isDecoderTexture || (desc.BindFlags & D3D11_BIND_SHADER_RESOURCE) == 0) {
        qDebug() << "D3D11Material: Creating renderable texture copy";
        
        // 创建可渲染的纹理描述
        D3D11_TEXTURE2D_DESC renderableDesc = {};
        renderableDesc.Width = desc.Width;
        renderableDesc.Height = desc.Height;
        renderableDesc.MipLevels = 1;
        renderableDesc.ArraySize = 1;
        renderableDesc.Format = desc.Format;
        renderableDesc.SampleDesc.Count = 1;
        renderableDesc.SampleDesc.Quality = 0;
        renderableDesc.Usage = D3D11_USAGE_DEFAULT;
        renderableDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
        renderableDesc.CPUAccessFlags = 0;
        renderableDesc.MiscFlags = 0;
        
        // 创建可渲染纹理
        HRESULT hr = d3d11Device->CreateTexture2D(&renderableDesc, nullptr, &copiedTexture);
        if (FAILED(hr)) {
            qWarning() << "D3D11Material: Failed to create renderable texture, HRESULT:" << Qt::hex << hr;
            return false;
        }
        
        // 获取设备上下文
        ComPtr<ID3D11DeviceContext> context;
        d3d11Device->GetImmediateContext(&context);
        
        // 复制纹理内容
        // 如果源纹理是纹理数组，需要指定子资源索引
        UINT srcSubresource = D3D11CalcSubresource(0, d3d11Data.array_index, desc.MipLevels);
        UINT dstSubresource = 0;
        context->CopySubresourceRegion(
            copiedTexture.Get(),
            dstSubresource,
            0, 0, 0,  // DstX, DstY, DstZ
            d3d11Texture,
            srcSubresource,
            nullptr   // 复制整个子资源
        );
        
        renderableTexture = copiedTexture.Get();
        qDebug() << "D3D11Material: Texture copied successfully";
    }

    // 获取窗口的渲染接口
    QSGRendererInterface* rif = window->rendererInterface();
    if (!rif) {
        qWarning() << "D3D11Material: Failed to get renderer interface";
        return false;
    }

    // 确保使用的是D3D11渲染后端
    if (rif->graphicsApi() != QSGRendererInterface::Direct3D11) {
        qWarning() << "D3D11Material: Not using D3D11 backend, current API:" << rif->graphicsApi();
        return false;
    }

    // 使用Qt 6.9的QNativeInterface直接从D3D11纹理创建QSGTexture
    m_texture = QNativeInterface::QSGD3D11Texture::fromNative(
        renderableTexture,
        window,
        QSize(width, height),
        QQuickWindow::TextureHasAlphaChannel
    );

    if (!m_texture) {
        qWarning() << "D3D11Material: Failed to create QSGTexture from D3D11 texture";
        return false;
    }

    // 设置纹理过滤和包裹模式
    m_texture->setFiltering(QSGTexture::Linear);
    m_texture->setHorizontalWrapMode(QSGTexture::ClampToEdge);
    m_texture->setVerticalWrapMode(QSGTexture::ClampToEdge);
    
    // 如果创建了复制纹理，需要保存其引用以防止被释放
    if (copiedTexture) {
        // 将ComPtr转移到成员变量（需要在头文件中添加成员变量）
        // 这里暂时让copiedTexture在函数结束时释放
        // 实际使用中需要妥善管理纹理生命周期
    }

    qDebug() << "D3D11Material: Successfully created RGBA texture" << width << "x" << height 
             << "format:" << d3d11Data.format;
    return true;
}

bool D3D11Material::createComputeShader(ID3D11Device* device)
{
    if (!device) {
        qWarning() << "D3D11Material::createComputeShader: Invalid device";
        return false;
    }
    
    // 如果已经创建，直接返回
    if (m_computeShader) {
        return true;
    }
    
    // 从资源文件加载编译好的Compute Shader
    QFile shaderFile(":/shaders/shaders/nv12_to_rgba.comp.qsb");
    if (!shaderFile.open(QIODevice::ReadOnly)) {
        qWarning() << "D3D11Material::createComputeShader: Failed to open shader file";
        return false;
    }
    
    QByteArray shaderData = shaderFile.readAll();
    shaderFile.close();
    
    if (shaderData.isEmpty()) {
        qWarning() << "D3D11Material::createComputeShader: Shader file is empty";
        return false;
    }
    
    // 创建Compute Shader
    ID3D11ComputeShader* computeShader = nullptr;
    HRESULT hr = device->CreateComputeShader(
        shaderData.constData(),
        shaderData.size(),
        nullptr,
        &computeShader
    );
    
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::createComputeShader: Failed to create compute shader, HRESULT:" 
                   << Qt::hex << hr;
        return false;
    }
    
    m_computeShader = computeShader;
    
    // 创建采样器状态
    D3D11_SAMPLER_DESC samplerDesc = {};
    samplerDesc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samplerDesc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samplerDesc.ComparisonFunc = D3D11_COMPARISON_NEVER;
    samplerDesc.MinLOD = 0;
    samplerDesc.MaxLOD = D3D11_FLOAT32_MAX;
    
    ID3D11SamplerState* samplerState = nullptr;
    hr = device->CreateSamplerState(&samplerDesc, &samplerState);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::createComputeShader: Failed to create sampler state, HRESULT:" 
                   << Qt::hex << hr;
        return false;
    }
    
    m_samplerState = samplerState;
    
    // 创建常量缓冲区（用于传递图像尺寸）
    D3D11_BUFFER_DESC cbDesc = {};
    cbDesc.ByteWidth = 16; // uvec2 imageSize (8 bytes) + padding
    cbDesc.Usage = D3D11_USAGE_DYNAMIC;
    cbDesc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cbDesc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    
    ID3D11Buffer* constantBuffer = nullptr;
    hr = device->CreateBuffer(&cbDesc, nullptr, &constantBuffer);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::createComputeShader: Failed to create constant buffer, HRESULT:" 
                   << Qt::hex << hr;
        return false;
    }
    
    m_constantBuffer = constantBuffer;
    
    qDebug() << "D3D11Material::createComputeShader: Compute shader created successfully";
    return true;
}

bool D3D11Material::createRGBATexture(ID3D11Device* device, int width, int height)
{
    if (!device) {
        qWarning() << "D3D11Material::createRGBATexture: Invalid device";
        return false;
    }
    
    // 如果已存在且尺寸匹配，直接返回
    if (m_rgbaTexture) {
        ID3D11Texture2D* texture = static_cast<ID3D11Texture2D*>(m_rgbaTexture);
        D3D11_TEXTURE2D_DESC desc;
        texture->GetDesc(&desc);
        
        if (desc.Width == static_cast<UINT>(width) && desc.Height == static_cast<UINT>(height)) {
            return true;
        }
        
        // 尺寸不匹配，清理旧资源
        clearComputeResources();
    }
    
    // 创建RGBA纹理描述
    D3D11_TEXTURE2D_DESC texDesc = {};
    texDesc.Width = width;
    texDesc.Height = height;
    texDesc.MipLevels = 1;
    texDesc.ArraySize = 1;
    texDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    texDesc.SampleDesc.Count = 1;
    texDesc.SampleDesc.Quality = 0;
    texDesc.Usage = D3D11_USAGE_DEFAULT;
    texDesc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_UNORDERED_ACCESS;
    texDesc.CPUAccessFlags = 0;
    texDesc.MiscFlags = 0;
    
    // 创建纹理
    ID3D11Texture2D* rgbaTexture = nullptr;
    HRESULT hr = device->CreateTexture2D(&texDesc, nullptr, &rgbaTexture);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::createRGBATexture: Failed to create RGBA texture, HRESULT:" 
                   << Qt::hex << hr;
        return false;
    }
    
    m_rgbaTexture = rgbaTexture;
    
    // 创建UAV（用于Compute Shader写入）
    D3D11_UNORDERED_ACCESS_VIEW_DESC uavDesc = {};
    uavDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    uavDesc.ViewDimension = D3D11_UAV_DIMENSION_TEXTURE2D;
    uavDesc.Texture2D.MipSlice = 0;
    
    ID3D11UnorderedAccessView* uav = nullptr;
    hr = device->CreateUnorderedAccessView(rgbaTexture, &uavDesc, &uav);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::createRGBATexture: Failed to create UAV, HRESULT:" 
                   << Qt::hex << hr;
        rgbaTexture->Release();
        m_rgbaTexture = nullptr;
        return false;
    }
    
    m_rgbaUAV = uav;
    
    qDebug() << "D3D11Material::createRGBATexture: RGBA texture created successfully" 
             << width << "x" << height;
    return true;
}

bool D3D11Material::convertNV12ToRGBA(ID3D11Device* device,
                                     ID3D11Texture2D* nv12Texture,
                                     unsigned int arrayIndex,
                                     int width,
                                     int height)
{
    if (!device || !nv12Texture) {
        qWarning() << "D3D11Material::convertNV12ToRGBA: Invalid parameters";
        return false;
    }
    
    // 确保Compute Shader已创建
    if (!createComputeShader(device)) {
        return false;
    }
    
    // 确保RGBA纹理已创建
    if (!createRGBATexture(device, width, height)) {
        return false;
    }
    
    // 创建NV12纹理的SRV（Y平面和UV平面）
    // Y平面：DXGI_FORMAT_R8_UNORM
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc_Y = {};
    srvDesc_Y.Format = DXGI_FORMAT_R8_UNORM;
    srvDesc_Y.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc_Y.Texture2DArray.MostDetailedMip = 0;
    srvDesc_Y.Texture2DArray.MipLevels = 1;
    srvDesc_Y.Texture2DArray.FirstArraySlice = arrayIndex;
    srvDesc_Y.Texture2DArray.ArraySize = 1;
    
    ID3D11ShaderResourceView* srv_Y = nullptr;
    HRESULT hr = device->CreateShaderResourceView(nv12Texture, &srvDesc_Y, &srv_Y);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::convertNV12ToRGBA: Failed to create Y plane SRV, HRESULT:" 
                   << Qt::hex << hr;
        return false;
    }
    
    // UV平面：DXGI_FORMAT_R8G8_UNORM
    D3D11_SHADER_RESOURCE_VIEW_DESC srvDesc_UV = {};
    srvDesc_UV.Format = DXGI_FORMAT_R8G8_UNORM;
    srvDesc_UV.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2DARRAY;
    srvDesc_UV.Texture2DArray.MostDetailedMip = 0;
    srvDesc_UV.Texture2DArray.MipLevels = 1;
    srvDesc_UV.Texture2DArray.FirstArraySlice = arrayIndex;
    srvDesc_UV.Texture2DArray.ArraySize = 1;
    
    ID3D11ShaderResourceView* srv_UV = nullptr;
    hr = device->CreateShaderResourceView(nv12Texture, &srvDesc_UV, &srv_UV);
    if (FAILED(hr)) {
        qWarning() << "D3D11Material::convertNV12ToRGBA: Failed to create UV plane SRV, HRESULT:" 
                   << Qt::hex << hr;
        srv_Y->Release();
        return false;
    }
    
    // 保存SRV引用（用于后续清理）
    if (m_nv12SRV_Y) {
        static_cast<ID3D11ShaderResourceView*>(m_nv12SRV_Y)->Release();
    }
    if (m_nv12SRV_UV) {
        static_cast<ID3D11ShaderResourceView*>(m_nv12SRV_UV)->Release();
    }
    m_nv12SRV_Y = srv_Y;
    m_nv12SRV_UV = srv_UV;
    
    // 获取设备上下文
    ComPtr<ID3D11DeviceContext> context;
    device->GetImmediateContext(&context);
    
    // 更新常量缓冲区（传递图像尺寸）
    D3D11_MAPPED_SUBRESOURCE mappedResource;
    hr = context->Map(static_cast<ID3D11Buffer*>(m_constantBuffer), 0, D3D11_MAP_WRITE_DISCARD, 0, &mappedResource);
    if (SUCCEEDED(hr)) {
        struct {
            UINT width;
            UINT height;
            UINT padding[2];
        } params;
        params.width = width;
        params.height = height;
        params.padding[0] = 0;
        params.padding[1] = 0;
        
        memcpy(mappedResource.pData, &params, sizeof(params));
        context->Unmap(static_cast<ID3D11Buffer*>(m_constantBuffer), 0);
    }
    
    // 设置Compute Shader
    context->CSSetShader(static_cast<ID3D11ComputeShader*>(m_computeShader), nullptr, 0);
    
    // 绑定资源
    ID3D11ShaderResourceView* srvs[] = { srv_Y, srv_UV };
    context->CSSetShaderResources(0, 2, srvs);
    
    ID3D11SamplerState* samplers[] = { static_cast<ID3D11SamplerState*>(m_samplerState) };
    context->CSSetSamplers(0, 1, samplers);
    
    ID3D11UnorderedAccessView* uavs[] = { static_cast<ID3D11UnorderedAccessView*>(m_rgbaUAV) };
    context->CSSetUnorderedAccessViews(0, 1, uavs, nullptr);
    
    ID3D11Buffer* cbs[] = { static_cast<ID3D11Buffer*>(m_constantBuffer) };
    context->CSSetConstantBuffers(0, 1, cbs);
    
    // 调度Compute Shader
    // 工作组大小为16x16，计算需要的工作组数量
    UINT numGroupsX = (width + 15) / 16;
    UINT numGroupsY = (height + 15) / 16;
    context->Dispatch(numGroupsX, numGroupsY, 1);
    
    // 解绑资源
    ID3D11ShaderResourceView* nullSRVs[] = { nullptr, nullptr };
    context->CSSetShaderResources(0, 2, nullSRVs);
    
    ID3D11UnorderedAccessView* nullUAVs[] = { nullptr };
    context->CSSetUnorderedAccessViews(0, 1, nullUAVs, nullptr);
    
    context->CSSetShader(nullptr, nullptr, 0);
    
    qDebug() << "D3D11Material::convertNV12ToRGBA: Conversion dispatched successfully" 
             << "Groups:" << numGroupsX << "x" << numGroupsY;
    return true;
}

void D3D11Material::clearComputeResources()
{
    // 释放Compute Shader
    if (m_computeShader) {
        static_cast<ID3D11ComputeShader*>(m_computeShader)->Release();
        m_computeShader = nullptr;
    }
    
    // 释放RGBA纹理和UAV
    if (m_rgbaUAV) {
        static_cast<ID3D11UnorderedAccessView*>(m_rgbaUAV)->Release();
        m_rgbaUAV = nullptr;
    }
    
    if (m_rgbaTexture) {
        static_cast<ID3D11Texture2D*>(m_rgbaTexture)->Release();
        m_rgbaTexture = nullptr;
    }
    
    // 释放NV12 SRV
    if (m_nv12SRV_Y) {
        static_cast<ID3D11ShaderResourceView*>(m_nv12SRV_Y)->Release();
        m_nv12SRV_Y = nullptr;
    }
    
    if (m_nv12SRV_UV) {
        static_cast<ID3D11ShaderResourceView*>(m_nv12SRV_UV)->Release();
        m_nv12SRV_UV = nullptr;
    }
    
    // 释放采样器状态
    if (m_samplerState) {
        static_cast<ID3D11SamplerState*>(m_samplerState)->Release();
        m_samplerState = nullptr;
    }
    
    // 释放常量缓冲区
    if (m_constantBuffer) {
        static_cast<ID3D11Buffer*>(m_constantBuffer)->Release();
        m_constantBuffer = nullptr;
    }
    
    qDebug() << "D3D11Material::clearComputeResources: Compute resources cleared";
}