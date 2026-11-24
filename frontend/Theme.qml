pragma Singleton
import QtQuick

// Every colour and size of the app lives here. Use it anywhere as Theme.windowBg, Theme.accent, ...
QtObject {
    // ---- surfaces ----
    readonly property color windowBg:      "#161616"
    readonly property color panelBg:       "#202020"
    readonly property color panelHeaderBg: "#2e2e2e"
    readonly property color panelEdge:     "#0c0c0c"
    readonly property color cardBg:        "#222222"
    readonly property color cardBgAlt:     "#272727"
    readonly property color divider:       "#333333"
    readonly property color hoverOverlay:  "#1affffff"
    readonly property color pressedOverlay:"#26ffffff"

    // ---- text ----
    readonly property color text:          "#ededed"
    readonly property color textStrong:    "#ffffff"
    readonly property color textMuted:     "#a8a8a8"
    readonly property color textDisabled:  "#777b83"

    // ---- tooltips ----
    readonly property color tooltipBg:     "#2b2d32"
    readonly property color tooltipText:   "#f3f4f6"

    // ---- states & accents ----
    readonly property color accent:        "#60cdff"
    readonly property color focusRing:     "#60cdff"
    readonly property color selectionBg:   "#24426b"
    readonly property color accentBg:      "#203a45"
    readonly property color danger:        "#e5484d"
    readonly property color dangerBg:      "#a9363c"
    readonly property color warning:       "#f59e0b"
    readonly property color ok:            "#22c55e"

    // ---- controls & buttons ----
    readonly property color controlBg:       "#2a2a2a"
    readonly property color controlBgHover:  "#353535"
    readonly property color controlBgActive: "#3d3d3d"
    readonly property color controlBorder:   "#404040"

    // ---- fields ----
    readonly property color field:           "#292b30"
    readonly property color fieldHover:      "#30333a"
    readonly property color fieldDisabled:   "#222328"
    readonly property color fieldBorder:     "#444750"

    // ---- popups ----
    readonly property color popupBg:         "#25272c"
    readonly property color popupBorder:     "#4a4d56"

    // ---- sizes ----
    readonly property int microSize:   9
    readonly property int captionSize: 10
    readonly property int smallSize:   11
    readonly property int bodySize:    13
    readonly property int headingSize: 15
    readonly property int stripHeight: 28
    readonly property int margin:      10
    readonly property int controlRadius: 4
    readonly property int panelRadius:   6
    readonly property int spaceXs: 4
    readonly property int spaceSm: 8
    readonly property int spaceMd: 12
    readonly property int spaceLg: 16

    // ---- typography ----
    readonly property string monospaceFontFamily: "Consolas, 'Cascadia Code', 'Fira Code', 'DejaVu Sans Mono', monospace"
}
