import QtQuick 2.15
import QtMultimedia 5.15

// A fresh backend and renderer per selection prevents old EOS/caps state reuse.
Item {
    id: surface
    readonly property var backend: sessionLoader.item !== null ? sessionLoader.item.player : null
    readonly property bool hasVideo: backend !== null && backend.hasVideo
    readonly property bool playbackStarted: sessionLoader.item !== null && sessionLoader.item.started
    readonly property bool loading: transport.pendingPlay || lifecycle.resetting
                                    || (backend !== null && backend.status === MediaPlayer.Loading)

    /** Ignore metadata and state callbacks from the session being released. */
    function isCurrentSession() {
        return lifecycle.active && !lifecycle.resetting
                && lifecycle.sessionSource.toString() === playerController.source.toString()
    }

    MediaSupportPolicy { id: supportPolicy }

    /** Stop rejected media, including its audio; do not auto-skip or retry software decode. */
    function rejectCurrent(code, detail) {
        if (!isCurrentSession() || sessionLoader.item === null || sessionLoader.item.blocked) {
            return
        }
        var size = backend.metaData.resolution
        var width = size && size.width > 0 ? size.width : 0
        var height = size && size.height > 0 ? size.height : 0
        var message = supportPolicy.message(code === MediaPlayer.FormatError, width, height, detail)
        sessionLoader.item.blocked = true
        transport.cancel()
        console.warn("[MediaPolicy] rejected:", backend.source, "backend error:", detail)
        backend.stop()
        playerController.reportPlayback(false, false)
        playerController.reportError(message)
    }

    MediaSessionLifecycle {
        id: lifecycle
        desiredSource: playerController.source
        onResettingChanged: {
            // Also publish a synchronous load failure that happened during construction.
            if (!resetting && surface.backend !== null && surface.backend.error !== MediaPlayer.NoError) {
                surface.rejectCurrent(surface.backend.error, surface.backend.errorString)
            }
        }
        onReleaseRequested: {
            if (surface.backend !== null) {
                surface.backend.stop()
                surface.backend.source = ""
            }
        }
    }

    PlaybackCoordinator {
        id: transport
        backend: surface.backend
        controller: playerController
        // Qt5 QML status names differ from Qt6.
        ready: surface.isCurrentSession() && surface.backend !== null && !sessionLoader.item.blocked
               && (surface.backend.status === MediaPlayer.Loaded
                   || surface.backend.status === MediaPlayer.Buffered
                   || surface.backend.status === MediaPlayer.Buffering
                   || surface.backend.status === MediaPlayer.EndOfMedia)
        atEnd: surface.isCurrentSession() && surface.backend !== null && !sessionLoader.item.blocked
               && surface.backend.status === MediaPlayer.EndOfMedia
    }

    Loader {
        id: sessionLoader
        anchors.fill: parent
        active: lifecycle.active
        sourceComponent: sessionComponent
    }

    Component {
        id: sessionComponent
        Item {
            id: mediaSession
            readonly property var player: media
            property bool started: false
            property bool blocked: false

            MediaPlayer {
                id: media
                // Assign the source after VideoOutput has attached (see completion below).
                source: ""
                onPositionChanged: {
                    if (surface.isCurrentSession() && !mediaSession.blocked) {
                        playerController.reportPosition(position)
                    }
                }
                onDurationChanged: {
                    if (surface.isCurrentSession() && !mediaSession.blocked) {
                        playerController.reportDuration(duration)
                    }
                }
                onPlaybackStateChanged: {
                    if (mediaSession.blocked && playbackState === MediaPlayer.PlayingState) {
                        stop()
                        return
                    }
                    if (surface.isCurrentSession() && !mediaSession.blocked) {
                        if (playbackState === MediaPlayer.PlayingState) {
                            mediaSession.started = true
                            playerController.reportDuration(duration)
                            console.info("[MediaSession] playing:", source)
                        }
                        transport.reportPlaying(playbackState === MediaPlayer.PlayingState)
                        playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
                    }
                }
                onSeekableChanged: {
                    if (surface.isCurrentSession() && !mediaSession.blocked) {
                        playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
                    }
                }
                onError: {
                    if (surface.isCurrentSession() && error !== MediaPlayer.NoError) {
                        surface.rejectCurrent(error, errorString)
                    }
                }
            }

            VideoOutput {
                id: output
                anchors.fill: parent
                source: media
                fillMode: VideoOutput.PreserveAspectFit
                onSourceRectChanged: {
                    if (sourceRect.width > 0 && sourceRect.height > 0) {
                        console.info("[MediaSession] video output format:", sourceRect.width, sourceRect.height)
                    }
                }
                // A held EOS keeps this session; source changes destroy it completely.
                flushMode: playerController.source.toString().length > 0 && !playerController.autoAdvance
                           ? VideoOutput.LastFrame : VideoOutput.EmptyFrame
            }

            // All children (including the video sink) exist before decoding can preroll.
            Component.onCompleted: media.source = lifecycle.sessionSource
        }
    }

    Connections {
        target: playerController
        function onSourceChanged() { lifecycle.reset() }
        function onPlayRequested() {
            if (sessionLoader.item === null || !sessionLoader.item.blocked) {
                transport.requestPlay()
            }
        }
        function onPauseRequested() {
            transport.cancel()
            if (surface.backend !== null) {
                surface.backend.pause()
            }
        }
        function onStopRequested() {
            transport.cancel()
            lifecycle.reset()
        }
        function onSeekRequested(milliseconds) {
            if (surface.backend !== null && surface.isCurrentSession()) {
                surface.backend.seek(milliseconds)
            }
        }
    }

    Component.onCompleted: lifecycle.reset()
}
