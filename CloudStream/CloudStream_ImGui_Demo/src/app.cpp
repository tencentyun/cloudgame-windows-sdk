#include "app.h"

#include <imgui.h>
#include <imgui_impl_sdl2.h>
#include <SDL.h>

#include "http_client.h"
#include "logger.h"
#if defined(RENDERER_D3D11)
#  include <imgui_impl_dx11.h>
#else
#  include <imgui_impl_opengl3.h>
#  if defined(__APPLE__)
#    include <OpenGL/gl3.h>
#  else
#    include <GL/gl.h>
#  endif
#endif

#include <algorithm>
#include <cstring>
#include <fstream>
#include <nlohmann/json.hpp>
#include <sstream>
#include <thread>

#include "tcr_c_api.h"
#include "tcr_types.h"

// =============================================================================
// Lifecycle
// =============================================================================

App::App() {}

App::~App() {
  m_is_destroying.store(true, std::memory_order_release);
  close_all_popups();
  close_session();
  for (auto& kv : m_instance_renderers) {
    kv.second->destroy();
    delete kv.second;
  }
  m_instance_renderers.clear();
}

// =============================================================================
// Init
// =============================================================================

#if defined(RENDERER_D3D11)
bool App::init(SDL_Window* window, void* d3d_device, void* d3d_context) {
  m_window = window;
#else
bool App::init(SDL_Window* window, SDL_GLContext gl_context) {
  m_window = window;
  m_gl_context = gl_context;
#endif
  std::string config_path;
  {
    char* bp = SDL_GetBasePath();
    std::string rp = bp ? std::string(bp) + "config.json" : "";
    SDL_free(bp);
    std::string mp;
    if (!rp.empty()) {
      std::string d = rp.substr(0, rp.rfind("config.json"));
      size_t p = d.rfind("Resources/");
      if (p != std::string::npos) mp = d.substr(0, p) + "MacOS/config.json";
    }
    if (!mp.empty()) {
      std::ifstream t(mp);
      if (t.good()) config_path = mp;
    }
    if (config_path.empty() && !rp.empty()) config_path = rp;
    if (config_path.empty()) config_path = "config.json";
  }
  m_config.load(config_path);

  static TcrLogCallback lcb = {};
  lcb.on_log = [](void*, TcrLogLevel lv, const char* tag, const char* msg) {
    switch (lv) {
      case TCR_LOG_LEVEL_DEBUG:
        LOG_DEBUG(tag, "%s", msg);
        break;
      case TCR_LOG_LEVEL_INFO:
        LOG_INFO(tag, "%s", msg);
        break;
      case TCR_LOG_LEVEL_WARN:
        LOG_WARN(tag, "%s", msg);
        break;
      case TCR_LOG_LEVEL_ERROR:
        LOG_ERROR(tag, "%s", msg);
        break;
      default:
        break;
    }
  };
  tcr_set_log_callback(&lcb);
  tcr_set_log_level(TCR_LOG_LEVEL_INFO);
  LOG_INFO("App", "Init ok. instances=%zu concurrent=%d", m_config.get_instance_id_list().size(),
           m_config.concurrent_streaming);
  m_main_imgui_ctx = ImGui::GetCurrentContext();
  return true;
}

// =============================================================================
// Events
// =============================================================================

void App::process_event(const SDL_Event& ev) {
  for (auto* pw : m_popups) {
    if (!pw->sdl_window) continue;
    Uint32 wid = SDL_GetWindowID(pw->sdl_window);

    if (ev.type == SDL_WINDOWEVENT && ev.window.windowID == wid && ev.window.event == SDL_WINDOWEVENT_CLOSE) {
      pw->close_requested = true;
      continue;
    }

    bool match = false;
    if (ev.type == SDL_MOUSEMOTION && ev.motion.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEBUTTONDOWN && ev.button.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEBUTTONUP && ev.button.windowID == wid) match = true;
    if (ev.type == SDL_MOUSEWHEEL && ev.wheel.windowID == wid) match = true;
    if (ev.type == SDL_KEYDOWN && ev.key.windowID == wid) match = true;
    if (ev.type == SDL_KEYUP && ev.key.windowID == wid) match = true;
    if (ev.type == SDL_TEXTINPUT && ev.text.windowID == wid) match = true;

    if (match) {
      m_popup_events_map[wid].push_back(ev);
    }
  }
}

// =============================================================================
// Main loop
// =============================================================================

void App::update(float dt) {
  // Deferred popup close
  for (size_t i = 0; i < m_popups.size();) {
    if (m_popups[i]->close_requested) {
      destroy_popup(m_popups[i]);
      m_popups.erase(m_popups.begin() + i);
    } else {
      ++i;
    }
  }

  batch_render_frames();

  // Upload frames for all popup windows
  for (auto* pw : m_popups) {
    if (pw->renderer) {
      VideoFrame f;
      if (pw->frame_queue.pop(f)) {
        pw->renderer->upload_frame(f.data_y, f.data_u, f.data_v, f.stride_y, f.stride_u, f.stride_v, f.width, f.height);
      }
    }
  }

  switch (m_state) {
    case AppState::TOKEN_PAGE:
    case AppState::REQUESTING_TOKEN:
      render_token_page();
      break;
    case AppState::MULTI_STREAM:
    case AppState::DISCONNECTED:
      render_multi_stream_page(dt);
      break;
  }
}

// =============================================================================
// Popup management
// =============================================================================

PopupWindow* App::create_popup(const std::string& title, bool is_sync, const std::string& instance_id,
                               const std::vector<std::string>& sync_ids) {
  if (!m_tcr_client) return nullptr;

  PopupWindow* pw = new PopupWindow();
  pw->id = m_next_popup_id++;
  pw->is_sync = is_sync;
  pw->instance_id = instance_id;
  pw->sync_ids = sync_ids;

#if !defined(RENDERER_D3D11)
  SDL_GL_SetAttribute(SDL_GL_SHARE_WITH_CURRENT_CONTEXT, 1);
  pw->sdl_window = SDL_CreateWindow(title.c_str(), SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED, 540, 960,
                                    SDL_WINDOW_OPENGL | SDL_WINDOW_RESIZABLE | SDL_WINDOW_ALLOW_HIGHDPI);
#endif
  if (!pw->sdl_window) {
    delete pw;
    return nullptr;
  }

  // ImGui context
  pw->imgui_ctx = ImGui::CreateContext();
  ImGui::SetCurrentContext(pw->imgui_ctx);
  ImGui::StyleColorsDark();
  ImGui_ImplSDL2_InitForOpenGL(pw->sdl_window, m_gl_context);
  ImGui_ImplOpenGL3_Init("#version 330");
  ImGui::SetCurrentContext(m_main_imgui_ctx);

  // Renderer
  pw->renderer = new VideoRenderer();
#if !defined(RENDERER_D3D11)
  pw->renderer->init();
#endif

  // Session
  TcrSessionConfig cfg = tcr_session_config_default();
  cfg.stream_profile.video_width = m_config.video_width;
  cfg.stream_profile.fps = m_config.video_fps;
  cfg.stream_profile.min_bitrate = m_config.video_min_bitrate;
  cfg.stream_profile.max_bitrate = m_config.video_max_bitrate;

  pw->session = tcr_client_create_session(static_cast<TcrClientHandle>(m_tcr_client), &cfg);
  if (!pw->session) {
    LOG_ERROR("App", "Failed to create popup session");
    destroy_popup(pw);
    return nullptr;
  }

  // Observers - user_data points to the PopupWindow itself
  TcrSessionObserver* obs = reinterpret_cast<TcrSessionObserver*>(pw->session_obs);
  memset(obs, 0, sizeof(TcrSessionObserver));
  obs->user_data = pw;
  obs->on_event = reinterpret_cast<decltype(obs->on_event)>(&App::on_popup_session_event);
  tcr_session_set_observer(static_cast<TcrSessionHandle>(pw->session), obs);

  TcrVideoFrameObserver* vobs = reinterpret_cast<TcrVideoFrameObserver*>(pw->frame_obs);
  memset(vobs, 0, sizeof(TcrVideoFrameObserver));
  vobs->user_data = pw;
  vobs->on_frame = reinterpret_cast<decltype(vobs->on_frame)>(&App::on_popup_video_frame);
  tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(pw->session), vobs);

  // Connect
  if (is_sync) {
    std::vector<const char*> ptrs;
    for (const auto& id : sync_ids) ptrs.push_back(id.c_str());
    tcr_session_access(static_cast<TcrSessionHandle>(pw->session), ptrs.data(), static_cast<int32_t>(ptrs.size()),
                       true);
    LOG_INFO("App", "Popup #%d: sync %zu instances (group control)", pw->id, sync_ids.size());
  } else {
    const char* id_ptr = instance_id.c_str();
    tcr_session_access(static_cast<TcrSessionHandle>(pw->session), &id_ptr, 1, false);
    LOG_INFO("App", "Popup #%d: instance %s", pw->id, instance_id.c_str());
  }

  m_popups.push_back(pw);
  return pw;
}

void App::destroy_popup(PopupWindow* pw) {
  if (!pw) return;

  if (pw->imgui_ctx) {
    ImGui::SetCurrentContext(pw->imgui_ctx);
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplSDL2_Shutdown();
    ImGui::DestroyContext(pw->imgui_ctx);
    pw->imgui_ctx = nullptr;
    ImGui::SetCurrentContext(m_main_imgui_ctx);
  }
  if (pw->session) {
    tcr_session_set_observer(static_cast<TcrSessionHandle>(pw->session), nullptr);
    tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(pw->session), nullptr);
    if (m_tcr_client)
      tcr_client_destroy_session(static_cast<TcrClientHandle>(m_tcr_client),
                                 static_cast<TcrSessionHandle>(pw->session));
    pw->session = nullptr;
  }
  if (pw->renderer) {
    pw->renderer->destroy();
    delete pw->renderer;
    pw->renderer = nullptr;
  }
  if (pw->sdl_window) {
    Uint32 wid = SDL_GetWindowID(pw->sdl_window);
    m_popup_events_map.erase(wid);
    SDL_DestroyWindow(pw->sdl_window);
    pw->sdl_window = nullptr;
  }
  pw->frame_queue.clear();
  delete pw;
}

void App::open_instance_popup(const std::string& id) {
  std::string title = "Instance: " + (id.size() > 30 ? "..." + id.substr(id.size() - 27) : id);
  create_popup(title, false, id, {});
}

void App::open_sync_popup(const std::set<std::string>& ids) {
  std::vector<std::string> v(ids.begin(), ids.end());
  create_popup("Sync Operations (Group Control)", true, "", v);
}

void App::close_all_popups() {
  for (auto* pw : m_popups) destroy_popup(pw);
  m_popups.clear();
  m_popup_events_map.clear();
}

// =============================================================================
// Popup callbacks - user_data is PopupWindow*
// =============================================================================

void App::on_popup_session_event(void* ud, int event, const char* data) {
  PopupWindow* pw = static_cast<PopupWindow*>(ud);
  if (!pw) return;
  TcrSessionEvent ev = (TcrSessionEvent)event;
  if (ev == TCR_SESSION_EVENT_STATE_CONNECTED) {
    pw->connected = true;
    LOG_INFO("App", "Popup #%d connected", pw->id);
  } else if (ev == TCR_SESSION_EVENT_STATE_CLOSED) {
    pw->connected = false;
    LOG_WARN("App", "Popup #%d closed: %s", pw->id, data ? data : "");
  } else if (ev == TCR_SESSION_EVENT_SCREEN_CONFIG_CHANGE && data) {
    // Parse rotation and screen size from event data
    try {
      auto j = nlohmann::json::parse(data);
      pw->video_width = j.value("width", 0);
      pw->video_height = j.value("height", 0);
      std::string degree = j.value("degree", "");
      // Map cloud rotation to client rotation (reverse)
      if (degree.find("90") != std::string::npos)
        pw->rotation_angle = 270;
      else if (degree.find("180") != std::string::npos)
        pw->rotation_angle = 180;
      else if (degree.find("270") != std::string::npos)
        pw->rotation_angle = 90;
      else
        pw->rotation_angle = 0;
      LOG_INFO("App", "Popup #%d screen config: %dx%d degree=%s rotation=%.0f", pw->id, pw->video_width,
               pw->video_height, degree.c_str(), pw->rotation_angle);
    } catch (...) {
    }
  }
}

void App::on_popup_video_frame(void* ud, void* fh) {
  PopupWindow* pw = static_cast<PopupWindow*>(ud);
  if (!pw || !fh) return;
  TcrVideoFrameHandle h = static_cast<TcrVideoFrameHandle>(fh);
  const TcrVideoFrameBuffer* b = tcr_video_frame_get_buffer(h);
  if (!b || b->type != TCR_VIDEO_BUFFER_TYPE_I420) return;
  tcr_video_frame_add_ref(h);
  const TcrI420Buffer& i = b->buffer.i420;
  VideoFrame vf(h, i.data_y, i.data_u, i.data_v, i.stride_y, i.stride_u, i.stride_v, i.width, i.height,
                b->timestamp_us);
  pw->frame_queue.push(std::move(vf));
}

// =============================================================================
// Popup rendering
// =============================================================================

void App::render_popup_windows() {
  for (auto* pw : m_popups) {
    if (pw->close_requested || !pw->sdl_window || !pw->imgui_ctx) continue;
    render_single_popup(pw);
  }
}

void App::render_single_popup(PopupWindow* pw) {
#if !defined(RENDERER_D3D11)
  ImGui::SetCurrentContext(pw->imgui_ctx);
  SDL_GL_MakeCurrent(pw->sdl_window, m_gl_context);

  int win_w, win_h;
  SDL_GetWindowSize(pw->sdl_window, &win_w, &win_h);
  int draw_w, draw_h;
  SDL_GL_GetDrawableSize(pw->sdl_window, &draw_w, &draw_h);

  glViewport(0, 0, draw_w, draw_h);
  glClearColor(0.08f, 0.08f, 0.08f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  // Replay buffered events
  Uint32 wid = SDL_GetWindowID(pw->sdl_window);
  auto it = m_popup_events_map.find(wid);
  if (it != m_popup_events_map.end()) {
    for (const auto& ev : it->second) ImGui_ImplSDL2_ProcessEvent(&ev);
    it->second.clear();
  }

  ImGui_ImplOpenGL3_NewFrame();
  ImGui_ImplSDL2_NewFrame();
  ImGui::NewFrame();

  int w = win_w, h = win_h;
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2((float)w, (float)h));
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));

  char win_id[32];
  snprintf(win_id, sizeof(win_id), "##popup_%d", pw->id);
  ImGui::Begin(win_id, nullptr,
               ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                   ImGuiWindowFlags_NoScrollbar | ImGuiWindowFlags_NoBringToFrontOnFocus);

  // Top bar
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(8, 4));
  bool close_clicked = ImGui::Button("Close");
  ImGui::SameLine();

  // Info
  TcrSessionHandle popup_session = static_cast<TcrSessionHandle>(pw->session);
  if (pw->is_sync) {
    ImGui::Text("Sync: %zu instances", pw->sync_ids.size());
  } else {
    std::string sid =
        pw->instance_id.size() > 30 ? "..." + pw->instance_id.substr(pw->instance_id.size() - 27) : pw->instance_id;
    ImGui::Text("Instance: %s", sid.c_str());
  }
  ImGui::SameLine();
  ImGui::TextColored(pw->connected ? ImVec4(0.3f, 1, 0.3f, 1) : ImVec4(1, 1, 0.3f, 1),
                     pw->connected ? "Connected" : "Connecting...");

  ImGui::SameLine(0, 20);
  auto sendKey = [&](const char* label, int code) {
    if (ImGui::Button(label) && popup_session) {
      tcr_session_send_keyboard_event(popup_session, code, true);
      tcr_session_send_keyboard_event(popup_session, code, false);
    }
  };
  sendKey("Back", 158);
  ImGui::SameLine();
  sendKey("Home", 172);
  ImGui::SameLine();
  sendKey("Menu", 139);
  ImGui::PopStyleVar();

  float bar_h = ImGui::GetCursorPosY();
  ImVec2 avail((float)w, (float)h - bar_h);

  if (pw->renderer && pw->renderer->has_frame()) {
    float tex_w = (float)pw->renderer->texture_width();
    float tex_h = (float)pw->renderer->texture_height();
    float rot = pw->rotation_angle;
    int vid_w = pw->video_width;
    int vid_h = pw->video_height;

    // If no screen config received yet, use texture size as video size
    if (vid_w <= 0 || vid_h <= 0) {
      vid_w = (int)tex_w;
      vid_h = (int)tex_h;
    }

    // Calculate display size considering rotation
    // For 90/270 rotation, the displayed aspect ratio is swapped (height/width)
    float display_w, display_h;
    bool is_rotated = (rot == 90 || rot == 270);
    if (is_rotated) {
      display_w = (float)vid_h;  // swapped
      display_h = (float)vid_w;
    } else {
      display_w = (float)vid_w;
      display_h = (float)vid_h;
    }

    // Fit to available area while maintaining aspect ratio
    float scale = std::min(avail.x / display_w, avail.y / display_h);
    ImVec2 isz(display_w * scale, display_h * scale);

    // Center in available area
    float px = (avail.x - isz.x) * 0.5f;
    float py = (avail.y - isz.y) * 0.5f;
    ImGui::SetCursorPos(ImVec2(px, bar_h + py));
    ImVec2 ip = ImGui::GetCursorScreenPos();

    // Render video (texture is always unrotated I420, rotation is display-only)
    // For now, render the texture as-is with aspect ratio correction
    // The GPU texture contains the raw frame; rotation would need shader support
    // So we render fitting the rotated aspect ratio
    ImGui::Image((ImTextureID)(intptr_t)pw->renderer->get_texture_id(), isz, ImVec2(0, 1), ImVec2(1, 0));

    // Touch input: transform view coordinates to cloud screen coordinates
    if (ImGui::IsItemHovered() && popup_session) {
      ImVec2 mouse = ImGui::GetMousePos();
      // Normalized coordinates in the view [0, 1]
      float nx = (mouse.x - ip.x) / isz.x;
      float ny = (mouse.y - ip.y) / isz.y;
      nx = std::max(0.0f, std::min(1.0f, nx));
      ny = std::max(0.0f, std::min(1.0f, ny));

      // Transform based on rotation (view coords -> cloud screen coords)
      // Cloud screen is always vid_w x vid_h (physical, unrotated)
      int cx, cy;
      if (rot == 90) {
        // Client rotated 90 CW to compensate cloud 270
        cx = (int)(ny * vid_w);
        cy = (int)((1.0f - nx) * vid_h);
      } else if (rot == 180) {
        cx = (int)((1.0f - nx) * vid_w);
        cy = (int)((1.0f - ny) * vid_h);
      } else if (rot == 270) {
        // Client rotated 270 CW to compensate cloud 90
        cx = (int)((1.0f - ny) * vid_w);
        cy = (int)(nx * vid_h);
      } else {
        cx = (int)(nx * vid_w);
        cy = (int)(ny * vid_h);
      }

      if (ImGui::IsMouseClicked(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 0, vid_w, vid_h, (int64_t)SDL_GetTicks());
      if (ImGui::IsMouseDown(0) && ImGui::IsMouseDragging(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 1, vid_w, vid_h, (int64_t)SDL_GetTicks());
      if (ImGui::IsMouseReleased(0))
        tcr_session_touchscreen_touch(popup_session, cx, cy, 2, vid_w, vid_h, (int64_t)SDL_GetTicks());
      float wheel = ImGui::GetIO().MouseWheel;
      if (wheel != 0.0f) tcr_session_send_mouse_scroll(popup_session, wheel > 0 ? 1.0f : -1.0f);
    }
  } else {
    ImGui::SetCursorPos(ImVec2(avail.x * 0.3f, bar_h + avail.y * 0.45f));
    ImGui::Text("Waiting for video...");
  }

  ImGui::End();
  ImGui::PopStyleVar();
  ImGui::Render();
  ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());
  SDL_GL_SwapWindow(pw->sdl_window);

  ImGui::SetCurrentContext(m_main_imgui_ctx);
  SDL_GL_MakeCurrent(m_window, m_gl_context);

  if (close_clicked) pw->close_requested = true;
#endif
}

// =============================================================================
// Token
// =============================================================================

void App::request_token() {
  if (m_state == AppState::REQUESTING_TOKEN) return;
  m_state = AppState::REQUESTING_TOKEN;
  m_error_message.clear();
  auto ids = m_config.get_instance_id_list();
  if (ids.empty()) {
    m_error_message = "No instance IDs";
    m_state = AppState::TOKEN_PAGE;
    return;
  }
  nlohmann::json req;
  req["RequestId"] = "imgui-" + std::to_string(SDL_GetTicks());
  nlohmann::json arr = nlohmann::json::array();
  for (const auto& id : ids) arr.push_back(id);
  req["AndroidInstanceIds"] = arr;
  std::string url = m_config.base_url + m_config.api_path;
  std::string body = req.dump();
  std::thread([this, url, body]() {
    HttpResponse r = http_post(url, body);
    if (!r.ok()) {
      m_error_message = "HTTP " + std::to_string(r.status_code);
      m_state = AppState::TOKEN_PAGE;
      return;
    }
    try {
      auto j = nlohmann::json::parse(r.body);
      if (j.contains("Error")) {
        m_error_message = j["Error"].value("Code", "") + ": " + j["Error"].value("Message", "");
        m_state = AppState::TOKEN_PAGE;
        return;
      }
      m_token = j.value("Token", "");
      m_access_info = j.value("AccessInfo", "");
      if (m_token.empty() || m_access_info.empty()) {
        m_error_message = "Missing Token/AccessInfo";
        m_state = AppState::TOKEN_PAGE;
        return;
      }
      start_multi_streaming();
    } catch (const std::exception& e) {
      m_error_message = e.what();
      m_state = AppState::TOKEN_PAGE;
    }
  }).detach();
}

// =============================================================================
// Multi-stream
// =============================================================================

void App::start_multi_streaming() {
  m_error_message.clear();
  m_tcr_client = tcr_client_get_instance();
  if (!m_tcr_client) {
    m_error_message = "No TcrClient";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  TcrConfig cfg = tcr_config_default();
  cfg.token = m_token.c_str();
  cfg.accessInfo = m_access_info.c_str();
  if (tcr_client_init(static_cast<TcrClientHandle>(m_tcr_client), &cfg) != TCR_SUCCESS) {
    m_error_message = "tcr_client_init failed";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  m_tcr_instance = tcr_client_get_android_instance(static_cast<TcrClientHandle>(m_tcr_client));
  m_all_instance_ids = m_config.get_instance_id_list();
  m_instance_states.clear();
  m_current_streaming_ids.clear();
  m_checked_instances.clear();
  for (const auto& id : m_all_instance_ids) m_instance_states[id] = InstanceState::Connecting;

  create_multi_session();
  access_all_instances();
  m_state = AppState::MULTI_STREAM;
}

void App::create_multi_session() {
  close_session();

  TcrSessionConfig c = tcr_session_config_default();
  c.stream_profile.video_width = m_config.video_width;
  c.stream_profile.fps = m_config.video_fps;
  c.stream_profile.min_bitrate = m_config.video_min_bitrate;
  c.stream_profile.max_bitrate = m_config.video_max_bitrate;
  c.concurrentStreamingInstances = m_config.concurrent_streaming;

  m_tcr_session = tcr_client_create_session(static_cast<TcrClientHandle>(m_tcr_client), &c);
  if (!m_tcr_session) {
    m_error_message = "create_session failed";
    m_state = AppState::TOKEN_PAGE;
    return;
  }

  TcrSessionObserver* o = reinterpret_cast<TcrSessionObserver*>(m_session_observer_storage);
  memset(o, 0, sizeof(TcrSessionObserver));
  o->user_data = this;
  o->on_event = reinterpret_cast<decltype(o->on_event)>(&App::on_multi_session_event);
  tcr_session_set_observer(static_cast<TcrSessionHandle>(m_tcr_session), o);

  TcrVideoFrameObserver* v = reinterpret_cast<TcrVideoFrameObserver*>(m_video_frame_observer_storage);
  memset(v, 0, sizeof(TcrVideoFrameObserver));
  v->user_data = this;
  v->on_frame = reinterpret_cast<decltype(v->on_frame)>(&App::on_multi_video_frame);
  tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(m_tcr_session), v);
}

void App::access_all_instances() {
  if (!m_tcr_session || m_all_instance_ids.empty()) return;
  std::vector<const char*> p;
  for (const auto& id : m_all_instance_ids) p.push_back(id.c_str());
  tcr_session_access_multi_stream(static_cast<TcrSessionHandle>(m_tcr_session), p.data(), (int32_t)p.size());
}

void App::close_session() {
  if (m_tcr_session) {
    tcr_session_set_observer(static_cast<TcrSessionHandle>(m_tcr_session), nullptr);
    tcr_session_set_video_frame_observer(static_cast<TcrSessionHandle>(m_tcr_session), nullptr);
    if (m_tcr_client)
      tcr_client_destroy_session(static_cast<TcrClientHandle>(m_tcr_client),
                                 static_cast<TcrSessionHandle>(m_tcr_session));
    m_tcr_session = nullptr;
  }
  m_multi_frame_cache.clear();
  m_current_streaming_ids.clear();
}

// =============================================================================
// Multi-stream callbacks
// =============================================================================

void App::on_multi_session_event(void* ud, int ev, const char* d) {
  App* s = static_cast<App*>(ud);
  if (!s || s->m_is_destroying.load(std::memory_order_acquire)) return;
  if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_STATE_CONNECTED)
    for (auto& kv : s->m_instance_states) kv.second = InstanceState::Connected;
  else if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_STATE_CLOSED) {
    for (auto& kv : s->m_instance_states) kv.second = InstanceState::Offline;
    s->m_state = AppState::DISCONNECTED;
    s->m_error_message = d ? d : "closed";
  } else if ((TcrSessionEvent)ev == TCR_SESSION_EVENT_CLIENT_STATS && d) {
    s->m_client_stats = d;
  }
}

void App::on_multi_video_frame(void* ud, void* fh) {
  App* s = static_cast<App*>(ud);
  if (!s || !fh || s->m_is_destroying.load(std::memory_order_acquire)) return;
  TcrVideoFrameHandle h = static_cast<TcrVideoFrameHandle>(fh);
  const TcrVideoFrameBuffer* b = tcr_video_frame_get_buffer(h);
  if (!b || b->type != TCR_VIDEO_BUFFER_TYPE_I420) return;
  std::string id;
  if (b->instance_id) id = b->instance_id;
  if (id.empty()) return;
  tcr_video_frame_add_ref(h);
  const TcrI420Buffer& i = b->buffer.i420;
  s->m_multi_frame_cache.push(id, VideoFrame(h, i.data_y, i.data_u, i.data_v, i.stride_y, i.stride_u, i.stride_v,
                                             i.width, i.height, b->timestamp_us));
}

// =============================================================================
// Batch render & stream switching
// =============================================================================

void App::batch_render_frames() {
  auto frames = m_multi_frame_cache.swap_all();
  for (auto& kv : frames) {
    if (!kv.second.valid()) continue;
    VideoRenderer* r = get_or_create_renderer(kv.first);
    if (r)
      r->upload_frame(kv.second.data_y, kv.second.data_u, kv.second.data_v, kv.second.stride_y, kv.second.stride_u,
                      kv.second.stride_v, kv.second.width, kv.second.height);
  }
}

VideoRenderer* App::get_or_create_renderer(const std::string& id) {
  auto it = m_instance_renderers.find(id);
  if (it != m_instance_renderers.end()) return it->second;
  VideoRenderer* r = new VideoRenderer();
#if !defined(RENDERER_D3D11)
  if (!r->init()) {
    delete r;
    return nullptr;
  }
#endif
  m_instance_renderers[id] = r;
  return r;
}

std::vector<std::string> App::calculate_visible_instances(float sy, float vh, float ch) {
  if (ch <= 0 || m_all_instance_ids.empty()) return {};
  int c = m_grid_columns;
  int sr = std::max(0, (int)(sy / ch));
  int er = (int)((sy + vh) / ch) + 1;
  int si = std::max(0, sr * c);
  int ei = std::min(er * c, (int)m_all_instance_ids.size());
  std::vector<std::string> v;
  for (int i = si; i < ei; ++i) v.push_back(m_all_instance_ids[i]);
  return v;
}

void App::switch_streaming_instances(const std::vector<std::string>& ids) {
  if (!m_tcr_session || ids.empty()) return;
  int lim = m_config.concurrent_streaming;
  std::vector<const char*> p;
  for (size_t i = 0; i < ids.size() && (int)i < lim; ++i) p.push_back(ids[i].c_str());
  tcr_session_switch_streaming_instances(static_cast<TcrSessionHandle>(m_tcr_session), p.data(), (int32_t)p.size());
  m_current_streaming_ids.clear();
  for (const auto& id : p) m_current_streaming_ids.insert(id);
}

// =============================================================================
// Page 1: Token
// =============================================================================

void App::render_token_page() {
  ImGuiIO& io = ImGui::GetIO();
  ImVec2 ws(480, 320);
  ImGui::SetNextWindowPos(ImVec2((io.DisplaySize.x - ws.x) * 0.5f, (io.DisplaySize.y - ws.y) * 0.5f), ImGuiCond_Always);
  ImGui::SetNextWindowSize(ws, ImGuiCond_Always);
  ImGui::Begin(
      "##token", nullptr,
      ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoTitleBar);
  ImGui::TextColored(ImVec4(0.1f, 0.46f, 0.82f, 1), "TcrSDK ImGui Demo");
  ImGui::Separator();
  auto ids = m_config.get_instance_id_list();
  if (ids.empty()) {
    ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "No instance IDs!");
  } else {
    ImGui::BeginChild("##ids", ImVec2(-1, 80), true);
    for (size_t i = 0; i < ids.size(); ++i) ImGui::Text("%zu. %s", i + 1, ids[i].c_str());
    ImGui::EndChild();
    ImGui::Text("Total: %zu, concurrent: %d", ids.size(), m_config.concurrent_streaming);
  }
  if (!m_error_message.empty()) ImGui::TextColored(ImVec4(1, 0.3f, 0.3f, 1), "%s", m_error_message.c_str());
  bool busy = (m_state == AppState::REQUESTING_TOKEN);
  if (busy) ImGui::BeginDisabled();
  if (ImGui::Button(busy ? "Requesting..." : "Create Access Token", ImVec2(ImGui::GetContentRegionAvail().x, 40)))
    request_token();
  if (busy) ImGui::EndDisabled();
  ImGui::End();
}

// =============================================================================
// Page 2: Multi-instance grid
// =============================================================================

void App::render_multi_stream_page(float dt) {
  ImGuiIO& io = ImGui::GetIO();

  // Top bar
  ImGui::SetNextWindowPos(ImVec2(0, 0));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 44));
  ImGui::Begin(
      "##top", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
  if (ImGui::Button("Back")) {
    close_all_popups();
    close_session();
    m_state = AppState::TOKEN_PAGE;
  }
  ImGui::SameLine();
  int conn = 0;
  for (const auto& kv : m_instance_states)
    if (kv.second == InstanceState::Connected) conn++;
  ImGui::Text("Connected: %d/%zu | Streaming: %zu", conn, m_all_instance_ids.size(), m_current_streaming_ids.size());
  ImGui::SameLine(0, 20);
  if (ImGui::Button(m_checked_instances.size() == m_all_instance_ids.size() ? "Deselect All" : "Select All")) {
    if (m_checked_instances.size() == m_all_instance_ids.size())
      m_checked_instances.clear();
    else
      for (const auto& id : m_all_instance_ids) m_checked_instances.insert(id);
  }
  ImGui::SameLine();
  ImGui::Text("Sel: %zu", m_checked_instances.size());
  ImGui::SameLine(0, 20);
  if (m_checked_instances.empty()) ImGui::BeginDisabled();
  if (ImGui::Button("Sync Ops")) open_sync_popup(m_checked_instances);
  if (m_checked_instances.empty()) ImGui::EndDisabled();
  ImGui::End();

  // Grid
  float gt = 44, gb = io.DisplaySize.y - 28, gh = gb - gt;
  ImGui::SetNextWindowPos(ImVec2(0, gt));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, gh));
  ImGui::Begin("##grid", nullptr, ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove);

  int cols = m_grid_columns;
  float sp = 6;
  float aw = ImGui::GetContentRegionAvail().x;
  float cw = (aw - sp * (cols - 1)) / cols;
  float vw = cw - 12, vh = vw * 16.0f / 9.0f, ch = vh + 30;

  for (size_t i = 0; i < m_all_instance_ids.size(); ++i) {
    if (i > 0 && (i % cols) != 0) ImGui::SameLine(0, sp);

    const std::string& id = m_all_instance_ids[i];
    bool ck = m_checked_instances.count(id) > 0;

    ImGui::PushID((int)i);
    ImGui::BeginGroup();

    ImVec2 c0 = ImGui::GetCursorScreenPos();
    ImGui::GetWindowDrawList()->AddRect(c0, ImVec2(c0.x + cw, c0.y + ch), IM_COL32(60, 60, 60, 255));

    VideoRenderer* r = m_instance_renderers.count(id) ? m_instance_renderers[id] : nullptr;
    if (r && r->has_frame()) {
      float tw = (float)r->texture_width(), th = (float)r->texture_height();
      float sc = std::min(vw / tw, vh / th);
      ImVec2 isz(tw * sc, th * sc);
      float px = (vw - isz.x) * 0.5f, py = (vh - isz.y) * 0.5f;
      ImGui::SetCursorScreenPos(ImVec2(c0.x + 6 + px, c0.y + py));
      ImGui::Image((ImTextureID)(intptr_t)r->get_texture_id(), isz, ImVec2(0, 1), ImVec2(1, 0));
      if (ImGui::IsItemClicked(0)) open_instance_popup(id);
    } else {
      ImGui::GetWindowDrawList()->AddRectFilled(ImVec2(c0.x + 1, c0.y + 1), ImVec2(c0.x + vw, c0.y + vh),
                                                IM_COL32(30, 30, 30, 255));
    }

    ImGui::SetCursorScreenPos(ImVec2(c0.x + 4, c0.y + vh + 2));

    InstanceState st = InstanceState::Offline;
    auto sit = m_instance_states.find(id);
    if (sit != m_instance_states.end()) st = sit->second;
    const char* lb[] = {"Off", "...", "On"};
    ImVec4 cl[] = {ImVec4(1, .3f, .3f, 1), ImVec4(1, 1, .3f, 1), ImVec4(.3f, 1, .3f, 1)};

    ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(1, 1));
    bool c = ck;
    if (ImGui::Checkbox("##cb", &c)) {
      if (c)
        m_checked_instances.insert(id);
      else
        m_checked_instances.erase(id);
    }
    ImGui::PopStyleVar();
    ImGui::SameLine();
    ImGui::TextColored(cl[(int)st], "%s", lb[(int)st]);
    ImGui::SameLine();
    std::string sid = id.size() > 16 ? "..." + id.substr(id.size() - 13) : id;
    ImGui::TextColored(ImVec4(.5f, .5f, .5f, 1), "%s", sid.c_str());

    ImGui::SetCursorScreenPos(ImVec2(c0.x, c0.y + ch));
    ImGui::Dummy(ImVec2(cw, 0));
    ImGui::EndGroup();
    ImGui::PopID();
  }

  // Scroll debounce
  float cs = ImGui::GetScrollY();
  if (cs != m_prev_scroll_y) {
    m_scroll_dirty = true;
    m_debounce_timer = 0;
    m_prev_scroll_y = cs;
  }
  if (m_scroll_dirty) {
    m_debounce_timer += dt;
    if (m_debounce_timer >= 0.5f) {
      auto vis = calculate_visible_instances(cs, gh, ch);
      std::set<std::string> vs(vis.begin(), vis.end());
      if (vs != m_current_streaming_ids && !vis.empty()) switch_streaming_instances(vis);
      m_scroll_dirty = false;
    }
  }
  if (m_current_streaming_ids.empty() && !m_all_instance_ids.empty()) {
    auto vis = calculate_visible_instances(0, gh, ch);
    if (!vis.empty()) switch_streaming_instances(vis);
  }
  ImGui::End();

  // Status bar
  ImGui::SetNextWindowPos(ImVec2(0, io.DisplaySize.y - 28));
  ImGui::SetNextWindowSize(ImVec2(io.DisplaySize.x, 28));
  ImGui::Begin(
      "##st", nullptr,
      ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoScrollbar);
  ImGui::Text("Instances: %zu | Connected: %d | Streaming: %zu | Selected: %zu | Popups: %zu",
              m_all_instance_ids.size(), conn, m_current_streaming_ids.size(), m_checked_instances.size(),
              m_popups.size());
  ImGui::End();
}
