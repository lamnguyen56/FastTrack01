import QtQuick 2.15
import QtQuick.Controls 2.15

/** Nonmodal USB removal feedback. Storage/player cleanup never waits for dismissal. */
Rectangle {
    id: notice

    property real unitScale: 1
    property bool shown: false
    visible: shown
    implicitHeight: unitScale * 72
    radius: unitScale * 10
    color: "#20313F"
    border.color: "#67DCE7"
    border.width: unitScale

    /** Show once per removal; a later removal starts a fresh display interval. */
    function show() {
        shown = true
        timeout.restart()
    }

    /** Hide feedback without changing the player or USB state. */
    function dismiss() {
        timeout.stop()
        shown = false
    }

    Timer {
        id: timeout
        interval: 6000
        repeat: false
        onTriggered: notice.dismiss()
    }

    Column {
        anchors.left: parent.left
        anchors.leftMargin: notice.unitScale * 16
        anchors.right: dismissButton.left
        anchors.rightMargin: notice.unitScale * 12
        anchors.verticalCenter: parent.verticalCenter
        spacing: notice.unitScale * 4

        Text {
            width: parent.width
            text: qsTr("USB disconnected")
            color: "#E0FCFF"
            font.bold: true
            font.pixelSize: notice.unitScale * 14
            elide: Text.ElideRight
        }
        Text {
            width: parent.width
            text: qsTr("Playback stopped. Reconnect USB to select a video.")
            color: "#B8CBD9"
            font.pixelSize: notice.unitScale * 11
            wrapMode: Text.Wrap
        }
    }

    Button {
        id: dismissButton
        anchors.right: parent.right
        anchors.rightMargin: notice.unitScale * 12
        anchors.verticalCenter: parent.verticalCenter
        width: notice.unitScale * 88
        height: notice.unitScale * 36
        padding: notice.unitScale * 6
        Accessible.name: qsTr("Dismiss USB disconnected notification")
        onClicked: notice.dismiss()
        background: Rectangle {
            color: dismissButton.down ? "#0C6479" : "#164B5B"
            radius: notice.unitScale * 6
        }
        contentItem: Text {
            text: qsTr("Dismiss")
            color: "#E0FCFF"
            font.pixelSize: notice.unitScale * 12
            horizontalAlignment: Text.AlignHCenter
            verticalAlignment: Text.AlignVCenter
        }
    }
}
