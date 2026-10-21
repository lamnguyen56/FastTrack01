import QtQuick 2.15
import QtMultimedia 6.2

// Desktop Qt6 uses videoOutput/audioOutput instead of Qt5's VideoOutput.source.
Item {
    id: surface
    readonly property bool hasVideo: media.hasVideo
    property bool playbackStarted: false
    readonly property bool loading: transport.pendingPlay || media.mediaStatus === MediaPlayer.LoadingMedia

    PlaybackCoordinator {
        id: transport
        backend: media
        controller: playerController
        ready: media.mediaStatus === MediaPlayer.LoadedMedia
               || media.mediaStatus === MediaPlayer.BufferedMedia
               || media.mediaStatus === MediaPlayer.BufferingMedia
               || media.mediaStatus === MediaPlayer.EndOfMedia
        atEnd: media.mediaStatus === MediaPlayer.EndOfMedia
    }

    MediaPlayer {
        id: media
        source: playerController.source
        onSourceChanged: surface.playbackStarted = false
        videoOutput: output
        audioOutput: AudioOutput {}
        onPositionChanged: playerController.reportPosition(position)
        onDurationChanged: playerController.reportDuration(duration)
        onPlaybackStateChanged: {
            if (playbackState === MediaPlayer.PlayingState) {
                surface.playbackStarted = true
            }
            transport.reportPlaying(playbackState === MediaPlayer.PlayingState)
            playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)
        }
        onSeekableChanged: playerController.reportPlayback(playbackState === MediaPlayer.PlayingState, seekable)

        onErrorOccurred: function(error, errorString) { transport.cancel(); playerController.reportError(errorString) }
    }

    VideoOutput {
        id: output
        anchors.fill: parent
        fillMode: VideoOutput.PreserveAspectFit
    }

    Connections {
        target: playerController
        function onPlayRequested() { transport.requestPlay() }
        function onPauseRequested() { transport.cancel(); media.pause() }
        function onStopRequested() { transport.cancel(); media.stop() }
        function onSeekRequested(milliseconds) { media.position = milliseconds }
    }
}
