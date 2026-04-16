#include "video_renderer.h"
#include <cstring>
#include "logger.h"

VideoRenderer::VideoRenderer() {}

VideoRenderer::~VideoRenderer() { destroy(); }

// =============================================================================
// D3D11 实现
// =============================================================================
#if defined(RENDERER_D3D11)

#  include <d3d11.h>
#  include <d3dcompiler.h>

struct VideoRenderer::D3D11Resources {
  ID3D11Texture2D* tex_y = nullptr;
  ID3D11Texture2D* tex_u = nullptr;
  ID3D11Texture2D* tex_v = nullptr;
  ID3D11Texture2D* tex_rgba = nullptr;
  ID3D11ShaderResourceView* srv_y = nullptr;
  ID3D11ShaderResourceView* srv_u = nullptr;
  ID3D11ShaderResourceView* srv_v = nullptr;
  ID3D11ShaderResourceView* srv_rgba = nullptr;
  ID3D11RenderTargetView* rtv = nullptr;
  ID3D11VertexShader* vs = nullptr;
  ID3D11PixelShader* ps = nullptr;
  ID3D11SamplerState* sampler = nullptr;
  ID3D11Buffer* vb = nullptr;
  ID3D11InputLayout* layout = nullptr;
  int alloc_w = 0;
  int alloc_h = 0;
};

static const char* d3d11_vs_src = R"(
struct VS_INPUT {
    float2 pos : POSITION;
    float2 uv  : TEXCOORD0;
};
struct VS_OUTPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
VS_OUTPUT main(VS_INPUT input) {
    VS_OUTPUT output;
    output.pos = float4(input.pos, 0.0, 1.0);
    output.uv  = input.uv;
    return output;
}
)";

static const char* d3d11_ps_src = R"(
Texture2D texY : register(t0);
Texture2D texU : register(t1);
Texture2D texV : register(t2);
SamplerState samp : register(s0);

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float y = texY.Sample(samp, input.uv).r;
    float u = texU.Sample(samp, input.uv).r - 0.5;
    float v = texV.Sample(samp, input.uv).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;

    return float4(r, g, b, 1.0);
}
)";

bool VideoRenderer::init(ID3D11Device* device, ID3D11DeviceContext* context) {
  m_device = device;
  m_context = context;
  m_d3d = new D3D11Resources();

  // 编译着色器
  ID3DBlob* vs_blob = nullptr;
  ID3DBlob* ps_blob = nullptr;
  ID3DBlob* err_blob = nullptr;

  HRESULT hr = D3DCompile(d3d11_vs_src, strlen(d3d11_vs_src), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0,
                          &vs_blob, &err_blob);
  if (FAILED(hr)) {
    LOG_ERROR("VideoRenderer", "Failed to compile VS: %s", err_blob ? (char*)err_blob->GetBufferPointer() : "unknown");
    if (err_blob) err_blob->Release();
    return false;
  }

  hr = D3DCompile(d3d11_ps_src, strlen(d3d11_ps_src), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &ps_blob,
                  &err_blob);
  if (FAILED(hr)) {
    LOG_ERROR("VideoRenderer", "Failed to compile PS: %s", err_blob ? (char*)err_blob->GetBufferPointer() : "unknown");
    if (err_blob) err_blob->Release();
    vs_blob->Release();
    return false;
  }

  m_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &m_d3d->vs);
  m_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &m_d3d->ps);

  // 输入布局
  D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
      {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
      {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
  };
  m_device->CreateInputLayout(layout_desc, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &m_d3d->layout);

  vs_blob->Release();
  ps_blob->Release();

  // 全屏四边形顶点
  float vertices[] = {
      // pos        uv
      -1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f,
  };

  D3D11_BUFFER_DESC vb_desc = {};
  vb_desc.ByteWidth = sizeof(vertices);
  vb_desc.Usage = D3D11_USAGE_DEFAULT;
  vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;

  D3D11_SUBRESOURCE_DATA vb_data = {};
  vb_data.pSysMem = vertices;
  m_device->CreateBuffer(&vb_desc, &vb_data, &m_d3d->vb);

  // 采样器
  D3D11_SAMPLER_DESC samp_desc = {};
  samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
  samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
  samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
  samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
  m_device->CreateSamplerState(&samp_desc, &m_d3d->sampler);

  LOG_INFO("VideoRenderer", "D3D11 video renderer initialized");
  return true;
}

void VideoRenderer::create_or_resize_d3d11(int width, int height) {
  if (m_d3d->alloc_w == width && m_d3d->alloc_h == height) return;

  // 释放旧纹理
  if (m_d3d->srv_y) {
    m_d3d->srv_y->Release();
    m_d3d->srv_y = nullptr;
  }
  if (m_d3d->srv_u) {
    m_d3d->srv_u->Release();
    m_d3d->srv_u = nullptr;
  }
  if (m_d3d->srv_v) {
    m_d3d->srv_v->Release();
    m_d3d->srv_v = nullptr;
  }
  if (m_d3d->srv_rgba) {
    m_d3d->srv_rgba->Release();
    m_d3d->srv_rgba = nullptr;
  }
  if (m_d3d->rtv) {
    m_d3d->rtv->Release();
    m_d3d->rtv = nullptr;
  }
  if (m_d3d->tex_y) {
    m_d3d->tex_y->Release();
    m_d3d->tex_y = nullptr;
  }
  if (m_d3d->tex_u) {
    m_d3d->tex_u->Release();
    m_d3d->tex_u = nullptr;
  }
  if (m_d3d->tex_v) {
    m_d3d->tex_v->Release();
    m_d3d->tex_v = nullptr;
  }
  if (m_d3d->tex_rgba) {
    m_d3d->tex_rgba->Release();
    m_d3d->tex_rgba = nullptr;
  }

  // 创建 Y 纹理 (R8, full resolution)
  D3D11_TEXTURE2D_DESC tex_desc = {};
  tex_desc.Width = width;
  tex_desc.Height = height;
  tex_desc.MipLevels = 1;
  tex_desc.ArraySize = 1;
  tex_desc.Format = DXGI_FORMAT_R8_UNORM;
  tex_desc.SampleDesc.Count = 1;
  tex_desc.Usage = D3D11_USAGE_DYNAMIC;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
  tex_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

  m_device->CreateTexture2D(&tex_desc, nullptr, &m_d3d->tex_y);

  // U 纹理 (R8, half resolution)
  tex_desc.Width = width / 2;
  tex_desc.Height = height / 2;
  m_device->CreateTexture2D(&tex_desc, nullptr, &m_d3d->tex_u);

  // V 纹理 (R8, half resolution)
  m_device->CreateTexture2D(&tex_desc, nullptr, &m_d3d->tex_v);

  // RGBA 输出纹理
  tex_desc.Width = width;
  tex_desc.Height = height;
  tex_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  tex_desc.Usage = D3D11_USAGE_DEFAULT;
  tex_desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
  tex_desc.CPUAccessFlags = 0;
  m_device->CreateTexture2D(&tex_desc, nullptr, &m_d3d->tex_rgba);

  // 创建 SRV
  D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
  srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
  srv_desc.Texture2D.MipLevels = 1;

  srv_desc.Format = DXGI_FORMAT_R8_UNORM;
  m_device->CreateShaderResourceView(m_d3d->tex_y, &srv_desc, &m_d3d->srv_y);
  m_device->CreateShaderResourceView(m_d3d->tex_u, &srv_desc, &m_d3d->srv_u);
  m_device->CreateShaderResourceView(m_d3d->tex_v, &srv_desc, &m_d3d->srv_v);

  srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  m_device->CreateShaderResourceView(m_d3d->tex_rgba, &srv_desc, &m_d3d->srv_rgba);

  // 创建 RTV
  m_device->CreateRenderTargetView(m_d3d->tex_rgba, nullptr, &m_d3d->rtv);

  m_d3d->alloc_w = width;
  m_d3d->alloc_h = height;
  m_srv = m_d3d->srv_rgba;

  LOG_INFO("VideoRenderer", "D3D11 textures created: %dx%d", width, height);
}

static void upload_r8_texture_d3d11(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, const uint8_t* data, int stride,
                                    int w, int h) {
  D3D11_MAPPED_SUBRESOURCE mapped;
  HRESULT hr = ctx->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped);
  if (FAILED(hr)) return;

  for (int row = 0; row < h; ++row) {
    memcpy((uint8_t*)mapped.pData + row * mapped.RowPitch, data + row * stride, w);
  }
  ctx->Unmap(tex, 0);
}

void VideoRenderer::upload_yuv_d3d11(const uint8_t* y, const uint8_t* u, const uint8_t* v, int sy, int su, int sv,
                                     int w, int h) {
  upload_r8_texture_d3d11(m_context, m_d3d->tex_y, y, sy, w, h);
  upload_r8_texture_d3d11(m_context, m_d3d->tex_u, u, su, w / 2, h / 2);
  upload_r8_texture_d3d11(m_context, m_d3d->tex_v, v, sv, w / 2, h / 2);

  // 渲染 YUV → RGBA
  m_context->OMSetRenderTargets(1, &m_d3d->rtv, nullptr);

  D3D11_VIEWPORT vp = {};
  vp.Width = (float)w;
  vp.Height = (float)h;
  vp.MaxDepth = 1.0f;
  m_context->RSSetViewports(1, &vp);

  m_context->IASetInputLayout(m_d3d->layout);
  UINT stride_val = sizeof(float) * 4;
  UINT offset = 0;
  m_context->IASetVertexBuffers(0, 1, &m_d3d->vb, &stride_val, &offset);
  m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

  m_context->VSSetShader(m_d3d->vs, nullptr, 0);
  m_context->PSSetShader(m_d3d->ps, nullptr, 0);

  ID3D11ShaderResourceView* srvs[] = {m_d3d->srv_y, m_d3d->srv_u, m_d3d->srv_v};
  m_context->PSSetShaderResources(0, 3, srvs);
  m_context->PSSetSamplers(0, 1, &m_d3d->sampler);

  m_context->Draw(4, 0);

  // 清理绑定
  ID3D11ShaderResourceView* null_srvs[3] = {};
  m_context->PSSetShaderResources(0, 3, null_srvs);
  ID3D11RenderTargetView* null_rtv = nullptr;
  m_context->OMSetRenderTargets(1, &null_rtv, nullptr);
}

void VideoRenderer::upload_frame(const uint8_t* data_y, const uint8_t* data_u, const uint8_t* data_v, int stride_y,
                                 int stride_u, int stride_v, int width, int height) {
  if (!m_device || !m_context || !m_d3d) return;
  create_or_resize_d3d11(width, height);
  upload_yuv_d3d11(data_y, data_u, data_v, stride_y, stride_u, stride_v, width, height);
  m_width = width;
  m_height = height;
  m_has_frame = true;
}

void* VideoRenderer::get_texture_id() const { return (void*)m_srv; }

void VideoRenderer::destroy() {
  if (m_d3d) {
    if (m_d3d->srv_y) m_d3d->srv_y->Release();
    if (m_d3d->srv_u) m_d3d->srv_u->Release();
    if (m_d3d->srv_v) m_d3d->srv_v->Release();
    if (m_d3d->srv_rgba) m_d3d->srv_rgba->Release();
    if (m_d3d->rtv) m_d3d->rtv->Release();
    if (m_d3d->tex_y) m_d3d->tex_y->Release();
    if (m_d3d->tex_u) m_d3d->tex_u->Release();
    if (m_d3d->tex_v) m_d3d->tex_v->Release();
    if (m_d3d->tex_rgba) m_d3d->tex_rgba->Release();
    if (m_d3d->vs) m_d3d->vs->Release();
    if (m_d3d->ps) m_d3d->ps->Release();
    if (m_d3d->sampler) m_d3d->sampler->Release();
    if (m_d3d->vb) m_d3d->vb->Release();
    if (m_d3d->layout) m_d3d->layout->Release();
    delete m_d3d;
    m_d3d = nullptr;
  }
  m_srv = nullptr;
  m_has_frame = false;
}

// =============================================================================
// OpenGL 实现
// =============================================================================
#else

#  if defined(__APPLE__)
#    include <OpenGL/gl3.h>
#  else
#    include <GL/gl.h>
#    include <GL/glext.h>
#  endif

static const char* gl_vert_src = R"(
#version 330 core
layout(location = 0) in vec2 aPos;
layout(location = 1) in vec2 aTexCoord;
out vec2 vTexCoord;
void main() {
    gl_Position = vec4(aPos, 0.0, 1.0);
    vTexCoord = aTexCoord;
}
)";

static const char* gl_frag_src = R"(
#version 330 core
in vec2 vTexCoord;
out vec4 outColor;
uniform sampler2D texY;
uniform sampler2D texU;
uniform sampler2D texV;
void main() {
    float y = texture(texY, vTexCoord).r;
    float u = texture(texU, vTexCoord).r - 0.5;
    float v = texture(texV, vTexCoord).r - 0.5;
    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;
    outColor = vec4(r, g, b, 1.0);
}
)";

bool VideoRenderer::init() {
  m_shader = compile_shader(gl_vert_src, gl_frag_src);
  if (!m_shader) {
    LOG_ERROR("VideoRenderer", "Failed to compile OpenGL shaders");
    return false;
  }

  // 全屏四边形
  float vertices[] = {
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

  LOG_INFO("VideoRenderer", "OpenGL video renderer initialized");
  return true;
}

unsigned int VideoRenderer::compile_shader(const char* vert_src, const char* frag_src) {
  unsigned int vs = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vs, 1, &vert_src, nullptr);
  glCompileShader(vs);
  int success;
  glGetShaderiv(vs, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(vs, 512, nullptr, log);
    LOG_ERROR("VideoRenderer", "Vertex shader error: %s", log);
    glDeleteShader(vs);
    return 0;
  }

  unsigned int fs = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fs, 1, &frag_src, nullptr);
  glCompileShader(fs);
  glGetShaderiv(fs, GL_COMPILE_STATUS, &success);
  if (!success) {
    char log[512];
    glGetShaderInfoLog(fs, 512, nullptr, log);
    LOG_ERROR("VideoRenderer", "Fragment shader error: %s", log);
    glDeleteShader(vs);
    glDeleteShader(fs);
    return 0;
  }

  unsigned int program = glCreateProgram();
  glAttachShader(program, vs);
  glAttachShader(program, fs);
  glLinkProgram(program);
  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    char log[512];
    glGetProgramInfoLog(program, 512, nullptr, log);
    LOG_ERROR("VideoRenderer", "Shader link error: %s", log);
    glDeleteProgram(program);
    program = 0;
  }

  glDeleteShader(vs);
  glDeleteShader(fs);
  return program;
}

static unsigned int create_r8_texture(int w, int h) {
  unsigned int tex;
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

void VideoRenderer::create_or_resize_gl(int width, int height) {
  if (m_tex_y && m_width == width && m_height == height) return;

  // 删除旧资源
  if (m_tex_y) {
    glDeleteTextures(1, &m_tex_y);
    m_tex_y = 0;
  }
  if (m_tex_u) {
    glDeleteTextures(1, &m_tex_u);
    m_tex_u = 0;
  }
  if (m_tex_v) {
    glDeleteTextures(1, &m_tex_v);
    m_tex_v = 0;
  }
  if (m_rgba_tex) {
    glDeleteTextures(1, &m_rgba_tex);
    m_rgba_tex = 0;
  }
  if (m_fbo) {
    glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
  }

  // Y/U/V 纹理
  m_tex_y = create_r8_texture(width, height);
  m_tex_u = create_r8_texture(width / 2, height / 2);
  m_tex_v = create_r8_texture(width / 2, height / 2);

  // RGBA 输出纹理
  glGenTextures(1, &m_rgba_tex);
  glBindTexture(GL_TEXTURE_2D, m_rgba_tex);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

  // FBO
  glGenFramebuffers(1, &m_fbo);
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, m_rgba_tex, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  LOG_INFO("VideoRenderer", "OpenGL textures created: %dx%d", width, height);
}

void VideoRenderer::upload_yuv_gl(const uint8_t* y, const uint8_t* u, const uint8_t* v, int sy, int su, int sv, int w,
                                  int h) {
  glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, sy);
  glBindTexture(GL_TEXTURE_2D, m_tex_y);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RED, GL_UNSIGNED_BYTE, y);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, su);
  glBindTexture(GL_TEXTURE_2D, m_tex_u);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, u);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, sv);
  glBindTexture(GL_TEXTURE_2D, m_tex_v);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w / 2, h / 2, GL_RED, GL_UNSIGNED_BYTE, v);

  glPixelStorei(GL_UNPACK_ROW_LENGTH, 0);

  // 保存当前状态
  GLint prev_fbo, prev_viewport[4], prev_program;
  glGetIntegerv(GL_FRAMEBUFFER_BINDING, &prev_fbo);
  glGetIntegerv(GL_VIEWPORT, prev_viewport);
  glGetIntegerv(GL_CURRENT_PROGRAM, &prev_program);

  // 渲染 YUV → RGBA
  glBindFramebuffer(GL_FRAMEBUFFER, m_fbo);
  glViewport(0, 0, w, h);
  glUseProgram(m_shader);

  glActiveTexture(GL_TEXTURE0);
  glBindTexture(GL_TEXTURE_2D, m_tex_y);
  glUniform1i(glGetUniformLocation(m_shader, "texY"), 0);

  glActiveTexture(GL_TEXTURE1);
  glBindTexture(GL_TEXTURE_2D, m_tex_u);
  glUniform1i(glGetUniformLocation(m_shader, "texU"), 1);

  glActiveTexture(GL_TEXTURE2);
  glBindTexture(GL_TEXTURE_2D, m_tex_v);
  glUniform1i(glGetUniformLocation(m_shader, "texV"), 2);

  glBindVertexArray(m_vao);
  glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
  glBindVertexArray(0);

  // 恢复状态
  glBindFramebuffer(GL_FRAMEBUFFER, prev_fbo);
  glViewport(prev_viewport[0], prev_viewport[1], prev_viewport[2], prev_viewport[3]);
  glUseProgram(prev_program);
}

void VideoRenderer::upload_frame(const uint8_t* data_y, const uint8_t* data_u, const uint8_t* data_v, int stride_y,
                                 int stride_u, int stride_v, int width, int height) {
  if (!m_shader) return;
  create_or_resize_gl(width, height);
  upload_yuv_gl(data_y, data_u, data_v, stride_y, stride_u, stride_v, width, height);
  m_width = width;
  m_height = height;
  m_has_frame = true;
}

void* VideoRenderer::get_texture_id() const { return (void*)(intptr_t)m_rgba_tex; }

void VideoRenderer::destroy() {
  if (m_tex_y) {
    glDeleteTextures(1, &m_tex_y);
    m_tex_y = 0;
  }
  if (m_tex_u) {
    glDeleteTextures(1, &m_tex_u);
    m_tex_u = 0;
  }
  if (m_tex_v) {
    glDeleteTextures(1, &m_tex_v);
    m_tex_v = 0;
  }
  if (m_rgba_tex) {
    glDeleteTextures(1, &m_rgba_tex);
    m_rgba_tex = 0;
  }
  if (m_fbo) {
    glDeleteFramebuffers(1, &m_fbo);
    m_fbo = 0;
  }
  if (m_shader) {
    glDeleteProgram(m_shader);
    m_shader = 0;
  }
  if (m_vao) {
    glDeleteVertexArrays(1, &m_vao);
    m_vao = 0;
  }
  if (m_vbo) {
    glDeleteBuffers(1, &m_vbo);
    m_vbo = 0;
  }
  m_has_frame = false;
}

#endif  // RENDERER_D3D11 / RENDERER_OPENGL
