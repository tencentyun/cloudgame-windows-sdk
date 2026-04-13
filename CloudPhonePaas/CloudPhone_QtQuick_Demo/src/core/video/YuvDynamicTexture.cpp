#include "YuvDynamicTexture.h"
#include "utils/Logger.h"
#include <QQuickWindow>
#include <QSGTexture>
#include <QElapsedTimer>
#include <private/qrhi_p.h>
#include <cstring>

YuvDynamicTexture::YuvDynamicTexture(const QSize& size, QRhi* rhi)
    : m_rhi(rhi)
    , m_rhiTexture(nullptr)
    , m_size(size)
    , m_dataDirty{false}  // 使用大括号初始化atomic类型
{
    // 预分配图像缓冲区（单通道灰度格式）
    m_imageBuffer = QImage(size, QImage::Format_Grayscale8);
    
    // 创建RHI纹理
    if (!createRhiTexture()) {
        Logger::warning("Failed to create RHI texture in YuvDynamicTexture constructor");
    }
}

YuvDynamicTexture::~YuvDynamicTexture()
{
    destroyRhiTexture();
}

qint64 YuvDynamicTexture::comparisonKey() const
{
    // 在RHI模式下，使用纹理对象指针作为比较键
    return m_rhiTexture ? qint64(m_rhiTexture) : 0;
}

QSize YuvDynamicTexture::textureSize() const
{
    return m_size;
}

bool YuvDynamicTexture::hasAlphaChannel() const
{
    // YUV纹理的单个平面不包含Alpha通道
    return false;
}

bool YuvDynamicTexture::hasMipmaps() const
{
    // 视频纹理通常不需要mipmap
    return false;
}

QRhiTexture* YuvDynamicTexture::rhiTexture() const
{
    return m_rhiTexture;
}

bool YuvDynamicTexture::updateTexture()
{
    // 使用memory_order_acquire确保读取到最新值
    bool dataDirty = m_dataDirty.load(std::memory_order_acquire);
    
    // 如果数据未标记为脏或纹理无效，则无需更新
    if (!dataDirty || !m_rhiTexture) {
        Logger::info("[YuvDynamicTexture::updateTexture] No update needed, returning false");
        return false;
    }

    return true;
}

void YuvDynamicTexture::commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates)
{
    QElapsedTimer timer;
    timer.start();
    
    // 使用memory_order_acquire读取最新值
    bool dataDirty = m_dataDirty.load(std::memory_order_acquire);
    
    // 检查纹理和资源更新批次是否有效
    if (!m_rhiTexture || !resourceUpdates) {
        Logger::warning("[YuvDynamicTexture::commitTextureOperations] Invalid texture or resourceUpdates");
        return;
    }
    
    // 如果图像缓冲区为空，无需上传
    if (m_imageBuffer.isNull()) {
        Logger::warning("[YuvDynamicTexture::commitTextureOperations] Image buffer is null");
        return;
    }
    
    // 只有在数据标记为脏时才上传
    if (!dataDirty) {
        // Logger::info("[YuvDynamicTexture::commitTextureOperations] Data not dirty, skipping upload");
        return;
    }
    
    // 创建纹理子资源上传描述（使用QImage数据）
    QRhiTextureSubresourceUploadDescription subresDesc(m_imageBuffer);
    
    // 创建纹理上传条目（layer=0, level=0表示第一层第一级mipmap）
    QRhiTextureUploadEntry uploadEntry(0, 0, subresDesc);
    
    // 创建纹理上传描述（直接用上传条目初始化）
    QRhiTextureUploadDescription uploadDesc(uploadEntry);
    
    // 将上传操作添加到资源更新批次
    resourceUpdates->uploadTexture(m_rhiTexture, uploadDesc);
    
    // 在实际上传完成后才清除脏标志
    m_dataDirty = false;
}

void YuvDynamicTexture::setTextureData(const uint8_t* data, int width, int height, int stride)
{
    QElapsedTimer timer;
    timer.start();
    
    // 验证参数
    if (!data || width <= 0 || height <= 0) {
        Logger::warning("Invalid texture data parameters");
        return;
    }
    
    // 检查尺寸是否匹配
    if (width != m_size.width() || height != m_size.height()) {
        Logger::warning(QString("Texture size mismatch: expected %1x%2, got %3x%4")
                       .arg(m_size.width()).arg(m_size.height())
                       .arg(width).arg(height));
        return;
    }
    
    // 记录是否覆盖了未上传的数据
    if (m_dataDirty.load(std::memory_order_acquire)) {
        // Logger::warning(QString("[YuvDynamicTexture::setTextureData] Overwriting dirty data! "
        //                        "size=%1x%2, texture=%3")
        //                .arg(width).arg(height)
        //                .arg(reinterpret_cast<quintptr>(m_rhiTexture)));
    }
    
    qint64 copyStartTime = timer.elapsed();
    
    // 复制数据到图像缓冲区
    for (int row = 0; row < height; ++row) {
        memcpy(m_imageBuffer.scanLine(row), data + row * stride, width);
    }
    
    qint64 copyTime = timer.elapsed() - copyStartTime;
    
    // 标记数据为脏，需要更新，使用memory_order_release确保写入对渲染线程可见
    m_dataDirty.store(true, std::memory_order_release);
}

bool YuvDynamicTexture::createRhiTexture()
{
    QElapsedTimer timer;
    timer.start();
    
    if (!m_rhi) {
        Logger::warning("RHI is null, cannot create texture");
        return false;
    }
    
    // 销毁旧纹理（如果存在）
    destroyRhiTexture();
    
    qint64 createStartTime = timer.elapsed();
    
    // 创建新的RHI纹理对象
    m_rhiTexture = m_rhi->newTexture(
        QRhiTexture::R8,               // 单通道8位格式（灰度）
        m_size,                        // 纹理尺寸
        1,                             // 采样数（无多重采样）
        QRhiTexture::Flag(0)           // 无特殊标志
    );
    
    if (!m_rhiTexture) {
        Logger::warning("Failed to create QRhiTexture");
        return false;
    }
    
    // 构建纹理资源
    if (!m_rhiTexture->create()) {
        Logger::warning("Failed to build QRhiTexture");
        delete m_rhiTexture;
        m_rhiTexture = nullptr;
        return false;
    }
    
    qint64 createTime = timer.elapsed() - createStartTime;
    
    // 配置纹理采样参数
    setFiltering(QSGTexture::Linear);
    setHorizontalWrapMode(QSGTexture::ClampToEdge);
    setVerticalWrapMode(QSGTexture::ClampToEdge);
    
    return true;
}

void YuvDynamicTexture::destroyRhiTexture()
{
    if (m_rhiTexture) {
        delete m_rhiTexture;
        m_rhiTexture = nullptr;
    }
}
