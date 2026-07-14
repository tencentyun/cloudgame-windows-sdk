#pragma once

#include <atomic>
#include <map>
#include <string>
#include <vector>

#include <UIlib.h>

using namespace DuiLib;

class ApiService;
class AndroidInstanceModel;
class BatchTaskOperator;
class BatchTaskOperatorModel;

// Menu item IDs for batch operations
#define ID_BATCH_FIRST 40000

class MainWindow : public WindowImplBase {
public:
    MainWindow(ApiService* apiService, AndroidInstanceModel* instanceModel,
               BatchTaskOperator* batchOperator);
    ~MainWindow() override;

    LPCTSTR GetWindowClassName() const override;
    CDuiString GetSkinFile() override;
    CDuiString GetSkinFolder() override;

    void InitWindow() override;
    void Notify(TNotifyUI& msg) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

private:
    ApiService* m_apiService;
    AndroidInstanceModel* m_instanceModel;
    BatchTaskOperator* m_batchOperator;
    BatchTaskOperatorModel* m_batchModel = nullptr;
    CTileLayoutUI* m_tileInstances = nullptr;
    CLabelUI* m_lblStatus = nullptr;

    std::vector<std::string> m_checkedInstanceIds;
    std::map<std::string, HWND> m_streamingWindowMap;

    // Screenshot polling
    static constexpr UINT_PTR TIMER_SCREENSHOT = 1001;
    std::atomic<bool> m_tcrSdkReady{false};
    std::atomic<int> m_screenshotSlot{0};  // ping-pong 0/1 to bypass DuiLib image cache

    void rebuildInstanceList();
    void onInstanceCardClicked(const std::string& instanceId);
    void onCheckBoxChanged(const std::string& instanceId, bool checked);
    void openStreamingWindow(const std::string& instanceId);

    void startScreenshotTimer();
    void fetchAndUpdateScreenshots();

    // Download image URL to a temp file, return the file path (empty on failure)
    static std::string downloadToTempFile(const std::string& url,
                                          const std::string& instanceId, int slot);

    // Build a single instance card control
    CContainerUI* createInstanceCard(const std::string& instanceId,
                                     const std::string& state,
                                     const std::string& region);

    static std::wstring stateToDisplay(const std::string& state);

    void showBatchMenu();
    void onBatchMenuCommand(int menuId);
    void showResultDialog(const std::string& title, const std::string& message);
};
