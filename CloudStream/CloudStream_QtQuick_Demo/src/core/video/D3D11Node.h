#pragma once

#include <QSGGeometryNode>
#include <QSizeF>
#include <QQuickWindow>
#include "Frame.h"

class D3D11Material;

/**
 * @brief D3D11纹理渲染节点类
 * 
 * 负责将D3D11 GPU纹理渲染到QML场景图中。
 * 该类继承自QSGGeometryNode，管理渲染所需的几何信息和材质。
 * 
 * @note 支持D3D11_GPU类型的帧数据
 * @note 需要Qt与D3D11的互操作支持
 */
class D3D11Node : public QSGGeometryNode
{
public:
    /**
     * @brief 构造函数
     * 
     * 初始化几何节点和D3D11材质
     */
    D3D11Node();
    
    /**
     * @brief 析构函数
     */
    ~D3D11Node();

    /**
     * @brief 设置视频帧数据并更新渲染
     * 
     * @param window 渲染窗口指针，用于获取渲染接口
     * @param frame 视频帧数据指针，必须是D3D11_GPU类型
     * @param itemSize 渲染区域的尺寸
     * @param frameDirty 标识帧数据是否已更新
     * 
     * @note 如果传入I420_CPU类型的帧，会被忽略
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
     * 将D3D11纹理信息传递给材质对象
     * 
     * @param window 渲染窗口指针
     * @param d3d11Data D3D11纹理数据
     * @param width 视频帧宽度
     * @param height 视频帧高度
     * @param frameDirty 标识是否需要更新
     */
    void updateMaterial(QQuickWindow* window,
                       const D3D11TextureData& d3d11Data,
                       int width, 
                       int height, 
                       bool frameDirty);

private:
    // 几何数据
    QSGGeometry m_geometry;              ///< 节点的几何信息（顶点、纹理坐标等）
    D3D11Material* m_material = nullptr; ///< D3D11材质对象指针

    // 当前帧参数缓存（用于检测变化）
    int m_frameWidth = 0;    ///< 当前帧宽度
    int m_frameHeight = 0;   ///< 当前帧高度
    void* m_lastTexture = nullptr;  ///< 上一次的纹理指针
};