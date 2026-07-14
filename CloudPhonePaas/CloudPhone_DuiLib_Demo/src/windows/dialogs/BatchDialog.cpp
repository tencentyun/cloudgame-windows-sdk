#include "BatchDialog.h"

bool BatchDialog::doModal(HWND parent) {
    Create(parent, m_title.empty() ? _T("操作") : m_title.c_str(),
           UI_WNDSTYLE_DIALOG, WS_EX_WINDOWEDGE);
    CenterWindow();
    ShowModal();
    return m_accepted;
}

void BatchDialog::InitWindow() {
    if (!m_title.empty()) {
        CLabelUI* lblTitle = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_title")));
        if (lblTitle) {
            lblTitle->SetText(m_title.c_str());
        }
    }
}

void BatchDialog::Notify(TNotifyUI& msg) {
    if (msg.sType == DUI_MSGTYPE_CLICK) {
        CDuiString name = msg.pSender->GetName();
        if (name == _T("btn_ok")) {
            m_accepted = true;
            Close();
            return;
        }
        if (name == _T("btn_cancel")) {
            m_accepted = false;
            Close();
            return;
        }
    }
    __super::Notify(msg);
}

LRESULT BatchDialog::HandleMessage(UINT uMsg, WPARAM wParam, LPARAM lParam) {
    return __super::HandleMessage(uMsg, wParam, lParam);
}
