#pragma once

// renderer_backend.h - video_renderer 与具体图形 API 之间的唯一接缝
//
// 上层（video_renderer.cpp）只做平台无关的事：尺寸裁剪、生命周期、状态维护。
// 所有 GPU 调用都在后端实现里，每个平台一个文件：
//   renderer_backend_gl.cpp     macOS / Linux（OpenGL）
//   renderer_backend_d3d11.cpp  Windows（D3D11）
//
// 要适配新平台或优化现有平台，只需改对应的后端文件，不必碰上层。

#include <cstdint>

#include "tcr_c_api.h"

// 一路流独占的输出纹理。
// opaque 是后端私有对象；ui_handle 是交给 ImGui::Image() 的句柄
// （OpenGL 为纹理 id，D3D11 为 ID3D11ShaderResourceView*）。
struct OutputTexture {
  void* opaque = nullptr;
  void* ui_handle = nullptr;
  int width = 0;
  int height = 0;
};

// I420 源帧。把散参数打包，避免 8 个参数在各层之间来回传。
struct I420Frame {
  const uint8_t* data_y = nullptr;
  const uint8_t* data_u = nullptr;
  const uint8_t* data_v = nullptr;
  int stride_y = 0;
  int stride_u = 0;
  int stride_v = 0;
  int width = 0;
  int height = 0;
};

// 平台渲染后端。
//
// **多路小流规模化的三条约束** —— 适配新平台或改动现有后端时必须保持，
// 否则显存与初始化开销会随路数线性爆掉：
//
//   1. 着色器、顶点几何、YUV 暂存纹理、渲染目标这些「只在上传那一瞬间用到」的
//      资源，全局共享一份（init_shared），**不要每路一套**。它们的内容上传完就
//      已经转进输出纹理了，没有常驻价值。
//   2. 每路只独占一张输出纹理 —— UI 每帧都要采样它，只有它必须常驻。
//   3. 输出纹理尺寸由上层裁剪后通过 create_output 传入，后端照着建即可，
//      **不要按源分辨率建**。平铺时格子只有几十像素，按源分辨率建纯属浪费。
//
// 实测（macOS / OpenGL，520 路 288x512@1fps）：违反第 1、2 条时占用 818MB 显存、
// 2103 个 GPU region；三条都遵守后降到 624KB、528 个 region，主循环 250→434fps。
class RendererBackend {
 public:
  virtual ~RendererBackend() = default;

  // 建立/销毁全局共享资源。进程内各调用一次。
  virtual bool init_shared() = 0;
  virtual void destroy_shared() = 0;

  // 每路独占的输出纹理。w/h 已是裁剪后的目标尺寸。
  // 重复调用前上层会先 destroy_output，后端无需自己处理尺寸变化。
  virtual bool create_output(int w, int h, OutputTexture* out) = 0;
  virtual void destroy_output(OutputTexture* out) = 0;

  // 把 I420 源帧转换并缩放进 dst。dst 尺寸可以小于源尺寸。
  virtual bool upload_i420(const I420Frame& src, OutputTexture* dst) = 0;

  // 硬件解码帧的零拷贝路径。平台或格式不支持时返回 false
  // （GPU 帧没有 I420 数据可回退，调用方只能放弃该帧）。
  virtual bool upload_gpu(const TcrGpuBuffer& src, OutputTexture* dst) = 0;

  // 回读输出纹理的平均亮度（0~255），用于自动化验证「真的渲染出了画面」。
  // 会做 GPU→CPU 同步回读，有开销。不支持的后端返回 -1。
  virtual int sample_luma(const OutputTexture& tex) = 0;
};

// 由各平台的后端文件实现。
// device/context 只有 D3D11 需要（ID3D11Device* / ID3D11DeviceContext*），
// OpenGL 平台忽略这两个参数。失败返回 nullptr。
RendererBackend* create_renderer_backend(void* device, void* context);
