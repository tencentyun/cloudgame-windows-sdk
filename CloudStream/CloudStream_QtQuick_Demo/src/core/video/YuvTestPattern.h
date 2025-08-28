#pragma once

#include <cstdint>
#include <memory>

// I420视频帧结构体，管理YUV分量内存及相关属性
struct I420Frame {
    I420Frame(int w, int h); // 构造函数，分配内存

    // 获取Y/U/V分量只读指针
    const uint8_t* DataY() const { return dataY_.get(); }
    const uint8_t* DataU() const { return dataU_.get(); }
    const uint8_t* DataV() const { return dataV_.get(); }

    // 获取Y/U/V分量可写指针
    uint8_t* MutableDataY() { return dataY_.get(); }
    uint8_t* MutableDataU() { return dataU_.get(); }
    uint8_t* MutableDataV() { return dataV_.get(); }

    // 获取Y/U/V分量步长
    int32_t StrideY() const { return strideY_; }
    int32_t StrideU() const { return strideU_; }
    int32_t StrideV() const { return strideV_; }

    // 获取帧宽高
    int32_t width() const { return width_; }
    int32_t height() const { return height_; }

private:
    int32_t width_;   // 帧宽
    int32_t height_;  // 帧高
    int32_t strideY_; // Y分量步长
    int32_t strideU_; // U分量步长
    int32_t strideV_; // V分量步长
    std::unique_ptr<uint8_t[]> dataY_; // Y分量数据
    std::unique_ptr<uint8_t[]> dataU_; // U分量数据
    std::unique_ptr<uint8_t[]> dataV_; // V分量数据
};

// YUV测试图案生成器
class YuvTestPattern
{
public:
    // 生成I420彩条测试图
    static I420Frame fillColorBarsI420(int width, int height);
};