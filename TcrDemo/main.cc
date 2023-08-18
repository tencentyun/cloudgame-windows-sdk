#include <string>
#include <vector>
#include <memory>

#include "main_wnd.h"
#include "crash_dump.h"
#include <shellapi.h> 


int PASCAL wWinMain(HINSTANCE instance,
                    HINSTANCE prev_instance,
                    wchar_t* cmd_line,
                    int cmd_show) {
  // 注册crash dump
  CrashDump::InitCrashDump();

  // 创建主窗口
  std::shared_ptr<MainWnd> wnd(new MainWnd());
  if (!wnd->Create()) {
    return -1;
  }

  // Main loop.
  MSG msg;
  BOOL gm;
  while ((gm = ::GetMessage(&msg, NULL, 0, 0)) != 0 && gm != -1) {
    if (!wnd->PreTranslateMessage(&msg)) {
      ::TranslateMessage(&msg);
      ::DispatchMessage(&msg);
    }
  }
  return 0;
}
