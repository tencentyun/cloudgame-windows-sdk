// main.cpp - 应用入口
// SDL2 + ImGui 初始化 + 主循环
// Windows: SDL2 + D3D11 + ImGui
// macOS/Linux: SDL2 + OpenGL3 + ImGui

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>

#include <cstdlib>
#include <string>

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

// 解析命令行窗口尺寸：--width <px> --height <px>，默认 1920x1080。
// 用于性能测试时通过「启动时指定不同窗口大小」来调整窗口可渲染的子流数量。
static void parse_window_size(int argc, char* argv[], int* out_width, int* out_height) {
  *out_width = 1920;
  *out_height = 1080;
  for (int i = 1; i + 1 < argc; ++i) {
    if (std::string(argv[i]) == "--width") {
      *out_width = std::atoi(argv[i + 1]);
      ++i;
    } else if (std::string(argv[i]) == "--height") {
      *out_height = std::atoi(argv[i + 1]);
      ++i;
    }
  }
  if (*out_width <= 0) *out_width = 1920;
  if (*out_height <= 0) *out_height = 1080;
}

int main(int argc, char* argv[]) {
  int win_w = 1920, win_h = 1080;
  parse_window_size(argc, argv, &win_w, &win_h);

  // 初始化日志：在可执行文件同目录下创建按日期时间命名的日志文件。
  // TcrSdk 日志回调（App::init 中设置）与 Demo 自身日志都会写入该文件。
  {
    char* bp = SDL_GetBasePath();
    std::string log_dir;
    if (bp) {
      log_dir = bp;
      SDL_free(bp);
    }
    // macOS 上 SDL_GetBasePath() 返回 .app/Contents/Resources/，而可执行文件在
    // 同级的 MacOS/ 目录；向上回溯一层让日志落在真正与可执行文件一起的位置。
#if defined(__APPLE__)
    {
      size_t p = log_dir.rfind("Resources/");
      if (p != std::string::npos) {
        log_dir = log_dir.substr(0, p) + "MacOS/";
      }
    }
#endif
    if (!log_dir.empty()) Log::init(log_dir);
  }
  LOG_INFO("Main", "CloudStream ImGui Demo starting...");
  if (!Log::log_path().empty()) {
    LOG_INFO("Main", "Log file: %s", Log::log_path().c_str());
  }

  // 初始化 SDL2
  if (SDL_Init(SDL_INIT_VIDEO | SDL_INIT_TIMER) != 0) {
    LOG_ERROR("Main", "SDL_Init failed: %s", SDL_GetError());
    return 1;
  }

  // 创建窗口
  SDL_Window* window = nullptr;

#if defined(RENDERER_D3D11)
  window = SDL_CreateWindow("CloudStream ImGui Demo (D3D11)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w,
                            win_h, SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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

  window = SDL_CreateWindow("CloudStream ImGui Demo (OpenGL)", SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, win_w,
                            win_h, SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);

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
  // VSync：默认开启。设置环境变量 IMGUI_DEMO_NO_VSYNC=1 可关闭。
  // 无 GUI 会话（如从终端/CI 启动）时窗口可能永不可见，Cocoa_GL_SwapWindow 会
  // 一直阻塞在 VSync 等待上导致主循环卡死，此时需要关闭 VSync。
  const char* no_vsync = SDL_getenv("IMGUI_DEMO_NO_VSYNC");
  SDL_GL_SetSwapInterval((no_vsync && no_vsync[0] == '1') ? 0 : 1);
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
  Log::shutdown();
  return 0;
}
