#pragma once

#include "BatchDialog.h"

/**
 * @brief Simple confirmation dialog with a message + OK/Cancel.
 * Used by: shake, blow, reboot, describeXxx, clearXxx, listXxx, moveAppBackground
 */
class ConfirmDialog : public BatchDialog {
public:
    ConfirmDialog() = default;

    void setMessage(const std::wstring& msg) { m_message = msg; }

    CDuiString GetSkinFile() override { return _T("dialogs\\confirm_dialog.xml"); }

protected:
    void InitWindow() override;

private:
    std::wstring m_message;
};
