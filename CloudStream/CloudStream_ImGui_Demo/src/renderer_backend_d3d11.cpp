// renderer_backend_d3d11.cpp - D3D11 渲染后端（Windows）
//
// 整个文件由平台宏守卫：源文件是 GLOB 收集的，非 Windows 构建时本文件会被编进去但内容为空。
//
// 结构与 renderer_backend_gl.cpp 严格对应（共享资源 + 每路一张输出纹理），
// 这样两个平台的优化思路可以互相参照。

#if defined(RENDERER_D3D11)

#  include <d3d11.h>
#  include <d3dcompiler.h>

#  include <cstring>
#  include <vector>

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

const char* kPsSrc = R"(
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

template <typename T>
void safe_release(T** p) {
  if (*p) {
    (*p)->Release();
    *p = nullptr;
  }
}

// 每路输出纹理的私有数据
struct OutputD3D {
  ID3D11Texture2D* tex = nullptr;
  ID3D11ShaderResourceView* srv = nullptr;
  ID3D11RenderTargetView* rtv = nullptr;
};

void upload_r8(ID3D11DeviceContext* ctx, ID3D11Texture2D* tex, const uint8_t* data, int stride, int w, int h) {
  D3D11_MAPPED_SUBRESOURCE mapped;
  if (FAILED(ctx->Map(tex, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) return;
  for (int row = 0; row < h; ++row) {
    memcpy((uint8_t*)mapped.pData + (size_t)row * mapped.RowPitch, data + (size_t)row * stride, w);
  }
  ctx->Unmap(tex, 0);
}

class D3D11Backend : public RendererBackend {
 public:
  D3D11Backend(ID3D11Device* device, ID3D11DeviceContext* context) : m_device(device), m_context(context) {}

  bool init_shared() override {
    if (!m_device || !m_context) {
      LOG_ERROR("D3D11Backend", "null device/context");
      return false;
    }

    ID3DBlob* vs_blob = nullptr;
    ID3DBlob* ps_blob = nullptr;
    ID3DBlob* err_blob = nullptr;

    HRESULT hr =
        D3DCompile(kVsSrc, strlen(kVsSrc), nullptr, nullptr, nullptr, "main", "vs_5_0", 0, 0, &vs_blob, &err_blob);
    if (FAILED(hr)) {
      LOG_ERROR("D3D11Backend", "Failed to compile VS: %s", err_blob ? (char*)err_blob->GetBufferPointer() : "unknown");
      safe_release(&err_blob);
      return false;
    }

    hr = D3DCompile(kPsSrc, strlen(kPsSrc), nullptr, nullptr, nullptr, "main", "ps_5_0", 0, 0, &ps_blob, &err_blob);
    if (FAILED(hr)) {
      LOG_ERROR("D3D11Backend", "Failed to compile PS: %s", err_blob ? (char*)err_blob->GetBufferPointer() : "unknown");
      safe_release(&err_blob);
      safe_release(&vs_blob);
      return false;
    }

    m_device->CreateVertexShader(vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), nullptr, &m_vs);
    m_device->CreatePixelShader(ps_blob->GetBufferPointer(), ps_blob->GetBufferSize(), nullptr, &m_ps);

    const D3D11_INPUT_ELEMENT_DESC layout_desc[] = {
        {"POSITION", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 0, D3D11_INPUT_PER_VERTEX_DATA, 0},
        {"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, sizeof(float) * 2, D3D11_INPUT_PER_VERTEX_DATA, 0},
    };
    m_device->CreateInputLayout(layout_desc, 2, vs_blob->GetBufferPointer(), vs_blob->GetBufferSize(), &m_layout);

    safe_release(&vs_blob);
    safe_release(&ps_blob);

    const float vertices[] = {
        // pos        uv
        -1.f, 1.f, 0.f, 0.f, -1.f, -1.f, 0.f, 1.f, 1.f, 1.f, 1.f, 0.f, 1.f, -1.f, 1.f, 1.f,
    };
    D3D11_BUFFER_DESC vb_desc = {};
    vb_desc.ByteWidth = sizeof(vertices);
    vb_desc.Usage = D3D11_USAGE_DEFAULT;
    vb_desc.BindFlags = D3D11_BIND_VERTEX_BUFFER;
    D3D11_SUBRESOURCE_DATA vb_data = {};
    vb_data.pSysMem = vertices;
    m_device->CreateBuffer(&vb_desc, &vb_data, &m_vb);

    // uvScale 常量缓冲（每次上传更新）
    D3D11_BUFFER_DESC cb_desc = {};
    cb_desc.ByteWidth = sizeof(float) * 4;  // 必须 16 字节对齐
    cb_desc.Usage = D3D11_USAGE_DYNAMIC;
    cb_desc.BindFlags = D3D11_BIND_CONSTANT_BUFFER;
    cb_desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;
    m_device->CreateBuffer(&cb_desc, nullptr, &m_cb);

    D3D11_SAMPLER_DESC samp_desc = {};
    samp_desc.Filter = D3D11_FILTER_MIN_MAG_MIP_LINEAR;
    samp_desc.AddressU = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressV = D3D11_TEXTURE_ADDRESS_CLAMP;
    samp_desc.AddressW = D3D11_TEXTURE_ADDRESS_CLAMP;
    m_device->CreateSamplerState(&samp_desc, &m_sampler);

    if (!m_vs || !m_ps || !m_layout || !m_vb || !m_cb || !m_sampler) {
      LOG_ERROR("D3D11Backend", "failed to create shared resources");
      return false;
    }

    LOG_INFO("D3D11Backend", "shared resources initialized");
    return true;
  }

  void destroy_shared() override {
    safe_release(&m_scratch_y);
    safe_release(&m_scratch_u);
    safe_release(&m_scratch_v);
    safe_release(&m_srv_y);
    safe_release(&m_srv_u);
    safe_release(&m_srv_v);
    safe_release(&m_sampler);
    safe_release(&m_cb);
    safe_release(&m_vb);
    safe_release(&m_layout);
    safe_release(&m_ps);
    safe_release(&m_vs);
    m_scratch_w = 0;
    m_scratch_h = 0;
  }

  bool create_output(int w, int h, OutputTexture* out) override {
    OutputD3D* o = new OutputD3D();

    D3D11_TEXTURE2D_DESC desc = {};
    desc.Width = w;
    desc.Height = h;
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DEFAULT;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE | D3D11_BIND_RENDER_TARGET;
    if (FAILED(m_device->CreateTexture2D(&desc, nullptr, &o->tex))) {
      delete o;
      return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
    if (FAILED(m_device->CreateShaderResourceView(o->tex, &srv_desc, &o->srv)) ||
        FAILED(m_device->CreateRenderTargetView(o->tex, nullptr, &o->rtv))) {
      safe_release(&o->rtv);
      safe_release(&o->srv);
      safe_release(&o->tex);
      delete o;
      return false;
    }

    out->opaque = o;
    out->ui_handle = o->srv;
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

  bool upload_i420(const I420Frame& src, OutputTexture* dst) override {
    OutputD3D* o = (OutputD3D*)dst->opaque;
    if (!o || !ensure_scratch(src.width, src.height)) return false;

    upload_r8(m_context, m_scratch_y, src.data_y, src.stride_y, src.width, src.height);
    upload_r8(m_context, m_scratch_u, src.data_u, src.stride_u, src.width / 2, src.height / 2);
    upload_r8(m_context, m_scratch_v, src.data_v, src.stride_v, src.width / 2, src.height / 2);

    // 暂存纹理比本帧大时，只采样左上角的有效区域
    D3D11_MAPPED_SUBRESOURCE mapped;
    if (SUCCEEDED(m_context->Map(m_cb, 0, D3D11_MAP_WRITE_DISCARD, 0, &mapped))) {
      float* p = (float*)mapped.pData;
      p[0] = (float)src.width / (float)m_scratch_w;
      p[1] = (float)src.height / (float)m_scratch_h;
      p[2] = 0.f;
      p[3] = 0.f;
      m_context->Unmap(m_cb, 0);
    }

    m_context->OMSetRenderTargets(1, &o->rtv, nullptr);

    D3D11_VIEWPORT vp = {};
    vp.Width = (float)dst->width;
    vp.Height = (float)dst->height;
    vp.MaxDepth = 1.0f;
    m_context->RSSetViewports(1, &vp);

    m_context->IASetInputLayout(m_layout);
    UINT stride = sizeof(float) * 4;
    UINT offset = 0;
    m_context->IASetVertexBuffers(0, 1, &m_vb, &stride, &offset);
    m_context->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLESTRIP);

    m_context->VSSetShader(m_vs, nullptr, 0);
    m_context->PSSetShader(m_ps, nullptr, 0);
    m_context->PSSetConstantBuffers(0, 1, &m_cb);

    ID3D11ShaderResourceView* srvs[] = {m_srv_y, m_srv_u, m_srv_v};
    m_context->PSSetShaderResources(0, 3, srvs);
    m_context->PSSetSamplers(0, 1, &m_sampler);

    m_context->Draw(4, 0);

    // 解绑，避免后续 ImGui 绘制时资源仍被占用为 render target
    ID3D11ShaderResourceView* null_srvs[3] = {};
    m_context->PSSetShaderResources(0, 3, null_srvs);
    ID3D11RenderTargetView* null_rtv = nullptr;
    m_context->OMSetRenderTargets(1, &null_rtv, nullptr);
    return true;
  }

  // Windows 硬解渲染路径待接入（当前 Windows demo 仍走 I420）
  bool upload_gpu(const TcrGpuBuffer& src, OutputTexture* dst) override {
    (void)src;
    (void)dst;
    return false;
  }

  // D3D11 暂未实现回读验证
  int sample_luma(const OutputTexture& tex) override {
    (void)tex;
    return -1;
  }

 private:
  // 暂存纹理按「见过的最大帧尺寸」增长，只增不减，所有流轮流复用
  bool ensure_scratch(int w, int h) {
    if (w <= m_scratch_w && h <= m_scratch_h) return true;
    const int nw = w > m_scratch_w ? w : m_scratch_w;
    const int nh = h > m_scratch_h ? h : m_scratch_h;

    safe_release(&m_srv_y);
    safe_release(&m_srv_u);
    safe_release(&m_srv_v);
    safe_release(&m_scratch_y);
    safe_release(&m_scratch_u);
    safe_release(&m_scratch_v);

    D3D11_TEXTURE2D_DESC desc = {};
    desc.MipLevels = 1;
    desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_R8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_DYNAMIC;
    desc.BindFlags = D3D11_BIND_SHADER_RESOURCE;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_WRITE;

    desc.Width = nw;
    desc.Height = nh;
    HRESULT hr = m_device->CreateTexture2D(&desc, nullptr, &m_scratch_y);
    desc.Width = nw / 2;
    desc.Height = nh / 2;
    hr |= m_device->CreateTexture2D(&desc, nullptr, &m_scratch_u);
    hr |= m_device->CreateTexture2D(&desc, nullptr, &m_scratch_v);
    if (FAILED(hr) || !m_scratch_y || !m_scratch_u || !m_scratch_v) {
      LOG_ERROR("D3D11Backend", "failed to create scratch textures %dx%d", nw, nh);
      return false;
    }

    D3D11_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
    srv_desc.ViewDimension = D3D11_SRV_DIMENSION_TEXTURE2D;
    srv_desc.Texture2D.MipLevels = 1;
    srv_desc.Format = DXGI_FORMAT_R8_UNORM;
    m_device->CreateShaderResourceView(m_scratch_y, &srv_desc, &m_srv_y);
    m_device->CreateShaderResourceView(m_scratch_u, &srv_desc, &m_srv_u);
    m_device->CreateShaderResourceView(m_scratch_v, &srv_desc, &m_srv_v);
    if (!m_srv_y || !m_srv_u || !m_srv_v) {
      LOG_ERROR("D3D11Backend", "failed to create scratch SRVs");
      return false;
    }

    m_scratch_w = nw;
    m_scratch_h = nh;
    LOG_INFO("D3D11Backend", "shared YUV scratch resized to %dx%d", nw, nh);
    return true;
  }

  ID3D11Device* m_device = nullptr;
  ID3D11DeviceContext* m_context = nullptr;

  ID3D11VertexShader* m_vs = nullptr;
  ID3D11PixelShader* m_ps = nullptr;
  ID3D11InputLayout* m_layout = nullptr;
  ID3D11Buffer* m_vb = nullptr;
  ID3D11Buffer* m_cb = nullptr;
  ID3D11SamplerState* m_sampler = nullptr;

  ID3D11Texture2D* m_scratch_y = nullptr;
  ID3D11Texture2D* m_scratch_u = nullptr;
  ID3D11Texture2D* m_scratch_v = nullptr;
  ID3D11ShaderResourceView* m_srv_y = nullptr;
  ID3D11ShaderResourceView* m_srv_u = nullptr;
  ID3D11ShaderResourceView* m_srv_v = nullptr;
  int m_scratch_w = 0;
  int m_scratch_h = 0;
};

}  // namespace

RendererBackend* create_renderer_backend(void* device, void* context) {
  return new D3D11Backend((ID3D11Device*)device, (ID3D11DeviceContext*)context);
}

#endif  // RENDERER_D3D11
