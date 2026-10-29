import QtQuick 2.15
import QtMultimedia 6.2
import UsbPlayer 1.0

// Qt6 desktop uses its native audio/video output API with the same C++ policy.
Item {
    id: surface
    readonly property var backend: sessionLoader.item !== null ? sessionLoader.item.player : null
    readonly property bool hasVideo: backend !== null && backend.hasVideo
    readonly property bool playbackStarted: bridge.playbackStarted
    readonly property bool loading: playbackTransport.pendingPlay || sessionLifecycle.resetting
                                    || (backend !== null && backend.mediaStatus === MediaPlayer.LoadingMedia)

    MediaSessionLifecycle {
        id: sessionLifecycle
        desiredSource: playerController.source
    }

    PlaybackCoordinator {
        id: playbackTransport
        backend: surface.backend
        controller: playerController
        ready: sessionLifecycle.active && !sessionLifecycle.resetting && surface.backend !== null && !bridge.blocked
               && (surface.backend.mediaStatus === MediaPlayer.LoadedMedia
                   || surface.backend.mediaStatus === MediaPlayer.BufferedMedia
                   || surface.backend.mediaStatus === MediaPlayer.BufferingMedia
                   || surface.backend.mediaStatus === MediaPlayer.EndOfMedia)
        atEnd: sessionLifecycle.active && !sessionLifecycle.resetting && surface.backend !== null && !bridge.blocked
               && surface.backend.mediaStatus === MediaPlayer.EndOfMedia
    }

    VideoPlaybackBridge {
        id: bridge
        qt5: false
        controller: playerController
        lifecycle: sessionLifecycle
        transport: playbackTransport
        backend: surface.backend
    }

    Loader {
        id: sessionLoader
        anchors.fill: parent
        active: sessionLifecycle.active
        sourceComponent: sessionComponent
    }

    Component {
        id: sessionComponent
        Item {
            readonly property var player: media
            MediaPlayer {
                id: media
                source: ""
                videoOutput: output
                audioOutput: AudioOutput {}
                onPositionChanged: bridge.reportPosition(position)
                onDurationChanged: bridge.reportDuration(duration)
                onPlaybackStateChanged: bridge.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable, duration)
                onSeekableChanged: bridge.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable, duration)
                onErrorOccurred: function(error, errorString) { bridge.reportMediaError(error, errorString) }
            }
            VideoOutput {
                id: output
                anchors.fill: parent
                fillMode: VideoOutput.PreserveAspectFit
            }
            Component.onCompleted: media.source = sessionLifecycle.sessionSource
        }
    }

    Component.onCompleted: bridge.initialize()
}
