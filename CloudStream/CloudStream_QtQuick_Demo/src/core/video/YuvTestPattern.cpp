#include "YuvTestPattern.h"
#include <cstring>
#include <vector>
#include <memory>

// 8种彩条的YUV值
static const struct {
    uint8_t y, u, v;
} colorbars[8] = {
    {235, 128, 128}, // 白
    {219, 16, 138},  // 黄
    {188, 154, 16},  // 青
    {173, 42, 26},   // 绿
    {78, 214, 230},  // 品红
    {63, 102, 240},  // 红
    {32, 240, 118},  // 蓝
    {16, 128, 128}   // 黑
};

I420Frame YuvTestPattern::fillColorBarsI420(int width, int height)
{
    I420Frame frame(width, height);
    
    uint8_t* y_plane = frame.MutableDataY();
    uint8_t* u_plane = frame.MutableDataU();
    uint8_t* v_plane = frame.MutableDataV();
    
    const int strideY = frame.StrideY();
    const int strideU = frame.StrideU();
    const int strideV = frame.StrideV();

    // 计算每个彩条的宽度
    const int bar_width = width / 8;

    // 填充Y平面（8种彩条）
    for (int y = 0; y < height; ++y) {
        for (int x = 0; x < width; ++x) {
            // 根据水平位置确定彩条索引
            int bar_index = x / bar_width;
            y_plane[y * strideY + x] = colorbars[bar_index].y;
        }
    }

    // 填充U/V平面（8种彩条）
    for (int y = 0; y < height / 2; ++y) {
        for (int x = 0; x < width / 2; ++x) {
            // 计算对应原始图像的位置（2x2块）
            int src_x = x * 2;
            int bar_index = src_x / bar_width;
            u_plane[y * strideU + x] = colorbars[bar_index].u;
            v_plane[y * strideV + x] = colorbars[bar_index].v;
        }
    }
    return frame;
}

// I420Frame构造函数实现
I420Frame::I420Frame(int w, int h)
    : width_(w), height_(h),
      strideY_(w),
      strideU_(w/2),
      strideV_(w/2)
{
    // 分配内存
    int y_size = w * h;
    int uv_size = (w/2) * (h/2);
    
    dataY_ = std::make_unique<uint8_t[]>(y_size);
    dataU_ = std::make_unique<uint8_t[]>(uv_size);
    dataV_ = std::make_unique<uint8_t[]>(uv_size);
    
    // 初始化内存
    memset(dataY_.get(), 0, y_size);
    memset(dataU_.get(), 128, uv_size); // U平面初始化为128（中性色）
    memset(dataV_.get(), 128, uv_size); // V平面初始化为128（中性色）
}
