import QtQuick 2.15
import QtMultimedia 5.15
import UsbPlayer 1.0

// QML owns media/render objects; C++ owns session and playback policy.
Item {
    id: surface
    // The view enables faster position reports only while displaying its timeline.
    property bool timelineActive: false
    readonly property var backend: sessionLoader.item !== null ? sessionLoader.item.player : null
    readonly property bool hasVideo: backend !== null && backend.hasVideo
    readonly property bool playbackStarted: bridge.playbackStarted
    readonly property bool loading: playbackTransport.pendingPlay || sessionLifecycle.resetting
                                    || (backend !== null && backend.status === MediaPlayer.Loading)

    MediaSessionLifecycle {
        id: sessionLifecycle
        desiredSource: playerController.source
    }

    PlaybackCoordinator {
        id: playbackTransport
        backend: surface.backend
        controller: playerController
        ready: sessionLifecycle.active && !sessionLifecycle.resetting && surface.backend !== null && !bridge.blocked
               && (surface.backend.status === MediaPlayer.Loaded
                   || surface.backend.status === MediaPlayer.Buffered
                   || surface.backend.status === MediaPlayer.Buffering
                   || surface.backend.status === MediaPlayer.EndOfMedia)
        atEnd: sessionLifecycle.active && !sessionLifecycle.resetting && surface.backend !== null && !bridge.blocked
               && surface.backend.status === MediaPlayer.EndOfMedia
    }

    VideoPlaybackBridge {
        id: bridge
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
                // Native Qt position notifications; no extra polling timer or interpolation.
                notifyInterval: surface.timelineActive ? 100 : 1000
                source: ""
                onPositionChanged: bridge.reportPosition(position)
                onDurationChanged: bridge.reportDuration(duration)
                onPlaybackStateChanged: bridge.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable, duration)
                onSeekableChanged: bridge.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable, duration)
                onError: bridge.reportMediaError(error, errorString)
            }

            VideoOutput {
                id: videoOutput
                anchors.fill: parent
                source: media
                fillMode: VideoOutput.PreserveAspectFit
                onSourceRectChanged: bridge.reportVideoRect(sourceRect)
                flushMode: playerController.source.toString().length > 0 && !playerController.autoAdvance
                           ? VideoOutput.LastFrame : VideoOutput.EmptyFrame
            }

            // All children, including VideoOutput, must exist before source loading.
            Component.onCompleted: bridge.prepareMedia(media, videoOutput.videoSurface, sessionLifecycle.sessionSource)
        }
    }

    Component.onCompleted: bridge.initialize()
}
