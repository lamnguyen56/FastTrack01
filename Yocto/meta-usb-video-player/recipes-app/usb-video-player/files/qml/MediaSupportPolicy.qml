import QtQml 2.15

/** Translate format failures without mislabelling I/O or device errors as resolution errors. */
QtObject {
    function message(formatFailure, width, height, backendMessage) {
        if (!formatFailure) {
            return backendMessage
        }
        var size = width > 0 && height > 0 ? " (" + width + " x " + height + ")" : ""
        return qsTr("Unsupported resolution or video format%1. Hardware H.264 decoding is required. Use H.264 720p.").arg(size)
    }
}
