#pragma once

#include <functional>

#ifdef _WIN32
#include <windows.h>
#endif

/**
 * @brief Helper to post callbacks from worker threads to the UI thread.
 *
 * Usage:
 *   // In worker thread:
 *   UiThreadHelper::postToUiThread(hWnd, [=]() { updateUI(); });
 *
 *   // In window's HandleMessage:
 *   if (uMsg == WM_UI_CALLBACK) {
 *       auto* func = reinterpret_cast<std::function<void()>*>(lParam);
 *       (*func)();
 *       delete func;
 *       return 0;
 *   }
 */

#define WM_UI_CALLBACK (WM_USER + 100)

namespace UiThreadHelper {

inline void postToUiThread(HWND hwnd, std::function<void()> func) {
    // Allocate callback on heap; the receiver is responsible for deleting it
    auto* heapFunc = new std::function<void()>(std::move(func));
    ::PostMessage(hwnd, WM_UI_CALLBACK, 0, reinterpret_cast<LPARAM>(heapFunc));
}

}  // namespace UiThreadHelper
