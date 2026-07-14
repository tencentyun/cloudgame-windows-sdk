#pragma once

#include <string>

#include <UIlib.h>

using namespace DuiLib;

/**
 * @brief Base class for all batch operation dialogs.
 *
 * Provides doModal() and accepted() pattern.
 * Subclasses override GetSkinFile() and Notify() to handle their specific UI.
 */
class BatchDialog : public WindowImplBase {
public:
    BatchDialog() = default;
    ~BatchDialog() override = default;

    // Show dialog modally. Returns true if user accepted (clicked OK).
    bool doModal(HWND parent);
    bool accepted() const { return m_accepted; }

    LPCTSTR GetWindowClassName() const override { return _T("BatchDialogClass"); }
    CDuiString GetSkinFolder() override { return _T("skin"); }

    void Notify(TNotifyUI& msg) override;
    LRESULT HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) override;

    // Set the dialog title (call before doModal)
    void setTitle(const std::wstring& title) { m_title = title; }

protected:
    bool m_accepted = false;
    std::wstring m_title;

    void InitWindow() override;
};
