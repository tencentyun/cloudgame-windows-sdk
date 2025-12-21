#pragma once

#include <QSGDynamicTexture>
#include <QSize>
#include <QImage>
#include <atomic>

// 前向声明RHI类型
class QRhi;
class QRhiTexture;

/**
 * @brief 动态YUV纹理类
 * 
 * 继承自QSGDynamicTexture，实现纹理复用和数据更新，
 * 避免每帧重建纹理对象，提升渲染性能。
 */
class YuvDynamicTexture : public QSGDynamicTexture
{
    Q_OBJECT
    
public:
    /**
     * @brief 构造函数
     * 
     * @param size 纹理尺寸
     * @param rhi RHI接口指针
     */
    explicit YuvDynamicTexture(const QSize& size, QRhi* rhi);
    
    /**
     * @brief 析构函数
     */
    ~YuvDynamicTexture() override;

    // ========== QSGTexture 接口实现 ==========
    
    /**
     * @brief 获取纹理ID（RHI模式下返回0）
     */
    qint64 comparisonKey() const override;
    
    /**
     * @brief 获取纹理尺寸
     */
    QSize textureSize() const override;
    
    /**
     * @brief 是否包含Alpha通道
     */
    bool hasAlphaChannel() const override;
    
    /**
     * @brief 是否有mipmap
     */
    bool hasMipmaps() const override;
    
    /**
     * @brief 获取RHI纹理对象
     */
    QRhiTexture* rhiTexture() const override;

    // ========== QSGDynamicTexture 接口实现 ==========
    
    /**
     * @brief 更新纹理数据
     * 
     * 这是关键方法，用于更新纹理内容而不重建纹理对象
     * 
     * @return 如果纹理数据已更新返回true
     */
    bool updateTexture() override;

    /**
     * @brief 提交纹理操作到GPU
     * 
     * 实际执行纹理数据上传到GPU的操作
     * 
     * @param rhi RHI接口指针
     * @param resourceUpdates 资源更新批次
     */
    void commitTextureOperations(QRhi* rhi, QRhiResourceUpdateBatch* resourceUpdates) override;

    // ========== 自定义方法 ==========
    
    /**
     * @brief 设置待更新的纹理数据
     * 
     * 将数据复制到内部缓冲区，在下次updateTexture()调用时上传到GPU
     * 
     * @param data 纹理数据指针（单通道灰度数据）
     * @param width 数据宽度
     * @param height 数据高度
     * @param stride 行步长（字节数）
     */
    void setTextureData(const uint8_t* data, int width, int height, int stride);
    
    /**
     * @brief 检查纹理是否有效
     */
    bool isValid() const { return m_rhiTexture != nullptr; }

private:
    /**
     * @brief 创建RHI纹理对象
     */
    bool createRhiTexture();
    
    /**
     * @brief 销毁RHI纹理对象
     */
    void destroyRhiTexture();

private:
    QRhi* m_rhi;                    ///< RHI接口指针
    QRhiTexture* m_rhiTexture;      ///< RHI纹理对象
    QSize m_size;                   ///< 纹理尺寸
    QImage m_imageBuffer;           ///< 图像数据缓冲区
    std::atomic<bool> m_dataDirty;  ///< 标识数据是否需要更新（原子操作保证线程安全）
};
