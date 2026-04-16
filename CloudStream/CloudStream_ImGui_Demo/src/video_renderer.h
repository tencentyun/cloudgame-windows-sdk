#pragma once

// video_renderer.h - 视频渲染器抽象接口 + 平台实现
// Windows: D3D11 纹理渲染
// macOS/Linux: OpenGL 纹理渲染
// 渲染结果通过 get_texture_id() 提供给 ImGui::Image() 使用

#include <cstdint>

#if defined(RENDERER_D3D11)
struct ID3D11Device;
struct ID3D11DeviceContext;
struct ID3D11ShaderResourceView;
#endif

class VideoRenderer {
 public:
  VideoRenderer();
  ~VideoRenderer();

  // 初始化渲染资源（在图形上下文创建后调用）
#if defined(RENDERER_D3D11)
  bool init(ID3D11Device* device, ID3D11DeviceContext* context);
#else
  bool init();
#endif

  // 上传 I420 帧数据到 GPU 纹理
  void upload_frame(const uint8_t* data_y, const uint8_t* data_u, const uint8_t* data_v, int stride_y, int stride_u,
                    int stride_v, int width, int height);

  // 获取用于 ImGui::Image() 的纹理 ID
  void* get_texture_id() const;

  // 获取当前纹理尺寸
  int texture_width() const { return m_width; }
  int texture_height() const { return m_height; }

  // 是否有有效帧
  bool has_frame() const { return m_has_frame; }

  // 释放渲染资源
  void destroy();

 private:
  int m_width = 0;
  int m_height = 0;
  bool m_has_frame = false;

#if defined(RENDERER_D3D11)
  // --- D3D11 实现 ---
  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;
  ID3D11ShaderResourceView* m_srv = nullptr;

  // D3D11 纹理和着色器资源
  struct D3D11Resources;
  D3D11Resources* m_d3d = nullptr;

  void create_or_resize_d3d11(int width, int height);
  void upload_yuv_d3d11(const uint8_t* y, const uint8_t* u, const uint8_t* v, int sy, int su, int sv, int w, int h);
#else
  // --- OpenGL 实现 ---
  unsigned int m_tex_y = 0;
  unsigned int m_tex_u = 0;
  unsigned int m_tex_v = 0;
  unsigned int m_fbo = 0;
  unsigned int m_rgba_tex = 0;
  unsigned int m_shader = 0;
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;

  void create_or_resize_gl(int width, int height);
  void upload_yuv_gl(const uint8_t* y, const uint8_t* u, const uint8_t* v, int sy, int su, int sv, int w, int h);
  unsigned int compile_shader(const char* vert_src, const char* frag_src);
#endif
};
