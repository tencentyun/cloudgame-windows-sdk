#pragma once

#include <string>

#include "BatchDialog.h"
#include "utils/StringUtils.h"

// ============================================================================
// PackageNameDialog — single package name input
// ============================================================================
class PackageNameDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\package_dialog.xml"); }

    std::string packageName() const { return m_packageName; }

protected:
    void InitWindow() override {
        BatchDialog::InitWindow();
    }

    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* edit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field1")));
            if (edit) m_packageName = StringUtils::WideToUtf8(std::wstring(edit->GetText().GetData()));
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_packageName;
};

// ============================================================================
// AppListDialog — comma-separated app list
// ============================================================================
class AppListDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\app_list_dialog.xml"); }

    std::string appListStr() const { return m_appListStr; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* edit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field1")));
            if (edit) m_appListStr = StringUtils::WideToUtf8(std::wstring(edit->GetText().GetData()));
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_appListStr;
};

// ============================================================================
// TextInputDialog — single text input with custom label
// ============================================================================
class TextInputDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\text_input_dialog.xml"); }

    void setFieldLabel(const std::wstring& label) { m_fieldLabel = label; }
    std::string text() const { return m_text; }

protected:
    void InitWindow() override {
        BatchDialog::InitWindow();
        if (!m_fieldLabel.empty()) {
            CLabelUI* lbl = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_field1")));
            if (lbl) lbl->SetText(m_fieldLabel.c_str());
        }
    }

    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* edit = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field1")));
            if (edit) m_text = StringUtils::WideToUtf8(std::wstring(edit->GetText().GetData()));
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::wstring m_fieldLabel;
    std::string m_text;
};

// ============================================================================
// TwoFieldDialog — two input fields with custom labels
// ============================================================================
class TwoFieldDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\two_field_dialog.xml"); }

    void setFieldLabels(const std::wstring& label1, const std::wstring& label2) {
        m_label1 = label1;
        m_label2 = label2;
    }
    std::string field1() const { return m_field1; }
    std::string field2() const { return m_field2; }

protected:
    void InitWindow() override {
        BatchDialog::InitWindow();
        if (!m_label1.empty()) {
            CLabelUI* lbl = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_field1")));
            if (lbl) lbl->SetText(m_label1.c_str());
        }
        if (!m_label2.empty()) {
            CLabelUI* lbl = static_cast<CLabelUI*>(m_PaintManager.FindControl(_T("lbl_field2")));
            if (lbl) lbl->SetText(m_label2.c_str());
        }
    }

    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* e1 = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field1")));
            CEditUI* e2 = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field2")));
            if (e1) m_field1 = StringUtils::WideToUtf8(std::wstring(e1->GetText().GetData()));
            if (e2) m_field2 = StringUtils::WideToUtf8(std::wstring(e2->GetText().GetData()));
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::wstring m_label1, m_label2;
    std::string m_field1, m_field2;
};

// ============================================================================
// GpsDialog — longitude + latitude
// ============================================================================
class GpsDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\gps_dialog.xml"); }

    double longitude() const { return m_longitude; }
    double latitude() const { return m_latitude; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* e1 = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field1")));
            CEditUI* e2 = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_field2")));
            if (e1) {
                try { m_longitude = std::stod(StringUtils::WideToUtf8(std::wstring(e1->GetText().GetData()))); }
                catch (...) {}
            }
            if (e2) {
                try { m_latitude = std::stod(StringUtils::WideToUtf8(std::wstring(e2->GetText().GetData()))); }
                catch (...) {}
            }
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    double m_longitude = 121.4737;
    double m_latitude = 31.2304;
};

// ============================================================================
// ResolutionDialog — width + height + DPI
// ============================================================================
class ResolutionDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\resolution_dialog.xml"); }

    int width() const { return m_width; }
    int height() const { return m_height; }
    int dpi() const { return m_dpi; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* eW = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_width")));
            CEditUI* eH = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_height")));
            CEditUI* eD = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_dpi")));
            if (eW) { try { m_width = std::stoi(StringUtils::WideToUtf8(std::wstring(eW->GetText().GetData()))); } catch (...) {} }
            if (eH) { try { m_height = std::stoi(StringUtils::WideToUtf8(std::wstring(eH->GetText().GetData()))); } catch (...) {} }
            if (eD) { try { m_dpi = std::stoi(StringUtils::WideToUtf8(std::wstring(eD->GetText().GetData()))); } catch (...) {} }
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    int m_width = 720;
    int m_height = 1280;
    int m_dpi = 320;
};

// ============================================================================
// SensorDialog — sensor type combo + X/Y/Z + accuracy
// ============================================================================
class SensorDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\sensor_dialog.xml"); }

    std::string sensorType() const { return m_sensorType; }
    std::string valuesStr() const { return m_valuesStr; }
    int accuracy() const { return m_accuracy; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CComboUI* combo = static_cast<CComboUI*>(m_PaintManager.FindControl(_T("combo_type")));
            CEditUI* eX = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_x")));
            CEditUI* eY = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_y")));
            CEditUI* eZ = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_z")));
            CEditUI* eAcc = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_accuracy")));

            if (combo) {
                int sel = combo->GetCurSel();
                if (sel >= 0) {
                    CControlUI* item = combo->GetItemAt(sel);
                    if (item) m_sensorType = StringUtils::WideToUtf8(std::wstring(item->GetText().GetData()));
                }
            }

            std::string x = "0", y = "0", z = "0";
            if (eX) x = StringUtils::WideToUtf8(std::wstring(eX->GetText().GetData()));
            if (eY) y = StringUtils::WideToUtf8(std::wstring(eY->GetText().GetData()));
            if (eZ) z = StringUtils::WideToUtf8(std::wstring(eZ->GetText().GetData()));
            m_valuesStr = x + "," + y + "," + z;

            if (eAcc) {
                try { m_accuracy = std::stoi(StringUtils::WideToUtf8(std::wstring(eAcc->GetText().GetData()))); }
                catch (...) {}
            }

            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_sensorType = "accelerometer";
    std::string m_valuesStr;
    int m_accuracy = 3;
};

// ============================================================================
// StartAppDialog — packageName + activityName
// ============================================================================
class StartAppDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\start_app_dialog.xml"); }

    std::string packageName() const { return m_packageName; }
    std::string activityName() const { return m_activityName; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* ePkg = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_package")));
            CEditUI* eAct = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_activity")));
            if (ePkg) m_packageName = StringUtils::WideToUtf8(std::wstring(ePkg->GetText().GetData()));
            if (eAct) m_activityName = StringUtils::WideToUtf8(std::wstring(eAct->GetText().GetData()));
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_packageName;
    std::string m_activityName;
};

// ============================================================================
// KeepFrontDialog — packageName + enable checkbox + restart interval
// ============================================================================
class KeepFrontDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\keep_front_dialog.xml"); }

    std::string packageName() const { return m_packageName; }
    bool enable() const { return m_enable; }
    int restartInterval() const { return m_restartInterval; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* ePkg = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_package")));
            COptionUI* chk = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("chk_enable")));
            CEditUI* eInt = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_interval")));
            if (ePkg) m_packageName = StringUtils::WideToUtf8(std::wstring(ePkg->GetText().GetData()));
            if (chk) m_enable = chk->IsSelected();
            if (eInt) {
                try { m_restartInterval = std::stoi(StringUtils::WideToUtf8(std::wstring(eInt->GetText().GetData()))); }
                catch (...) {}
            }
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_packageName;
    bool m_enable = false;
    int m_restartInterval = 0;
};

// ============================================================================
// CameraPlayDialog — filePath + loops
// ============================================================================
class CameraPlayDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\camera_play_dialog.xml"); }

    std::string filePath() const { return m_filePath; }
    int loops() const { return m_loops; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            CEditUI* eFp = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_filepath")));
            CEditUI* eLp = static_cast<CEditUI*>(m_PaintManager.FindControl(_T("edit_loops")));
            if (eFp) m_filePath = StringUtils::WideToUtf8(std::wstring(eFp->GetText().GetData()));
            if (eLp) {
                try { m_loops = std::stoi(StringUtils::WideToUtf8(std::wstring(eLp->GetText().GetData()))); }
                catch (...) {}
            }
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    std::string m_filePath;
    int m_loops = 1;
};

// ============================================================================
// MuteDialog — mute checkbox
// ============================================================================
class MuteDialog : public BatchDialog {
public:
    CDuiString GetSkinFile() override { return _T("dialogs\\mute_dialog.xml"); }

    bool isMuted() const { return m_mute; }

protected:
    void Notify(TNotifyUI& msg) override {
        if (msg.sType == DUI_MSGTYPE_CLICK && msg.pSender->GetName() == _T("btn_ok")) {
            COptionUI* chk = static_cast<COptionUI*>(m_PaintManager.FindControl(_T("chk_mute")));
            if (chk) m_mute = chk->IsSelected();
            m_accepted = true;
            Close();
            return;
        }
        BatchDialog::Notify(msg);
    }

private:
    bool m_mute = false;
};
