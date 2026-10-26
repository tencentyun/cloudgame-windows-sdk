#include "video_renderer.h"

#include "logger.h"

namespace {

// 进程级后端实例与共享资源。所有 VideoRenderer 共用。
RendererBackend* g_backend = nullptr;

// 输出纹理尺寸上限，0 表示不限制
int g_max_out_w = 0;
int g_max_out_h = 0;

// 按上限等比缩小。保持宽高比，这样上层按源尺寸算出的摆放比例依然正确。
void clamp_output_size(int src_w, int src_h, int* out_w, int* out_h) {
  *out_w = src_w;
  *out_h = src_h;
  if (g_max_out_w <= 0 || g_max_out_h <= 0) return;
  if (src_w <= g_max_out_w && src_h <= g_max_out_h) return;

  double scale_w = (double)g_max_out_w / src_w;
  double scale_h = (double)g_max_out_h / src_h;
  double scale = scale_w < scale_h ? scale_w : scale_h;

  int w = (int)(src_w * scale);
  int h = (int)(src_h * scale);
  *out_w = w < 2 ? 2 : w;
  *out_h = h < 2 ? 2 : h;
}

}  // namespace

bool VideoRenderer::init_graphics(void* device, void* context) {
  if (g_backend) return true;

  g_backend = create_renderer_backend(device, context);
  if (!g_backend) {
    LOG_ERROR("VideoRenderer", "create_renderer_backend failed");
    return false;
  }
  if (!g_backend->init_shared()) {
    LOG_ERROR("VideoRenderer", "backend init_shared failed");
    delete g_backend;
    g_backend = nullptr;
    return false;
  }
  LOG_INFO("VideoRenderer", "graphics initialized");
  return true;
}

void VideoRenderer::shutdown_graphics() {
  if (!g_backend) return;
  g_backend->destroy_shared();
  delete g_backend;
  g_backend = nullptr;
}

void VideoRenderer::set_max_output_size(int max_w, int max_h) {
  g_max_out_w = max_w > 0 ? max_w : 0;
  g_max_out_h = max_h > 0 ? max_h : 0;
}

VideoRenderer::VideoRenderer() {}

VideoRenderer::~VideoRenderer() { destroy(); }

bool VideoRenderer::init() { return g_backend != nullptr; }

bool VideoRenderer::ensure_output(int src_w, int src_h) {
  int target_w = 0, target_h = 0;
  clamp_output_size(src_w, src_h, &target_w, &target_h);

  if (m_out.ui_handle && m_out.width == target_w && m_out.height == target_h) return true;

  g_backend->destroy_output(&m_out);
  return g_backend->create_output(target_w, target_h, &m_out);
}

void VideoRenderer::upload_frame(const uint8_t* data_y, const uint8_t* data_u, const uint8_t* data_v, int stride_y,
                                 int stride_u, int stride_v, int width, int height) {
  if (!g_backend || width <= 0 || height <= 0) return;
  if (!ensure_output(width, height)) return;

  I420Frame src;
  src.data_y = data_y;
  src.data_u = data_u;
  src.data_v = data_v;
  src.stride_y = stride_y;
  src.stride_u = stride_u;
  src.stride_v = stride_v;
  src.width = width;
  src.height = height;

  if (!g_backend->upload_i420(src, &m_out)) return;

  m_frame_w = width;
  m_frame_h = height;
  m_has_frame = true;
}

bool VideoRenderer::upload_gpu_frame(const TcrGpuBuffer& gpu) {
  if (!g_backend || gpu.width <= 0 || gpu.height <= 0) return false;
  if (!ensure_output(gpu.width, gpu.height)) return false;
  if (!g_backend->upload_gpu(gpu, &m_out)) return false;

  m_frame_w = gpu.width;
  m_frame_h = gpu.height;
  m_has_frame = true;
  return true;
}

int VideoRenderer::sample_average_luma() {
  if (!g_backend || !m_has_frame) return -1;
  return g_backend->sample_luma(m_out);
}

void VideoRenderer::destroy() {
  if (g_backend) g_backend->destroy_output(&m_out);
  m_out = OutputTexture();
  m_frame_w = 0;
  m_frame_h = 0;
  m_has_frame = false;
}
