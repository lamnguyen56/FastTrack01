import QtQuick 2.15
import QtMultimedia 5.15

// Qt5 media backend. Transport decisions stay in the C++ controller.
Item {
    readonly property bool hasVideo: media.hasVideo

    MediaPlayer {
        id: media
        source: playerController.source
        onPositionChanged: playerController.reportPosition(position)
        onDurationChanged: playerController.reportDuration(duration)
        onPlaybackStateChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        onSeekableChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        onStatusChanged: {
            if (status === MediaPlayer.EndOfMedia)
                playerController.next()
        }
        onError: {
            if (error !== MediaPlayer.NoError)
                playerController.reportError(errorString)
        }
    }

    VideoOutput {
        anchors.fill: parent
        source: media
        fillMode: VideoOutput.PreserveAspectFit
    }

    Connections {
        target: playerController
        function onPlayRequested() { media.play() }
        function onPauseRequested() { media.pause() }
        function onStopRequested() { media.stop() }
        function onSeekRequested(milliseconds) { media.seek(milliseconds) }
    }
}
