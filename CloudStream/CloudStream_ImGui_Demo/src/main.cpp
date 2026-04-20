// main.cpp - 应用入口
// SDL2 + ImGui 初始化 + 主循环
// Windows: SDL2 + D3D11 + ImGui
// macOS/Linux: SDL2 + OpenGL3 + ImGui

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>

#include "app.h"
#include "logger.h"

#if defined(RENDERER_D3D11)
// ======= Windows: D3D11 =======
#  include <d3d11.h>
#  include <imgui_impl_dx11.h>

static ID3D11Device* g_d3d_device = nullptr;
static ID3D11DeviceContext* g_d3d_context = nullptr;
static IDXGISwapChain* g_swap_chain = nullptr;
static ID3D11RenderTargetView* g_rtv = nullptr;

static bool create_d3d11(SDL_Window* window) {
  SDL_SysWMinfo wmInfo;
  SDL_VERSION(&wmInfo.version);
  SDL_GetWindowWMInfo(window, &wmInfo);
  HWND hwnd = wmInfo.info.win.window;

  DXGI_SWAP_CHAIN_DESC sd = {};
  sd.BufferCount = 2;
  sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
  sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
  sd.OutputWindow = hwnd;
  sd.SampleDesc.Count = 1;
  sd.Windowed = TRUE;
  sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

  D3D_FEATURE_LEVEL feature_level;
  HRESULT hr =
      D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, 0, nullptr, 0, D3D11_SDK_VERSION, &sd,
                                    &g_swap_chain, &g_d3d_device, &feature_level, &g_d3d_context);

  if (FAILED(hr)) {
    LOG_ERROR("Main", "D3D11CreateDeviceAndSwapChain failed: 0x%08x", hr);
    return false;
  }

  ID3D11Texture2D* back_buffer = nullptr;
  g_swap_chain->GetBuffer(0, IID_PPV_ARGS(&back_buffer));
  g_d3d_device->CreateRenderTargetView(back_buffer, nullptr, &g_rtv);
  back_buffer->Release();

  return true;
}

static void cleanup_d3d11() {
  if (g_rtv) {
    g_rtv->Release();
    g_rtv = nullptr;
  }
  if (g_swap_chain) {
    g_swap_chain->Release();
    g_swap_chain = nullptr;
  }
  if (g_d3d_context) {
    g_d3d_context->Release();
    g_d3d_context = nullptr;
  }
  if (g_d3d_device) {
    g_d3d_device->Release();
    g_d3d_device = nullptr;
  }
}

#else
// ======= macOS/Linux: OpenGL =======
#  include <imgui_impl_opengl3.h>
#  if defined(__APPLE__)
#    include <OpenGL/gl3.h>
#  else
#    include <GL/gl.h>
#  endif

static SDL_GLContext g_gl_context = nullptr;

#endif  // RENDERER_D3D11

// =============================================================================
// 主函数
// =============================================================================

int main(int argc, char* argv[]) {
  (void)argc;
  (void)argv;

  LOG_INFO("Main", "CloudStream ImGui Demo starting...");

  // 初始化 SDL2
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    LOG_ERROR("Main", "SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  // 创建窗口
  SDL_Window* window = nullptr;

#if defined(RENDERER_D3D11)
  window = SDL_CreateWindow("CloudStream ImGui Demo (D3D11)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280, 800,
                            SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!window) {
    LOG_ERROR("Main", "SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  if (!create_d3d11(window)) {
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
#else
  // OpenGL 属性设置
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_FLAGS, SDL_GL_CONTEXT_FORWARD_COMPATIBLE_FLAG);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_PROFILE_MASK, SDL_GL_CONTEXT_PROFILE_CORE);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MAJOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_CONTEXT_MINOR_VERSION, 3);
  SDL_GL_SetAttribute(SDL_GL_DOUBLEBUFFER, 1);
  SDL_GL_SetAttribute(SDL_GL_DEPTH_SIZE, 24);

  window = SDL_CreateWindow("CloudStream ImGui Demo (OpenGL)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 1280,
                            800, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

  if (!window) {
    LOG_ERROR("Main", "SDL_CreateWindow failed: %s", SDL_GetError());
    SDL_Quit();
    return 1;
  }

  g_gl_context = SDL_GL_CreateContext(window);
  if (!g_gl_context) {
    LOG_ERROR("Main", "SDL_GL_CreateContext failed: %s", SDL_GetError());
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 1;
  }
  SDL_GL_MakeCurrent(window, g_gl_context);
  SDL_GL_SetSwapInterval(1);  // VSync
#endif

  // 初始化 ImGui
  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  ImGui::StyleColorsDark();

  // 初始化 ImGui 后端
#if defined(RENDERER_D3D11)
  ImGui_ImplSDL2_InitForD3D(window);
  ImGui_ImplDX11_Init(g_d3d_device, g_d3d_context);
#else
  ImGui_ImplSDL2_InitForOpenGL(window, g_gl_context);
  ImGui_ImplOpenGL3_Init("#version 330");
#endif

  // 初始化应用
  App app;
#if defined(RENDERER_D3D11)
  if (!app.init(window, g_d3d_device, g_d3d_context)) {
    LOG_ERROR("Main", "App init failed");
    return 1;
  }
#else
  if (!app.init(window, g_gl_context)) {
    LOG_ERROR("Main", "App init failed");
    return 1;
  }
#endif

  // 主循环
  Uint64 last_ticks = SDL_GetPerformanceCounter();
  Uint64 freq = SDL_GetPerformanceFrequency();
  bool running = true;
  while (running && !app.should_quit()) {
    SDL_Event event;
    while (SDL_PollEvent(&event)) {
      ImGui_ImplSDL2_ProcessEvent(&event);
      app.process_event(event);
      if (event.type == SDL_QUIT) {
        running = false;
      }
      if (event.type == SDL_WINDOWEVENT && event.window.event == SDL_WINDOWEVENT_CLOSE &&
          event.window.windowID == SDL_GetWindowID(window)) {
        running = false;
      }
    }

    // ImGui 新帧
#if defined(RENDERER_D3D11)
    ImGui_ImplDX11_NewFrame();
#else
    ImGui_ImplOpenGL3_NewFrame();
#endif
    ImGui_ImplSDL2_NewFrame();
    ImGui::NewFrame();

    // 计算 delta_time
    Uint64 now = SDL_GetPerformanceCounter();
    float delta_time = (float)(now - last_ticks) / (float)freq;
    last_ticks = now;

    // 应用更新（处理帧 + 渲染 UI）
    app.update(delta_time);

    // 渲染
    ImGui::Render();

#if defined(RENDERER_D3D11)
    float clear_color[] = {0.1f, 0.1f, 0.1f, 1.0f};
    g_d3d_context->OMSetRenderTargets(1, &g_rtv, nullptr);
    g_d3d_context->ClearRenderTargetView(g_rtv, clear_color);
    ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
    g_swap_chain->Present(1, 0);
#else
    int display_w, display_h;
    SDL_GL_GetDrawableSize(window, &display_w, &display_h);
    glViewport(0, 0, display_w, display_h);
    glClearColor(0.1f, 0.1f, 0.1f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT);
    ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
    SDL_GL_SwapWindow(window);
#endif

    // 渲染弹出窗口（在主窗口渲染完成后）
    app.render_popup_windows();
  }

  // 清理
  LOG_INFO("Main", "Shutting down...");

#if defined(RENDERER_D3D11)
  ImGui_ImplDX11_Shutdown();
#else
  ImGui_ImplOpenGL3_Shutdown();
#endif
  ImGui_ImplSDL2_Shutdown();
  ImGui::DestroyContext();

#if defined(RENDERER_D3D11)
  cleanup_d3d11();
#else
  SDL_GL_DeleteContext(g_gl_context);
#endif

  SDL_DestroyWindow(window);
  SDL_Quit();

  LOG_INFO("Main", "Goodbye!");
  return 0;
}
