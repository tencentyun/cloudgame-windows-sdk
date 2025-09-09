import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

Item {
    id: dialogsRoot

    property int dialogWidth: 600
    property int dialogHeight: 400

    // 通用提示框
    Dialog {
        id: genericTipDialog
        property string tipTitle: ""
        property string tipMessage: ""
        property var buttons: Dialog.Ok
        
        title: tipTitle
        standardButtons: buttons
        width: dialogsRoot.dialogWidth
        height: dialogsRoot.dialogHeight
        anchors.centerIn: parent
        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 20
            spacing: 10
            ScrollView {
                Layout.fillWidth: true
                Layout.fillHeight: true
                TextArea {
                    text: genericTipDialog.tipMessage
                    wrapMode: Text.WordWrap
                    readOnly: true
                    selectByMouse: true
                    background: null
                }
            }
        }
    }
    property alias genericTipDialog: genericTipDialog
}