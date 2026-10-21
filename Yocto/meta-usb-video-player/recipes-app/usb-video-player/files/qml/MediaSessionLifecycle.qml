import QtQml 2.15

/** Own the lifetime boundary between media sources, including same-file replay. */
QtObject {
    id: lifecycle

    property url desiredSource: ""
    property url sessionSource: ""
    property bool active: false
    property bool resetting: false
    property int generation: 0

    /** Release the old backend synchronously before disposing its renderer. */
    signal releaseRequested()

    /** Recreate only the latest requested session after the old callback unwinds. */
    function reset() {
        generation += 1
        var ticket = generation
        resetting = true
        releaseRequested()
        active = false
        sessionSource = ""
        Qt.callLater(function() {
            if (ticket !== generation) {
                return
            }
            sessionSource = desiredSource
            active = sessionSource.toString().length > 0
            resetting = false
            console.info("[MediaSession] new session:", sessionSource)
        })
    }
}
