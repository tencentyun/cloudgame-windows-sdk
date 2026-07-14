#pragma once

#include <string>

#include "BatchDialog.h"

/**
 * @brief Dialog to display batch task results (scrollable text + OK button).
 */
class ResultDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\result_dialog.xml"); }

    void setResultText(const std::wstring& text) { m_resultText = text; }

protected:
    void InitWindow() override;

private:
    std::wstring m_resultText;
};
