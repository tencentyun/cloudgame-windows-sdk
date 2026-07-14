#pragma once

#include <memory>
#include <string>
#include <vector>

#include <windows.h>
#include <UIlib.h>

using namespace DuiLib;

class StreamingViewModel;
class VideoRenderer;

class StreamingWindow : public WindowImplBase {
public:
    StreamingWindow(const std::vector<std::string>& instanceIds, bool isGroupControl);
    ~StreamingWindow() override;

    LPCTSTR GetWindowClassName() const override;
    CDuiString GetSkinFile() override;
    CDuiString GetSkinFolder() override;

    void InitWindow() override;
    void Notify(TNotifyUI& msg) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    std::vector<std::string> m_instanceIds;
    bool m_isGroupControl = false;

    // Video child window (plain Win32 STATIC control used as render target)
    HWND m_videoHwnd = nullptr;

    // ViewModel and renderer
    std::unique_ptr<StreamingViewModel> m_viewModel;
    std::unique_ptr<VideoRenderer> m_renderer;

    // DuiLib controls
    CLabelUI* m_lblSessionStatus = nullptr;
    CLabelUI* m_lblStats = nullptr;
    CEditUI* m_editMinBitrate = nullptr;
    CEditUI* m_editMaxBitrate = nullptr;
    CEditUI* m_editFps = nullptr;

    // Render scale buttons
    CButtonUI* m_btnScale25 = nullptr;
    CButtonUI* m_btnScale50 = nullptr;
    CButtonUI* m_btnScale100 = nullptr;
    CButtonUI* m_btnScale150 = nullptr;
    CButtonUI* m_btnScale200 = nullptr;

    // Video size tracking (for mouse coordinate mapping)
    int m_videoWidth = 0;
    int m_videoHeight = 0;

    void createVideoChildWindow();
    void syncVideoWindowPos();
    void onVideoFrameReceived(std::shared_ptr<struct VideoFrameData> frame);

    // Mouse event helpers for the video child window
    void onVideoMouseEvent(UINT msg, WPARAM wParam, LPARAM lParam);

    // Static subclass proc for the video child window
    static LRESULT CALLBACK VideoSubclassProc(HWND hwnd, UINT msg, WPARAM wParam, LPARAM lParam,
                                               UINT_PTR uIdSubclass, DWORD_PTR dwRefData);
};
