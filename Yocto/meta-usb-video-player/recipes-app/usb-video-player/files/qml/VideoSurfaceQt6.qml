import QtQuick 2.15
import QtMultimedia 6.2

// Desktop Qt6 uses videoOutput/audioOutput instead of Qt5's VideoOutput.source.
Item {
    readonly property bool hasVideo: media.hasVideo

    MediaPlayer {
        id: media
        source: playerController.source
        videoOutput: output
        audioOutput: AudioOutput {}
        onPositionChanged: playerController.reportPosition(position)
        onDurationChanged: playerController.reportDuration(duration)
        onPlaybackStateChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        onSeekableChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        onMediaStatusChanged: {
            if (mediaStatus === MediaPlayer.EndOfMedia)
                playerController.next()
        }
        onErrorOccurred: function(error, errorString) { playerController.reportError(errorString) }
    }

    VideoOutput {
        id: output
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    Connections {
        target: playerController
        function onPlayRequested() { media.play() }
        function onPauseRequested() { media.pause() }
        function onStopRequested() { media.stop() }
        function onSeekRequested(milliseconds) { media.position = milliseconds }
    }
}
