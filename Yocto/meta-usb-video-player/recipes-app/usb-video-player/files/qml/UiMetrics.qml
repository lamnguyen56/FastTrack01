import QtQml 2.15

/** Responsive logical units shared by every panel, control, label and icon. */
QtObject {
    property real viewportWidth: 1920
    property real viewportHeight: 1080

    readonly property bool portrait: viewportHeight > viewportWidth
    // These are design references, not fixed window or panel dimensions.
    readonly property real referenceWidth: portrait ? 1080 : 1920
    readonly property real referenceHeight: portrait ? 1920 : 1080
    readonly property real scale: Math.max(0.01, Math.min(viewportWidth / referenceWidth,
                                                       viewportHeight / referenceHeight))
    // Match the original compact chrome on the Pi HDMI reference display.
    // These fractions remain relative at every resolution; no size cap is used.
    readonly property real playlistWidthFraction: 300 / 1920
    readonly property real portraitVideoFraction: 0.75
    readonly property real panelGap: u(14)
    readonly property real headerHeight: u(portrait ? 96 : 56)
    readonly property real footerHeight: u(portrait ? 172 : 112)

    /** Convert design units to viewport-relative Qt logical pixels. */
    function u(value) {
        return value * scale
    }
}
