import QtQml 2.15

/** Text feedback follows transport state, not delayed hasVideo metadata. */
QtObject {
    property bool mounted: false
    property int mediaCount: 0
    property string errorText: ""
    property bool loading: false
    property bool playbackStarted: false

    readonly property bool visible: !mounted || errorText.length > 0
                                    || mediaCount === 0 || !playbackStarted
    readonly property bool busy: mounted && mediaCount > 0
                                 && errorText.length === 0 && loading && !playbackStarted
    readonly property bool canPlay: mounted && mediaCount > 0 && errorText.length === 0 && !busy
    readonly property string title: !mounted ? qsTr("NO USB CONNECTED")
                                    : errorText.length > 0 ? qsTr("PLAYBACK ERROR")
                                    : mediaCount === 0 ? qsTr("NO MEDIA")
                                    : busy ? qsTr("LOADING VIDEO") : qsTr("READY TO PLAY")
    readonly property string detail: !mounted ? qsTr("Insert a USB drive containing videos")
                                     : errorText.length > 0 ? errorText
                                     : mediaCount === 0 ? qsTr("Waiting for playlist / no supported media files")
                                     : busy ? qsTr("Preparing playback...")
                                     : qsTr("Select a video from the playlist or press Play")
}
