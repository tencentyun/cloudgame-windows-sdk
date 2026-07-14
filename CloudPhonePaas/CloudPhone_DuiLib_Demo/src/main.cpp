/**
 * @file main.cpp
 * @brief CloudPhone_DuiLib_Demo entry point
 *
 * Initializes crash handler, logging, curl, services, DuiLib.
 * Shows LoginWindow first; on successful login, opens MainWindow with instance list.
 */

#include <windows.h>
#include <objbase.h>

#include <curl/curl.h>
#include <UIlib.h>

#include "services/ApiService.h"
#include "services/NetworkService.h"
#include "utils/CrashDumpHandler.h"
#include "utils/Logger.h"
#include "core/BatchTaskOperator.h"
#include "viewmodels/AndroidInstanceModel.h"
#include "windows/LoginWindow.h"
#include "windows/MainWindow.h"

using namespace DuiLib;

int WINAPI WinMain(_In_ HINSTANCE hInstance,
                   _In_opt_ HINSTANCE /*hPrevInstance*/,
                   _In_ LPSTR /*lpCmdLine*/,
                   _In_ int /*nCmdShow*/) {
    // Initialize crash dump handler (must be first)
    CrashDumpHandler::initialize();

    // Initialize logging system
    Logger::globalInit();
    Logger::info("Application started");

    // Initialize COM
    ::CoInitialize(nullptr);

    // Initialize libcurl globally
    curl_global_init(CURL_GLOBAL_ALL);
    Logger::info("libcurl initialized");

    // Create service layer
    NetworkService networkService;
    networkService.setRequestHost("ap-shenzhen");
    networkService.setOrigin("https://mouhong.test-cai-experience.crtrcloud.com");
    ApiService apiService(&networkService);

    // Create batch operator and instance model
    BatchTaskOperator batchOperator;
    AndroidInstanceModel instanceModel(&apiService, &batchOperator, &networkService);

    // Initialize DuiLib
    CPaintManagerUI::SetInstance(hInstance);
    CPaintManagerUI::SetResourcePath(CPaintManagerUI::GetInstancePath() + _T("skin\\"));
    Logger::info("DuiLib initialized");

    // Show login window (modal)
    LoginWindow loginWindow(&apiService);
    loginWindow.Create(nullptr, _T("登录"), UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
    loginWindow.CenterWindow();
    Logger::info("LoginWindow created, entering modal loop");
    loginWindow.ShowModal();

    // If login succeeded, show main window
    if (loginWindow.loginSucceeded()) {
        Logger::info("Login succeeded, opening MainWindow");

        MainWindow mainWindow(&apiService, &instanceModel, &batchOperator);
        mainWindow.Create(nullptr, _T("主窗口"), UI_WNDSTYLE_FRAME, WS_EX_WINDOWEDGE);
        mainWindow.CenterWindow();
        mainWindow.ShowWindow(true);

        // Enter message loop for main window
        CPaintManagerUI::MessageLoop();
    }

    // Cleanup
    Logger::info("Application exiting");
    curl_global_cleanup();
    ::CoUninitialize();
    return 0;
}
