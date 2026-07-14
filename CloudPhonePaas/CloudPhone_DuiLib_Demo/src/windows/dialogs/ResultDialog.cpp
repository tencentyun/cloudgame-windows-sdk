#include "ResultDialog.h"

void ResultDialog::InitWindow() {
    BatchDialog::InitWindow();

    CRichEditUI* edit = static_cast<CRichEditUI*>(m_PaintManager.FindControl(_T("edit_result")));
    if (edit && !m_resultText.empty()) {
        edit->SetText(m_resultText.c_str());
    }
}
