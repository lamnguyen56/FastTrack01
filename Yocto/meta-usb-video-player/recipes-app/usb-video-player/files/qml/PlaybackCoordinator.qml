import QtQml 2.15

/** Serialize transport requests with asynchronous media loading and end callbacks. */
QtObject {
    id: gate

    property var backend
    property var controller
    property bool ready: false
    property bool atEnd: false
    property bool pendingPlay: false
    property int generation: 0
    property string requestedSource: ""

    /** Queue Play outside the decoder's current state-change callback. */
    function requestPlay() {
        generation += 1
        requestedSource = controller.source.toString()
        pendingPlay = requestedSource.length > 0
        schedulePlay()
    }

    /** Recheck readiness later; old queued requests cannot affect a newer source. */
    function schedulePlay() {
        var ticket = generation
        Qt.callLater(function() {
            if (ticket === generation && pendingPlay && ready
                    && backend !== null && backend !== undefined
                    && backend.source.toString() === requestedSource
                    && controller.source.toString() === requestedSource) {
                backend.play()
            }
        })
    }

    /** Cancel delayed playback on Pause, Stop, source replacement or errors. */
    function cancel() {
        generation += 1
        pendingPlay = false
    }

    /** Consume the request only after the backend confirms actual playback. */
    function reportPlaying(playing) {
        if (playing) {
            pendingPlay = false
        }
    }

    onReadyChanged: {
        if (ready && pendingPlay) {
            schedulePlay()
        }
    }

    onAtEndChanged: {
        if (atEnd && backend !== null && backend !== undefined) {
            var completedSource = backend.source.toString()
            var ticket = generation
            // Let the old backend finish its EOS transition before loading another file.
            Qt.callLater(function() {
                if (ticket === generation && atEnd
                        && controller.source.toString() === completedSource) {
                    controller.handleEndOfMedia()
                }
            })
        }
    }
}
