#pragma once

#include <QSGGeometryNode>
#include <QSizeF>
#include <memory>
#include <QQuickWindow>
#include "Frame.h"

class YuvMaterial;

// YUV渲染节点，负责将YUV帧渲染到QML场景中
class YuvNode : public QSGGeometryNode
{
public:
    YuvNode();   // 构造函数
    ~YuvNode();  // 析构函数

    // 设置帧数据和渲染区域尺寸，frameDirty表示是否需要上传新数据
    void setFrame(QQuickWindow* window, const VideoFrameData* frame, const QSizeF& itemSize, bool frameDirty);

private:
    // 更新几何信息（顶点、纹理坐标等）
    void updateGeometry(const QSizeF& itemSize);

    // 更新材质（上传YUV数据到纹理）
    void updateMaterial(
        QQuickWindow* window,
        const uint8_t* DataY,
        const uint8_t* DataU,
        const uint8_t* DataV,
        int StrideY, 
        int StrideU, 
        int StrideV,
        int width, int height, 
        bool frameDirty);

    QSGGeometry m_geometry;      // 节点几何信息
    YuvMaterial* m_material = nullptr; // YUV材质

    int m_frameWidth = 0;        // 当前帧宽度
    int m_frameHeight = 0;       // 当前帧高度
    int m_strideY = 0;           // Y分量步长
    int m_strideU = 0;           // U分量步长
    int m_strideV = 0;           // V分量步长
};