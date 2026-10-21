import QtQuick 2.15
import org.freedesktop.gstreamer.GLVideoItem 1.0

// GStreamer owns playback; this item only participates in Qt Quick composition.
Item {
    readonly property bool playbackStarted: gstPlayer.playbackStarted
    readonly property bool loading: gstPlayer.loading

    GstGLVideoItem {
        id: output
        anchors.fill: parent
        forceAspectRatio: true
        visible: playerController.source.toString().length > 0
    }

    // Defer attachment until the Loader has parented the item into its window.
    Component.onCompleted: Qt.callLater(function() { gstPlayer.attach(output) })
    Component.onDestruction: gstPlayer.detach()
}
