import QtQuick 2.15
import QtMultimedia 5.15

// Qt5 media backend. Transport decisions stay in the C++ controller.
Item {
    readonly property bool hasVideo: media.hasVideo

    PlaybackCoordinator {
        id: transport
        backend: media
        controller: playerController
        // Qt5 QML uses Loaded/Buffered/Buffering (unlike the Qt6 names).
        ready: media.status === MediaPlayer.Loaded
               || media.status === MediaPlayer.Buffered
               || media.status === MediaPlayer.Buffering
               || media.status === MediaPlayer.EndOfMedia
        atEnd: media.status === MediaPlayer.EndOfMedia
    }

    MediaPlayer {
        id: media
        source: playerController.source
        onPositionChanged: playerController.reportPosition(position)
        onDurationChanged: playerController.reportDuration(duration)
        onPlaybackStateChanged: {
            transport.reportPlaying(playbackState === MediaPlayer.PlayingState)
            playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        }
        onSeekableChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)

        onError: {
            if (error !== MediaPlayer.NoError) {
                transport.cancel()
                playerController.reportError(errorString)
            }
        }
    }

    VideoOutput {
        anchors.fill: parent
        source: media
        fillMode: VideoOutput.PreserveAspectFit
        // Keep the final image when automatic next is disabled; clear on unplug.
        flushMode: playerController.source.toString().length > 0 && playerController.autoAdvance === false
                   ? VideoOutput.LastFrame : VideoOutput.EmptyFrame
    }

    Connections {
        target: playerController
        function onPlayRequested() { transport.requestPlay() }
        function onPauseRequested() { transport.cancel(); media.pause() }
        function onStopRequested() { transport.cancel(); media.stop() }
        function onSeekRequested(milliseconds) { media.seek(milliseconds) }
    }
}
