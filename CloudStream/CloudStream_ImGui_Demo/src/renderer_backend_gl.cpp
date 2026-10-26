// renderer_backend_gl.cpp - OpenGL 渲染后端（macOS / Linux）
//
// 整个文件由平台宏守卫：源文件是 GLOB 收集的，Windows 构建时本文件会被编进去但内容为空。

#if defined(RENDERER_OPENGL)

#  include <vector>

#  include "logger.h"
#  include "renderer_backend.h"

#  if defined(__APPLE__)
#    include <CoreVideo/CoreVideo.h>
#    include <IOSurface/IOSurface.h>
#    include <OpenGL/CGLIOSurface.h>
#    include <OpenGL/gl3.h>
#    include <OpenGL/OpenGL.h>
#  else
#    include <GL/gl.h>
#    include <GL/glext.h>
#  endif

namespace {

const char* kVertSrc = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

const char* kFragI420Src = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 outColor;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
// 共享暂存纹理按「历史最大帧」分配，本帧可能只占左上角一块。
// uvScale 把采样限制在有效区域内，否则会采到上一路残留的像素。
uniform vec2 uvScale;
void main() {
    vec2 uv = vTexCoord * uvScale;
    float y = texture(texY, uv).r;
    float u = texture(texU, uv).r - 0.5;
    float v = texture(texV, uv).r - 0.5;
    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    outColor = vec4(r, g, b, 1.0);
}
)";

// NV12 片元着色器：Y 来自 R8 纹理，UV 来自 RG8 纹理（双平面）。
// 硬件解码（VideoToolbox）输出的 CVPixelBuffer 就是这个布局，
// 直接在 shader 里做 YUV->RGB，避免额外一次 GPU 转换 pass。
//
// 注意：IOSurface 只能绑定到 GL_TEXTURE_RECTANGLE，对应 sampler2DRect，
// 且纹理坐标是**非归一化**的（像素坐标），因此需要各自乘以平面尺寸。
// 使用 BT.709 video-range 系数，与解码器输出的
// kCVPixelFormatType_420YpCbCr8BiPlanarVideoRange 对应。
const char* kFragNv12Src = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 outColor;
uniform sampler2DRect texY;
uniform sampler2DRect texUV;
uniform vec2 sizeY;
uniform vec2 sizeUV;
void main() {
    // video range: Y 16-235, UV 16-240
    float y = (texture(texY, vTexCoord * sizeY).r - 0.0625) * 1.164383;
    vec2 uv = texture(texUV, vTexCoord * sizeUV).rg - vec2(0.5, 0.5);
    float r = y + 1.792741 * uv.y;
    float g = y - 0.213249 * uv.x - 0.532909 * uv.y;
    float b = y + 2.112402 * uv.x;
    outColor = vec4(r, g, b, 1.0);
}
)";

unsigned int compile_program(const char* vert_src, const char* frag_src) {
  unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vert_src, nullptr);
  glCompileShader(vs);
  int ok = 0;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(vs, 512, nullptr, log);
    LOG_ERROR("GLBackend", "Vertex shader error: %s", log);
    glDeleteShader(vs);
    return 0;
  }

  unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &frag_src, nullptr);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetShaderInfoLog(fs, 512, nullptr, log);
    LOG_ERROR("GLBackend", "Fragment shader error: %s", log);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return 0;
  }

  unsigned int prog = glCreateProgram();
  glAttachShader(prog, vs);
  glAttachShader(prog, fs);
  glLinkProgram(prog);
  glGetProgramiv(prog, GL_LINK_STATUS, &ok);
  if (!ok) {
    char log[512];
    glGetProgramInfoLog(prog, 512, nullptr, log);
    LOG_ERROR("GLBackend", "Shader link error: %s", log);
    glDeleteProgram(prog);
    prog = 0;
  }
  glDeleteShader(vs);
  glDeleteShader(fs);
  return prog;
}

unsigned int create_r8_texture(int w, int h) {
  unsigned int tex = 0;
  glGenTextures(1, &tex);
  glBindTexture(GL_TEXTURE_2D, tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_R8, w, h, 0, GL_RED, GL_UNSIGNED_BYTE, nullptr);
  return tex;
}

// OutputTexture.opaque / ui_handle 都存 GLuint 纹理 id
unsigned int tex_of(const OutputTexture& t) { return (unsigned int)(uintptr_t)t.opaque; }

class GlBackend : public RendererBackend {
 public:
  bool init_shared() override {
    m_prog_i420 = compile_program(kVertSrc, kFragI420Src);
    if (!m_prog_i420) return false;

    // NV12 program 只有硬解路径需要，编译失败不阻塞软解
    m_prog_nv12 = compile_program(kVertSrc, kFragNv12Src);
    if (!m_prog_nv12) {
      LOG_WARN("GLBackend", "NV12 program unavailable, hardware decode rendering disabled");
    }

    const float vertices[] = {
        // pos        uv
        -1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f,
    };
    glGenVertexArrays(1, &m_vao);
    glGenBuffers(1, &m_vbo);
    glBindVertexArray(m_vao);
    glBindBuffer(GL_ARRAY_BUFFER, m_vbo);
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);
    glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
    glEnableVertexAttribArray(0);
    glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
    glEnableVertexAttribArray(1);
    glBindVertexArray(0);

    // 单个共享 FBO，每次上传时把目标输出纹理挂上去
    glGenFramebuffers(1, &m_fbo);

    // uniform 位置在 link 后就固定了，缓存下来避免每帧按名字做字符串查找。
    // 数百路 × 每路每帧 4 次查找，是白给的开销。
    m_u_tex_y = glGetUniformLocation(m_prog_i420, "texY");
    m_u_tex_u = glGetUniformLocation(m_prog_i420, "texU");
    m_u_tex_v = glGetUniformLocation(m_prog_i420, "texV");
    m_u_uv_scale = glGetUniformLocation(m_prog_i420, "uvScale");
    if (m_prog_nv12) {
      m_u_nv12_y = glGetUniformLocation(m_prog_nv12, "texY");
      m_u_nv12_uv = glGetUniformLocation(m_prog_nv12, "texUV");
      m_u_nv12_size_y = glGetUniformLocation(m_prog_nv12, "sizeY");
      m_u_nv12_size_uv = glGetUniformLocation(m_prog_nv12, "sizeUV");
    }

    LOG_INFO("GLBackend", "shared resources initialized");
    return true;
  }

  void destroy_shared() override {
    if (m_prog_i420) glDeleteProgram(m_prog_i420);
    if (m_prog_nv12) glDeleteProgram(m_prog_nv12);
    if (m_scratch_y) glDeleteTextures(1, &m_scratch_y);
    if (m_scratch_u) glDeleteTextures(1, &m_scratch_u);
    if (m_scratch_v) glDeleteTextures(1, &m_scratch_v);
    if (m_nv12_y) glDeleteTextures(1, &m_nv12_y);
    if (m_nv12_uv) glDeleteTextures(1, &m_nv12_uv);
    if (m_vao) glDeleteVertexArrays(1, &m_vao);
    if (m_vbo) glDeleteBuffers(1, &m_vbo);
    if (m_fbo) glDeleteFramebuffers(1, &m_fbo);
    *this = GlBackend();
  }

  bool create_output(int w, int h, OutputTexture* out) override {
    unsigned int tex = 0;
    glGenTextures(1, &tex);
    glBindTexture(GL_TEXTURE_2D, tex);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, w, h, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
    if (!tex) return false;

    out->opaque = (void*)(uintptr_t)tex;
    out->ui_handle = (void*)(uintptr_t)tex;
    out->width = w;
    out->height = h;
    return true;
  }

  void destroy_output(OutputTexture* out) override {
    unsigned int tex = tex_of(*out);
    if (tex) glDeleteTextures(1, &tex);
    *out = OutputTexture();
  }

  bool upload_i420(const I420Frame& src, OutputTexture* dst) override {
    if (!m_prog_i420 || !dst->ui_handle) return false;
    ensure_scratch(src.width, src.height);

    glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.stride_y);
    glBindTexture(GL_TEXTURE_2D, m_scratch_y);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src.width, src.height, GL_RED, GL_UNSIGNED_BYTE, src.data_y);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.stride_u);
    glBindTexture(GL_TEXTURE_2D, m_scratch_u);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src.width / 2, src.height / 2, GL_RED, GL_UNSIGNED_BYTE, src.data_u);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, src.stride_v);
    glBindTexture(GL_TEXTURE_2D, m_scratch_v);
    glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, src.width / 2, src.height / 2, GL_RED, GL_UNSIGNED_BYTE, src.data_v);

    glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

    GlStateGuard guard;
    begin_draw(*dst);
    glUseProgram(m_prog_i420);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_scratch_y);
    glUniform1i(m_u_tex_y, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_scratch_u);
    glUniform1i(m_u_tex_u, 1);

    glActiveTexture(GL_TEXTURE2);
    glBindTexture(GL_TEXTURE_2D, m_scratch_v);
    glUniform1i(m_u_tex_v, 2);

    // 暂存纹理比本帧大时，只采样左上角的有效区域
    glUniform2f(m_u_uv_scale, (float)src.width / (float)m_scratch_w, (float)src.height / (float)m_scratch_h);

    draw_quad();
    return true;
  }

  bool upload_gpu(const TcrGpuBuffer& src, OutputTexture* dst) override {
#  if defined(__APPLE__)
    if (src.platform != TCR_GPU_PLATFORM_CVPIXELBUFFER || !src.handle) return false;
    if (src.pixel_format != TCR_GPU_PIXEL_FORMAT_NV12) return false;
    if (!m_prog_nv12 || !dst->ui_handle) return false;

    CVPixelBufferRef pb = (CVPixelBufferRef)src.handle;
    IOSurfaceRef surface = CVPixelBufferGetIOSurface(pb);
    if (!surface) return false;  // 非 IOSurface 支撑，无法零拷贝

    CGLContextObj cgl = CGLGetCurrentContext();
    if (!cgl) return false;

    const int w = src.width;
    const int h = src.height;

    if (!m_nv12_y) glGenTextures(1, &m_nv12_y);
    if (!m_nv12_uv) glGenTextures(1, &m_nv12_uv);

    // IOSurface 只能绑定到 GL_TEXTURE_RECTANGLE
    if (!bind_iosurface_plane(cgl, m_nv12_y, GL_R8, w, h, GL_RED, surface, 0)) return false;
    if (!bind_iosurface_plane(cgl, m_nv12_uv, GL_RG8, (w + 1) / 2, (h + 1) / 2, GL_RG, surface, 1)) return false;

    GlStateGuard guard;
    begin_draw(*dst);
    glUseProgram(m_prog_nv12);

    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_nv12_y);
    glUniform1i(m_u_nv12_y, 0);

    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_RECTANGLE, m_nv12_uv);
    glUniform1i(m_u_nv12_uv, 1);

    // sampler2DRect 使用像素坐标，需把 [0,1] 的 uv 缩放到各平面实际尺寸
    glUniform2f(m_u_nv12_size_y, (float)w, (float)h);
    glUniform2f(m_u_nv12_size_uv, (float)((w + 1) / 2), (float)((h + 1) / 2));

    draw_quad();
    return true;
#  else
    (void)src;
    (void)dst;
    return false;
#  endif
  }

  int sample_luma(const OutputTexture& tex) override {
    if (!tex.ui_handle || tex.width <= 0 || tex.height <= 0) return -1;

    // 缩小采样区域，避免整帧回读带来的开销
    const int sw = tex.width < 64 ? tex.width : 64;
    const int sh = tex.height < 64 ? tex.height : 64;
    std::vector<uint8_t> px((size_t)sw * sh * 4, 0);

    GLint prev_fbo = 0;
    glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_of(tex), 0);
    glPixelStorei(GL_PACK_ALIGNMENT, 1);
    glReadPixels(0, 0, sw, sh, GL_RGBA, GL_UNSIGNED_BYTE, px.data());
    glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);

    uint64_t sum = 0;
    for (size_t i = 0; i + 2 < px.size(); i += 4) {
      sum += (uint64_t)((px[i] * 299 + px[i + 1] * 587 + px[i + 2] * 114) / 1000);
    }
    return (int)(sum / (uint64_t)(sw * sh));
  }

 private:
  // 保存/恢复被我们改动的 GL 状态。ImGui 在同一个 context 上绘制，
  // 不恢复会让后续 UI 绘制用错 FBO/program/viewport。
  struct GlStateGuard {
    GLint fbo = 0;
    GLint program = 0;
    GLint viewport[4] = {};
    GlStateGuard() {
      glGetIntegerv(GL_FRAMEBUFFER_BINDING, &fbo);
      glGetIntegerv(GL_CURRENT_PROGRAM, &program);
      glGetIntegerv(GL_VIEWPORT, viewport);
    }
    ~GlStateGuard() {
      glBindFramebuffer(GL_FRAMEBUFFER, fbo);
      glUseProgram(program);
      glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
    }
  };

  // 把共享 FBO 指向目标输出纹理并设好视口
  void begin_draw(const OutputTexture& dst) {
    glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
    glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex_of(dst), 0);
    glViewport(0, 0, dst.width, dst.height);
  }

  void draw_quad() {
    glBindVertexArray(m_vao);
    glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
    glBindVertexArray(0);
  }

  // 暂存纹理按「见过的最大帧尺寸」增长，只增不减，所有流轮流复用
  void ensure_scratch(int w, int h) {
    if (w <= m_scratch_w && h <= m_scratch_h) return;
    const int nw = w > m_scratch_w ? w : m_scratch_w;
    const int nh = h > m_scratch_h ? h : m_scratch_h;
    if (m_scratch_y) glDeleteTextures(1, &m_scratch_y);
    if (m_scratch_u) glDeleteTextures(1, &m_scratch_u);
    if (m_scratch_v) glDeleteTextures(1, &m_scratch_v);
    m_scratch_y = create_r8_texture(nw, nh);
    m_scratch_u = create_r8_texture(nw / 2, nh / 2);
    m_scratch_v = create_r8_texture(nw / 2, nh / 2);
    m_scratch_w = nw;
    m_scratch_h = nh;
    LOG_INFO("GLBackend", "shared YUV scratch resized to %dx%d", nw, nh);
  }

#  if defined(__APPLE__)
  bool bind_iosurface_plane(CGLContextObj cgl, unsigned int tex, GLenum internal_fmt, int w, int h, GLenum fmt,
                            IOSurfaceRef surface, uint32_t plane) {
    glBindTexture(GL_TEXTURE_RECTANGLE, tex);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
    glTexParameteri(GL_TEXTURE_RECTANGLE, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
    CGLError err =
        CGLTexImageIOSurface2D(cgl, GL_TEXTURE_RECTANGLE, internal_fmt, w, h, fmt, GL_UNSIGNED_BYTE, surface, plane);
    if (err != kCGLNoError) {
      LOG_ERROR("GLBackend", "CGLTexImageIOSurface2D(plane%u) failed: %d", plane, (int)err);
      return false;
    }
    return true;
  }
#  endif

  unsigned int m_prog_i420 = 0;
  unsigned int m_prog_nv12 = 0;
  unsigned int m_vao = 0;
  unsigned int m_vbo = 0;
  unsigned int m_fbo = 0;

  unsigned int m_scratch_y = 0;
  unsigned int m_scratch_u = 0;
  unsigned int m_scratch_v = 0;
  int m_scratch_w = 0;
  int m_scratch_h = 0;

  unsigned int m_nv12_y = 0;
  unsigned int m_nv12_uv = 0;

  // 缓存的 uniform 位置（init_shared 里解析一次）
  int m_u_tex_y = -1;
  int m_u_tex_u = -1;
  int m_u_tex_v = -1;
  int m_u_uv_scale = -1;
  int m_u_nv12_y = -1;
  int m_u_nv12_uv = -1;
  int m_u_nv12_size_y = -1;
  int m_u_nv12_size_uv = -1;
};

}  // namespace

RendererBackend* create_renderer_backend(void* device, void* context) {
  (void)device;
  (void)context;
  return new GlBackend();
}

#endif  // RENDERER_OPENGL
