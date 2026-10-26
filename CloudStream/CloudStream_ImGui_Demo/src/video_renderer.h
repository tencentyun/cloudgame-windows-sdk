#pragma once

// video_renderer.h - 单路视频画面的渲染器（平台无关）
//
// 一个 VideoRenderer 对应一路流，持有该路独占的输出纹理，
// 通过 get_texture_id() 把它交给 ImGui::Image() 显示。
// 所有 GPU 调用都委托给 RendererBackend，本类不含任何平台代码。

#include <cstdint>

#include "renderer_backend.h"

class VideoRenderer {
 public:
  // === 进程级图形初始化（调用一次）===

  // 创建平台后端并建立共享 GPU 资源。
  // Windows 传 (ID3D11Device*, ID3D11DeviceContext*)；OpenGL 平台两个参数都忽略，传 nullptr。
  static bool init_graphics(void* device, void* context);
  static void shutdown_graphics();

  // 限制每路输出纹理的尺寸上限，超出则等比缩小。0 表示不限制。
  //
  // 这是多路小流能把显存压下来的关键：平铺时每个格子在屏幕上只有几十像素，
  // 按解码分辨率（如 288x512）给每路留一张全尺寸纹理，数百路下就是数百 MB 的浪费。
  static void set_max_output_size(int max_w, int max_h);

  VideoRenderer();
  ~VideoRenderer();

  VideoRenderer(const VideoRenderer&) = delete;
  VideoRenderer& operator=(const VideoRenderer&) = delete;

  // 必须在 init_graphics() 成功之后调用。
  bool init();

  // 上传 I420 帧（软解路径）
  void upload_frame(const uint8_t* data_y, const uint8_t* data_u, const uint8_t* data_v, int stride_y, int stride_u,
                    int stride_v, int width, int height);

  // 上传硬件解码产出的 GPU 纹理帧（零拷贝路径）。
  // 返回 false 表示该平台/格式暂不支持，GPU 帧无 I420 可回退，调用方只能放弃该帧。
  bool upload_gpu_frame(const TcrGpuBuffer& gpu);

  // 用于 ImGui::Image() 的纹理句柄
  void* get_texture_id() const { return m_out.ui_handle; }

  // **源帧**尺寸，用于按原始宽高比摆放画面。
  // 注意这不是 GPU 上纹理的实际尺寸 —— 后者受 set_max_output_size 裁剪，通常更小。
  int frame_width() const { return m_frame_w; }
  int frame_height() const { return m_frame_h; }

  bool has_frame() const { return m_has_frame; }

  // 回读平均亮度（0~255），用于验证确实渲染出了画面。-1 表示不可用。
  int sample_average_luma();

  void destroy();

 private:
  // 确保输出纹理存在且尺寸匹配（按上限裁剪后的目标尺寸）
  bool ensure_output(int src_w, int src_h);

  OutputTexture m_out;
  int m_frame_w = 0;
  int m_frame_h = 0;
  bool m_has_frame = false;
};
