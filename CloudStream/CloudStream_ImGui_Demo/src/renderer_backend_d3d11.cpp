// renderer_backend_d3d11.cpp - D3D11 渲染后端（Windows）
//
// 整个文件由平台宏守卫：非 Windows 构建时本文件会被编进去但内容为空。
//
// 结构与 renderer_backend_gl.cpp 严格对应（共享资源 + 每路一张输出纹理），
// 这样两个平台的优化思路可以互相参照。
//
// 与 GL 的两处差异不是风格问题，照抄 GL 会出错：
//
// 1. 暂存纹理的写入方式。GL 的 glTexSubImage2D 只写有效子矩形，纹理之后可以
//    继续当 shader 输入；D3D11 若用 Map(WRITE_DISCARD)，丢的是**整块**内容，
//    之后再写一个小帧，子区域外就是未定义值，采样到会黑屏或错位。一帧内 500 路
//    又轮流复用同一张暂存纹理，所以这里用 USAGE_DEFAULT + UpdateSubresource，
//    每次写入的就是本帧完整有效区域，内容不依赖上一位使用者。
//
// 2. 上传与绘制必须在同一次调用内完成（upload_i420 内部就是 上传→Draw）。
//    上层按「每实例最多保留一帧」喂帧，采样时刻与上传时刻不重合，暂存纹理若跨越
//    调用边界就会被后续的流覆盖。这一点与 GL 一致，但要清楚为什么。
//
// 硬解路径：D3D11 VideoProcessor 输出 BGRA8，NV12 是双平面。两者都能直接当
// SRV 采样做缩放，不需要额外的转换 pass。

#if defined(RENDERER_D3D11)

#  include <d3d11.h>
#  include <d3dcompiler.h>

#  include <cstdint>
#  include <cstring>

#  include "logger.h"
#  include "renderer_backend.h"

namespace {

const char* kVsSrc = R"(
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

// I420 -> RGBA，BT.601 全范围系数，与 GL 后端保持一致
const char* kPsI420Src = R"(
Texture2D texY : register(t0);
Texture2D texU : register(t1);
Texture2D texV : register(t2);
SamplerState samp : register(s0);

// 共享暂存纹理按「历史最大帧」分配，本帧可能只占左上角一块。
// uvScale 把采样限制在有效区域内，否则会采到上一路残留的像素。
cbuffer Params : register(b0) {
    float2 uvScale;
    float2 _pad;
};

struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};

float4 main(PS_INPUT input) : SV_TARGET {
    float2 uv = input.uv * uvScale;
    float y = texY.Sample(samp, uv).r;
    float u = texU.Sample(samp, uv).r - 0.5;
    float v = texV.Sample(samp, uv).r - 0.5;

    float r = y + 1.402 * v;
    float g = y - 0.344136 * u - 0.714136 * v;
    float b = y + 1.772 * u;

    return float4(r, g, b, 1.0);
}
)";

// 硬解 BGRA 源纹理（D3D11 VideoProcessor 的输出）-> 输出纹理。
// 当 RGBA 采样会红蓝互换，必须显式调换通道。
const char* kPsBgraSrc = R"(
Texture2D texSrc : register(t0);
SamplerState samp : register(s0);
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(PS_INPUT input) : SV_TARGET {
    float4 c = texSrc.Sample(samp, input.uv);
    return float4(c.b, c.g, c.r, 1.0);
}
)";

// 硬解 NV12 双平面源纹理 -> RGBA（BT.709 video-range，对应解码器输出）
const char* kPsNv12Src = R"(
Texture2D texY : register(t0);
Texture2D texUV : register(t1);
SamplerState samp : register(s0);
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(PS_INPUT input) : SV_TARGET {
    float y = (texY.Sample(samp, input.uv).r - 0.0625) * 1.164383;
    float2 uv = texUV.Sample(samp, input.uv).rg - float2(0.5, 0.5);
    float r = y + 1.792741 * uv.y;
    float g = y - 0.213249 * uv.x - 0.532909 * uv.y;
    float b = y + 2.112402 * uv.x;
    return float4(r, g, b, 1.0);
}
)";

// sample_luma 的缩小 pass
const char* kPsCopySrc = R"(
Texture2D texSrc : register(t0);
SamplerState samp : register(s0);
struct PS_INPUT {
    float4 pos : SV_POSITION;
    float2 uv  : TEXCOORD0;
};
float4 main(PS_INPUT input) : SV_TARGET { return texSrc.Sample(samp, input.uv); }
)";

template <typename T>
void safe_release(T** p) {
  if (*p) {
    (*p)->Release();
    *p = nullptr;
  }
}

// 常量缓冲大小必须是 16 字节的倍数
struct CbParams {
  float uv_scale[2];
  float pad[2];
};

struct OutputD3D {
  ID3D11Texture2D* tex = nullptr;
  ID3D11ShaderResourceView* srv = nullptr;
  ID3D11RenderTargetView* rtv = nullptr;
};

const int kLumaSize = 64;

class D3D11Backend : public RendererBackend {
 public:
  D3D11Backend(ID3D11Device* device, ID3D11DeviceContext* context) : m_device(device), m_context(context) {
    if (m_device) m_device->AddRef();
    if (m_context) m_context->AddRef();
  }

  ~D3D11Backend() override {
    destroy_shared();
    safe_release(&m_luma_rtv);
    safe_release(&m_luma_tex);
    safe_release(&m_luma_staging);
    safe_release(&m_device);
    safe_release(&m_context);
  }

  // ===========================================================================
  // 共享资源：进程内一份
  // ===========================================================================
  bool init_shared() override {
    if (!m_device || !m_context) {
      LOG_ERROR("D3D11Backend", "null device/context");
      return false;
    }

    ID3DBlob* vs_blob = nullptr;
    if (!compile_shader(kVsSrc, "vs_5_0", &vs_blob, nullptr)) return false;
    if (!compile_shader(kPsI420Src, "ps_5_0", nullptr, &m_ps_i420)) {
      safe_release(&vs_blob);
      return false;
    }
    // 硬解与回读着色器失败不阻塞软解
    if (!compile_shader(kPsBgraSrc, "ps_5_0", nullptr, &m_ps_bgra))
      LOG_WARN("D3D11Backend", "BGRA shader unavailable, hardware decode rendering disabled");
    if (!compile_shader(kPsNv12Src, "ps_5_0", nullptr, &m_ps_nv12))
      LOG_WARN("D3D11Backend", "NV12 shader unavailable, hardware decode rendering disabled");
    if (!compile_shader(kPsCopySrc, "ps_5_0", nullptr, &m_ps_copy))
      LOG_WARN("D3D11Backend", "copy shader unavailable, sample_luma disabled");

    // 输入布局只能由 VS 字节码创建，所以保留 blob 直到这里
    HRESULT hr = m_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &m_vs);
    const D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    hr |= m_device->CreateInputLayout(layout_desc, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &m_layout);
    safe_release(&vs_blob);

    const float vertices[] = {
        // pos        uv
        -1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f,
    };
    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.Usage = D3D11_USAGE_IMMUTABLE;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vb_data = {};
    vb_data.pSysMem = vertices;
    hr |= m_device->CreateBuffer(&vb_desc, &vb_data, &m_vb);

    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.ByteWidth = sizeof(CbParams);  // 16 字节对齐，否则 CreateBuffer 直接失败
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    hr |= m_device->CreateBuffer(&cb_desc, nullptr, &m_cb);

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.MaxLOD = D3D11_FLOAT32_MAX;
    hr |= m_device->CreateSamplerState(&samp_desc, &m_sampler);

    if (FAILED(hr) || !m_vs || !m_layout || !m_vb || !m_cb || !m_sampler) {
      LOG_ERROR("D3D11Backend", "failed to create shared resources: 0x%08lx", (unsigned long)hr);
      return false;
    }

    ensure_luma_resources();
    LOG_INFO("D3D11Backend", "shared resources initialized");
    return true;
  }

  void destroy_shared() override {
    safe_release(&m_scratch_y);
    safe_release(&m_scratch_uv);
    safe_release(&m_srv_y);
    safe_release(&m_srv_uv);
    safe_release(&m_sampler);
    safe_release(&m_cb);
    safe_release(&m_vb);
    safe_release(&m_layout);
    safe_release(&m_ps_i420);
    safe_release(&m_ps_bgra);
    safe_release(&m_ps_nv12);
    safe_release(&m_ps_copy);
    safe_release(&m_vs);
    m_scratch_w = 0;
    m_scratch_h = 0;
    m_staging.clear();
  }

  // ===========================================================================
  // 每路独占：只有一张输出纹理和它的 SRV/RTV
  // ===========================================================================
  bool create_output(int w, int h, OutputTexture* out) override {
    if (!m_device || w <= 0 || h <= 0) return false;

    OutputD3D* o = new OutputD3D();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = (UINT)w;
    desc.Height = (UINT)h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &o->tex);

    // 视图都用 nullptr 描述符：格式取自纹理自身，避免手写描述符与纹理不一致
    if (SUCCEEDED(hr)) hr = m_device->CreateShaderResourceView(o->tex, nullptr, &o->srv);
    if (SUCCEEDED(hr)) hr = m_device->CreateRenderTargetView(o->tex, nullptr, &o->rtv);
    if (FAILED(hr)) {
      safe_release(&o->rtv);
      safe_release(&o->srv);
      safe_release(&o->tex);
      delete o;
      return false;
    }

    out->opaque = o;
    out->ui_handle = o->srv;  // 直接交给 ImGui::Image()
    out->width = w;
    out->height = h;
    return true;
  }

  void destroy_output(OutputTexture* out) override {
    OutputD3D* o = (OutputD3D*)out->opaque;
    if (o) {
      safe_release(&o->rtv);
      safe_release(&o->srv);
      safe_release(&o->tex);
      delete o;
    }
    *out = OutputTexture();
  }

  // ===========================================================================
  // I420 软解路径
  // ===========================================================================
  bool upload_i420(const I420Frame& src, OutputTexture* dst) override {
    OutputD3D* o = dst ? (OutputD3D*)dst->opaque : nullptr;
    if (!o || !m_ps_i420 || !src.data_y || src.width <= 0 || src.height <= 0) return false;
    if (!ensure_scratch(src.width, src.height)) return false;

    const int cw = src.width / 2;
    const int chh = src.height / 2;
    if (!update_plane(src.data_y, src.stride_y, src.width, src.height)) return false;
    if (!update_uv_plane(src.data_u, src.stride_u, src.data_v, src.stride_v, cw, chh)) return false;

    // 暂存纹理按历史最大值分配，本帧可能只占左上角，用 uvScale 限制采样范围
    if (!set_uv_scale((float)src.width / (float)m_scratch_w, (float)src.height / (float)m_scratch_h)) return false;

    ID3D11ShaderResourceView* srvs[2] = {m_srv_y, m_srv_uv};
    draw_to(o, m_ps_i420, srvs, 2, true);
    return true;
  }

  // ===========================================================================
  // 硬解零拷贝路径：源就是解码器给出的 D3D11 纹理
  // ===========================================================================
  bool upload_gpu(const TcrGpuBuffer& src, OutputTexture* dst) override {
    OutputD3D* o = dst ? (OutputD3D*)dst->opaque : nullptr;
    if (!o || !m_device || !m_context) return false;
    if (src.platform != TCR_GPU_PLATFORM_D3D11 || !src.handle) return false;

    bool nv12 = (src.pixel_format == TCR_GPU_PIXEL_FORMAT_NV12);
    ID3D11PixelShader* ps = nv12 ? m_ps_nv12 : m_ps_bgra;
    if (!ps) return false;

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    switch (src.pixel_format) {
      case TCR_GPU_PIXEL_FORMAT_NV12:
        sd.Format = DXGI_FORMAT_NV12;
        break;
      case TCR_GPU_PIXEL_FORMAT_BGRA8:
        sd.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
        break;
      case TCR_GPU_PIXEL_FORMAT_RGBA8:
        sd.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
        break;
      default:
        return false;
    }

    // 这张纹理可能不是本 device 创建的（客户端自己的解码器），跨 device 需要标准
    // shared handle，无法在此判断；CreateShaderResourceView 失败即为不可用。
    ID3D11ShaderResourceView* srv = nullptr;
    HRESULT hr = m_device->CreateShaderResourceView((ID3D11Texture2D*)src.handle, &sd, &srv);
    if (FAILED(hr)) {
      static bool warned = false;
      if (!warned) {
        warned = true;
        LOG_WARN("D3D11Backend", "SRV on decode texture failed: 0x%08lx (cross-device texture?)", (unsigned long)hr);
      }
      return false;
    }

    ID3D11ShaderResourceView* srvs[2] = {srv, nullptr};
    draw_to(o, ps, srvs, nv12 ? 2 : 1, false);
    srv->Release();
    return true;
  }

  // ===========================================================================
  // 画面正确性回读
  //
  // 帧计数只能说明「收到了帧」，黑屏和错位时它照样正常。这里把输出纹理缩到
  // 固定 64x64 再拷进 STAGING，回读开销与纹理尺寸无关。
  // ===========================================================================
  int sample_luma(const OutputTexture& tex) override {
    OutputD3D* o = (OutputD3D*)tex.opaque;
    if (!o || !o->srv || !m_ps_copy || tex.width <= 0 || tex.height <= 0) return -1;
    if (!ensure_luma_resources()) return -1;

    const int sw = tex.width < kLumaSize ? tex.width : kLumaSize;
    const int sh = tex.height < kLumaSize ? tex.height : kLumaSize;

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)kLumaSize;  // 整张 luma 纹理，缩放在采样时由线性过滤完成
    vp.Height = (float)kLumaSize;
    vp.MaxDepth = 1.0f;
    m_context->OMSetRenderTargets(1, &m_luma_rtv, nullptr);
    const float clear[4] = {0.f, 0.f, 0.f, 1.f};
    m_context->ClearRenderTargetView(m_luma_rtv, clear);
    m_context->RSSetViewports(1, &vp);
    bind_pipeline();
    m_context->PSSetShader(m_ps_copy, nullptr, 0);
    m_context->PSSetShaderResources(0, 1, &o->srv);
    m_context->PSSetSamplers(0, 1, &m_sampler);
    m_context->Draw(4, 0);
    unbind_output();

    // 只拷有效像素：源可能比 64x64 小
    D3D11_BOX box = {};
    box.right = (UINT)sw;
    box.bottom = (UINT)sh;
    box.back = 1;
    m_context->CopySubresourceRegion(m_luma_staging, 0, 0, 0, 0, m_luma_tex, 0, &box);

    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(m_context->Map(m_luma_staging, 0, D3D11_MAP_READ, 0, &mapped))) return -1;

    uint64_t sum = 0;
    for (int row = 0; row < sh; ++row) {
      const uint8_t* px = (const uint8_t*)mapped.pData + (size_t)row * mapped.RowPitch;
      for (int col = 0; col < sw; ++col, px += 4) {
        sum += (uint64_t)((px[0] * 299 + px[1] * 587 + px[2] * 114) / 1000);
      }
    }
    m_context->Unmap(m_luma_staging, 0);
    return (int)(sum / (uint64_t)(sw * sh));
  }

 private:
  bool compile_shader(const char* src, const char* profile, ID3DBlob** vs_blob, ID3D11PixelShader** ps_out) {
    ID3DBlob* blob = nullptr;
    ID3DBlob* err = nullptr;
    HRESULT hr = D3DCompile(src, strlen(src), nullptr, nullptr, nullptr, "main", profile, 0, 0, &blob, &err);
    if (FAILED(hr)) {
      LOG_ERROR("D3D11Backend", "%s compile failed: %s", profile, err ? (char*)err->GetBufferPointer() : "unknown");
      safe_release(&err);
      return false;
    }
    if (vs_blob) {
      *vs_blob = blob;
      return true;
    }
    hr = m_device->CreatePixelShader(blob->GetBufferPointer(), blob->GetBufferSize(), nullptr, ps_out);
    safe_release(&blob);
    if (FAILED(hr)) {
      LOG_ERROR("D3D11Backend", "CreatePixelShader(%s) failed: 0x%08lx", profile, (unsigned long)hr);
      return false;
    }
    return true;
  }

  void bind_pipeline() {
    m_context->IASetInputLayout(m_layout);
    UINT stride = sizeof(float) * 4;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);
    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetSamplers(0, 1, &m_sampler);
  }

  bool set_uv_scale(float sx, float sy) {
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (FAILED(m_context->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return false;
    CbParams* p = (CbParams*)mapped.pData;
    p->uv_scale[0] = sx;
    p->uv_scale[1] = sy;
    p->pad[0] = 0.f;
    p->pad[1] = 0.f;
    m_context->Unmap(m_cb, 0);
    return true;
  }

  // 上传完必须解绑 RTV/SRV：ImGui 紧接着要用自己的 RTV 和纹理绘制，
  // 我们绑着的不释放会让它拿到错的资源，表现为整窗黑屏或画面错位。
  void unbind_output() {
    ID3D11ShaderResourceView* null_srvs[2] = {nullptr, nullptr};
    m_context->PSSetShaderResources(0, 2, null_srvs);
    ID3D11RenderTargetView* null_rtv = nullptr;
    m_context->OMSetRenderTargets(1, &null_rtv, nullptr);
  }

  void draw_to(OutputD3D* dst, ID3D11PixelShader* ps, ID3D11ShaderResourceView** srvs, int n, bool use_cb) {
    D3D11_VIEWPORT vp = {};
    vp.Width = (float)(dst->width > 0 ? dst->width : 1);
    vp.Height = (float)(dst->height > 0 ? dst->height : 1);
    vp.MaxDepth = 1.0f;

    m_context->OMSetRenderTargets(1, &dst->rtv, nullptr);
    m_context->RSSetViewports(1, &vp);
    bind_pipeline();
    m_context->PSSetShader(ps, nullptr, 0);
    m_context->PSSetShaderResources(0, (UINT)n, srvs);

    ID3D11Buffer* cb = use_cb ? m_cb : nullptr;
    m_context->PSSetConstantBuffers(0, 1, &cb);

    m_context->Draw(4, 0);
    unbind_output();

    // 像素着色器按寄存器保留最后一次绑定值，解绑常量缓冲以免干扰后续绘制
    ID3D11Buffer* null_cb = nullptr;
    m_context->PSSetConstantBuffers(0, 1, &null_cb);
  }

  bool ensure_luma_resources() {
    if (m_luma_tex && m_luma_rtv && m_luma_staging) return true;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = kLumaSize;
    desc.Height = kLumaSize;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_RENDER_TARGET | D3D11_BIND_SHADER_RESOURCE;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &m_luma_tex))) return false;
    if (FAILED(m_device->CreateRenderTargetView(m_luma_tex, nullptr, &m_luma_rtv))) return false;

    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &m_luma_staging))) return false;
    return true;
  }

  // 暂存纹理按「见过的最大帧尺寸」增长，只增不减，所有流轮流复用。
  // Y 平面单独一张 R8；U/V 合成一张 RG8（U 在 R，V 在 G），少一次采样槽和一次拷贝。
  // 用 USAGE_DEFAULT：UpdateSubresource 只写指定子矩形，子区域外的内容不会被丢掉。
  bool ensure_scratch(int w, int h) {
    if (w <= m_scratch_w && h <= m_scratch_h) return true;
    const int nw = w > m_scratch_w ? w : m_scratch_w;
    const int nh = h > m_scratch_h ? h : m_scratch_h;

    safe_release(&m_srv_y);
    safe_release(&m_srv_uv);
    safe_release(&m_scratch_y);
    safe_release(&m_scratch_uv);
    m_scratch_w = 0;
    m_scratch_h = 0;

    D3D11_TEXTURE2D_DESC desc = {};
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.Width = (UINT)nw;
    desc.Height = (UINT)nh;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_scratch_y);

    desc.Width = (UINT)(nw / 2);
    desc.Height = (UINT)(nh / 2);
    desc.Format = DXGI_FORMAT_R8G8_UNORM;
    hr |= m_device->CreateTexture2D(&desc, nullptr, &m_scratch_uv);
    if (FAILED(hr) || !m_scratch_y || !m_scratch_uv) {
      LOG_ERROR("D3D11Backend", "failed to create scratch textures %dx%d: 0x%08lx", nw, nh, (unsigned long)hr);
      return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC sd = {};
    sd.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    sd.Texture2D.MipLevels = 1;
    sd.Format = DXGI_FORMAT_R8_UNORM;
    hr = m_device->CreateShaderResourceView(m_scratch_y, &sd, &m_srv_y);
    sd.Format = DXGI_FORMAT_R8G8_UNORM;
    hr |= m_device->CreateShaderResourceView(m_scratch_uv, &sd, &m_srv_uv);
    if (FAILED(hr) || !m_srv_y || !m_srv_uv) {
      LOG_ERROR("D3D11Backend", "failed to create scratch SRVs: 0x%08lx", (unsigned long)hr);
      return false;
    }

    m_scratch_w = nw;
    m_scratch_h = nh;
    LOG_INFO("D3D11Backend", "shared YUV scratch resized to %dx%d", nw, nh);
    return true;
  }

  // 把一块 R8 平面写进暂存 Y 纹理的左上角 w x h。
  // 源 stride 可能大于 w：除了更新接口本身的 row pitch 参数之外没有别的办法
  // 在 CPU 侧跳行，所以 stride != w 时先按行规整成紧密排布。
  bool update_plane(const uint8_t* data, int stride, int w, int h) {
    if (!data || w <= 0 || h <= 0) return false;

    D3D11_BOX box = {};
    box.right = (UINT)w;
    box.bottom = (UINT)h;
    box.back = 1;

    if (stride == w) {
      m_context->UpdateSubresource(m_scratch_y, 0, &box, data, (UINT)stride, 0);
      return true;
    }

    if (!ensure_staging(w, h, 1)) return false;
    const UINT pitch = (UINT)w;
    for (int row = 0; row < h; ++row) {
      memcpy(m_staging.data() + (size_t)row * pitch, data + (size_t)row * stride, (size_t)w);
    }
    m_context->UpdateSubresource(m_scratch_y, 0, &box, m_staging.data(), pitch, 0);
    return true;
  }

  // U/V 两个平面交错写入 RG8 暂存纹理：U 进 R，V 进 G。
  // D3D11 没有「只写某一通道」的更新接口，所以这里必然在 CPU 侧交错一次；
  // 换来的是一张纹理一次上传，着色器少一个采样槽。
  bool update_uv_plane(const uint8_t* u, int stride_u, const uint8_t* v, int stride_v, int w, int h) {
    if (!u || !v || w <= 0 || h <= 0) return false;
    if (!ensure_staging(w, h, 2)) return false;

    const UINT pitch = (UINT)(w * 2);
    for (int row = 0; row < h; ++row) {
      const uint8_t* su = u + (size_t)row * stride_u;
      const uint8_t* sv = v + (size_t)row * stride_v;
      uint8_t* dst = m_staging.data() + (size_t)row * pitch;
      for (int col = 0; col < w; ++col) {
        dst[col * 2] = su[col];
        dst[col * 2 + 1] = sv[col];
      }
    }

    D3D11_BOX box = {};
    box.right = (UINT)w;
    box.bottom = (UINT)h;
    box.back = 1;
    m_context->UpdateSubresource(m_scratch_uv, 0, &box, m_staging.data(), pitch, 0);
    return true;
  }

  bool ensure_staging(int w, int h, int bpp) {
    const size_t need = (size_t)w * h * bpp;
    if (m_staging.size() < need) m_staging.resize(need);
    return true;
  }

  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;

  ID3D11VertexShader* m_vs = nullptr;
  ID3D11PixelShader* m_ps_i420 = nullptr;
  ID3D11PixelShader* m_ps_bgra = nullptr;
  ID3D11PixelShader* m_ps_nv12 = nullptr;
  ID3D11PixelShader* m_ps_copy = nullptr;
  ID3D11InputLayout* m_layout = nullptr;
  ID3D11Buffer* m_vb = nullptr;
  ID3D11Buffer* m_cb = nullptr;
  ID3D11SamplerState* m_sampler = nullptr;

  ID3D11Texture2D* m_scratch_y = nullptr;
  ID3D11Texture2D* m_scratch_uv = nullptr;
  ID3D11ShaderResourceView* m_srv_y = nullptr;
  ID3D11ShaderResourceView* m_srv_uv = nullptr;
  int m_scratch_w = 0;
  int m_scratch_h = 0;

  ID3D11Texture2D* m_luma_tex = nullptr;
  ID3D11RenderTargetView* m_luma_rtv = nullptr;
  ID3D11Texture2D* m_luma_staging = nullptr;

  // 上传路径的行规整缓冲（只用于 stride != width 的少数情况）
  std::vector<uint8_t> m_staging;
};

}  // namespace

RendererBackend* create_renderer_backend(void* device, void* context) {
  return new D3D11Backend((ID3D11Device*)device, (ID3D11DeviceContext*)context);
}

#endif  // RENDERER_D3D11
