#pragma once

#include <QSGGeometryNode>
#include <QSizeF>
#include <memory>
#include <QQuickWindow>
#include "Frame.h"

class YuvMaterial;

/**
 * @brief YUV渲染节点类
 * 
 * 负责将YUV420格式的视频帧渲染到QML场景图中。
 * 该类继承自QSGGeometryNode，管理渲染所需的几何信息和材质。
 * 
 * @note 当前版本仅支持I420_CPU类型的帧数据
 * @todo 未来需要扩展支持D3D11_GPU类型的纹理渲染
 */
class YuvNode : public QSGGeometryNode
{
public:
    /**
     * @brief 构造函数
     * 
     * 初始化几何节点和YUV材质
     */
    YuvNode();
    
    /**
     * @brief 析构函数
     */
    ~YuvNode();

    /**
     * @brief 设置视频帧数据并更新渲染
     * 
     * @param window 渲染窗口指针，用于创建纹理
     * @param frame 视频帧数据指针，当前仅支持I420_CPU类型
     * @param itemSize 渲染区域的尺寸
     * @param frameDirty 标识帧数据是否已更新，需要重新上传到GPU
     * 
     * @note 如果传入D3D11_GPU类型的帧，当前会被忽略
     */
    void setFrame(QQuickWindow* window, 
                  const VideoFrameData* frame, 
                  const QSizeF& itemSize, 
                  bool frameDirty);

private:
    /**
     * @brief 更新几何信息
     * 
     * 根据渲染区域尺寸更新顶点位置和纹理坐标
     * 
     * @param itemSize 渲染区域的尺寸
     */
    void updateGeometry(const QSizeF& itemSize);

    /**
     * @brief 更新材质数据
     * 
     * 将YUV平面数据上传到GPU纹理
     * 
     * @param window 渲染窗口指针
     * @param DataY Y分量数据指针（亮度）
     * @param DataU U分量数据指针（色度蓝）
     * @param DataV V分量数据指针（色度红）
     * @param StrideY Y平面的行步长（字节数）
     * @param StrideU U平面的行步长（字节数）
     * @param StrideV V平面的行步长（字节数）
     * @param width 视频帧宽度
     * @param height 视频帧高度
     * @param frameDirty 标识是否需要重新上传数据
     */
    void updateMaterial(QQuickWindow* window,
                       const uint8_t* DataY,
                       const uint8_t* DataU,
                       const uint8_t* DataV,
                       int StrideY, 
                       int StrideU, 
                       int StrideV,
                       int width, 
                       int height, 
                       bool frameDirty);

private:
    // 几何数据
    QSGGeometry m_geometry;            ///< 节点的几何信息（顶点、纹理坐标等）
    YuvMaterial* m_material = nullptr; ///< YUV材质对象指针

    // 当前帧参数缓存（用于检测变化）
    int m_frameWidth = 0;   ///< 当前帧宽度
    int m_frameHeight = 0;  ///< 当前帧高度
    int m_strideY = 0;      ///< Y分量行步长
    int m_strideU = 0;      ///< U分量行步长
    int m_strideV = 0;      ///< V分量行步长
};