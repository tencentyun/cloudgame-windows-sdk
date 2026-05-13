pragma Singleton
import QtQuick 2.15

QtObject {
    readonly property SystemPalette _pal: SystemPalette { colorGroup: SystemPalette.Active }
    readonly property bool isDark: (0.299 * _pal.window.r + 0.587 * _pal.window.g + 0.114 * _pal.window.b) < 0.5

    // ---- Text colors ----
    readonly property color textPrimary: _pal.windowText
    readonly property color textSecondary: isDark ? "#AAAAAA" : "#666666"
    readonly property color textHint: isDark ? "#999999" : "#888888"

    // ---- Background colors ----
    readonly property color windowBg: _pal.window
    readonly property color cardBg: isDark ? "#2D2D2D" : "white"
    readonly property color inputBg: isDark ? "#3A3A3A" : "#FAFAFA"
    readonly property color headerBg: isDark ? "#383838" : "#F5F5F5"
    readonly property color statusBarBg: isDark ? "#2D2D2D" : "#F0F0F0"
    readonly property color listEvenBg: isDark ? "#2A2A2A" : "#F9F9F9"
    readonly property color listOddBg: isDark ? "#333333" : "white"
    readonly property color listHoverBg: isDark ? "#3A4A5A" : "#E8F4FD"

    // ---- Border colors ----
    readonly property color border: isDark ? "#555555" : "#CCCCCC"

    // ---- Primary accent (constant) ----
    readonly property color primary: "#1976D2"
    readonly property color primaryHover: "#1565C0"

    // ---- Button colors ----
    readonly property color btnBg: isDark ? "#424242" : "#F5F5F5"
    readonly property color btnHoverBg: isDark ? "#4A4A4A" : "#E3F2FD"
    readonly property color btnDisabledBg: isDark ? "#555555" : "#BDBDBD"
    readonly property color cancelBtnBg: isDark ? "#424242" : "#F5F5F5"
    readonly property color cancelBtnHoverBg: isDark ? "#4A4A4A" : "#E0E0E0"
    readonly property color cancelBtnBorder: isDark ? "#666666" : "#BDBDBD"
    readonly property color cancelBtnText: isDark ? "#E0E0E0" : "#424242"

    // ---- Palette colors for Controls (Button, ComboBox, SpinBox, GroupBox, etc.) ----
    // Qt Quick Controls 2 Basic style reads palette to render text/background.
    // Setting palette on the Window propagates to all child controls.
    readonly property color paletteBase: isDark ? "#3A3A3A" : "#FFFFFF"
    readonly property color paletteAlternateBase: isDark ? "#424242" : "#F7F7F7"
    readonly property color paletteButton: isDark ? "#4A4A4A" : "#ECECEC"
    readonly property color paletteButtonText: isDark ? "#E0E0E0" : "#1C1C1C"
    readonly property color paletteMid: isDark ? "#666666" : "#B0B0B0"
    readonly property color paletteLight: isDark ? "#5A5A5A" : "#FFFFFF"
    readonly property color paletteMidlight: isDark ? "#505050" : "#CACACA"
    readonly property color paletteDark: isDark ? "#2A2A2A" : "#9F9F9F"
    readonly property color paletteHighlight: "#1976D2"
    readonly property color paletteHighlightedText: "#FFFFFF"
    readonly property color paletteToolTipBase: isDark ? "#4A4A4A" : "#FFFFDC"
    readonly property color paletteToolTipText: isDark ? "#E0E0E0" : "#1C1C1C"
}
