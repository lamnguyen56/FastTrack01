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
    // Keep the transport bar compact and size the overlay drawer by orientation.
    // Maximum drawer width; shorter names use only their measured content width.
    readonly property real playlistDrawerWidth: viewportWidth * (portrait ? 0.72 : 0.26)
    readonly property real footerHeight: u(portrait ? 172 : 112)

    /** Convert design units to viewport-relative Qt logical pixels. */
    function u(value) {
        return value * scale
    }
}
