#include "ConfirmDialog.h"

void ConfirmDialog::InitWindow() {
    BatchDialog::InitWindow();

    CLabelUI* lblMsg = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_message")));
    if (lblMsg && !m_message.empty()) {
        lblMsg->SetText(m_message.c_str());
    }
}
