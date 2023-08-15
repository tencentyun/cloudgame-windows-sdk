#ifndef TCRDEMO_MAIN_WND_H_
#define TCRDEMO_MAIN_WND_H_

#include <map>
#include <memory>
#include <string>

#include <vector>
#include <sstream>

#include "video_renderer.h"
#include "tcr_session.h"
#include "video_frame_observer.h"

#include "cloud_game_api.h"

#ifdef WIN32

class MainWnd : public ServerResponseListener, public tcrsdk::TcrSession::Observer,
    public std::enable_shared_from_this<MainWnd> {
 public:
  static const wchar_t kClassName[];

  enum class UI {
      CONNECT_TO_SERVER,
      STREAMING,
  };

  MainWnd();
  ~MainWnd();

  bool Create();
  bool Destroy();
  bool PreTranslateMessage(MSG* msg);

  bool IsWindow();
  void SwitchToConnectUI();
  void SwitchToStreamingUI();
  void MessageBox(const char* caption, const char* text, bool is_error);
  UI current_ui() { return ui_; }

  void QueueUIThreadCallback(int msg_id, void* data);

  // 通过 ServerResponseListener 继承
  virtual void onSuccess(std::string body) override;
  virtual void onFailed(std::string msg) override;

  HWND handle() const { return wnd_; }


 protected:
  enum class ChildWindowID {
    EDIT_ID = 1,
    BUTTON_ID,
    LABEL1_ID,
  };

  void OnPaint();
  void OnDestroyed();

  void OnButtonClick();

  bool OnMessage(UINT msg, WPARAM wp, LPARAM lp, LRESULT* result);

  static LRESULT CALLBACK WndProc(HWND hwnd, UINT msg, WPARAM wp, LPARAM lp);
  static bool RegisterWindowClass();

  void CreateChildWindow(HWND* wnd,
                         ChildWindowID id,
                         const wchar_t* class_name,
                         DWORD control_style,
                         DWORD ex_style);
  void CreateChildWindows();

  void LayoutConnectUI(bool show);

  // 计算鼠标发送坐标
  void CalculateMouselocation(int32_t& x, int32_t& y);

  void GetViewport(int32_t& width, int32_t& height);

  // 通过 TcrSession::Observer 继承
  virtual void onEvent(tcrsdk::TcrSession::Event event, const char* eventData) override;

 private:
  std::shared_ptr<VideoRenderer> remote_renderer_;
  UI ui_;
  HWND wnd_;
  DWORD ui_thread_id_;
  HWND edit1_;
  HWND label1_;
  HWND button_;
  bool destroyed_;
  void* nested_msg_;
  static ATOM wnd_class_;
  std::string exprience_code_;
  std::unique_ptr <tcrsdk::TcrSession> tcr_session_;
  std::unique_ptr<CloudGameApi> cloud_game_api_;
  std::shared_ptr<tcrsdk::TcrLogger>logger_;
  volatile bool is_connected_ = false;
  static const char* const TAG;
};
#endif  // WIN32

#endif 
