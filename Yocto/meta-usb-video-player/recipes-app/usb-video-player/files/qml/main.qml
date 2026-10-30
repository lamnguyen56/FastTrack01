import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15
import UsbPlayer 1.0

ApplicationWindow {
    id: rootWindow
    visible: true
    width: Screen.width
    height: Screen.height
    title: qsTr("FastTrack Automotive Infotainment")
    color: "#0B0E14" // Deep Cockpit Dark

    UiMetrics {
        id: ui
        viewportWidth: rootWindow.width
        viewportHeight: rootWindow.height
    }

    // Feedback is informational; USB teardown and empty state never wait for user input.
    UsbDisconnectNotice {
        id: usbNotice
        anchors.horizontalCenter: parent.horizontalCenter
        anchors.bottom: parent.bottom
        anchors.bottomMargin: ui.footerHeight + ui.u(12)
        width: Math.min(rootWindow.width * 0.80, ui.u(640))
        unitScale: ui.scale
        z: 100
    }

    // All state comes from the actual USB model and media engine.
    readonly property bool isPlaying: playerController.playing
    readonly property int currentTrackIndex: playerController.currentIndex
    // Keep controls available while paused, hovering below or seeking; the list is independent.
    // No fade or geometry animation competes with video rendering.
    readonly property bool controlsShown: (!rootWindow.isPlaying || controlsHover.hovered || progressSlider.pressed)
    // Hidden controls detach timeline bindings to avoid unnecessary per-frame UI work.
    readonly property real currentProgress: controlsShown ? playerController.position : 0

    PlaybackClickFeedback {
        id: clickFeedback
    }
    Connections {
        target: playerController
        function onSourceChanged() { clickFeedback.dismiss() }
        function onErrorChanged() {
            if (playerController.errorText.length > 0) clickFeedback.dismiss()
        }
    }

    // Video geometry stays fixed while controls and playlist overlay it.
    Item {
        id: mainContent
        anchors.top: parent.top
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: ui.u(14)

        // --- Video Canvas Surface ---
        Rectangle {
            id: videoSurface
            anchors.fill: parent
            color: "#05070A"
            clip: true

            Loader {
                id: videoLoader
                anchors.fill: parent
                anchors.margins: ui.u(2)
                source: qtMajorVersion >= 6 ? "qrc:/ui/VideoSurfaceQt6.qml" : "qrc:/ui/VideoSurfaceQt5.qml"
            }

            Binding {
                target: videoLoader.item
                property: "timelineActive"
                value: rootWindow.controlsShown
                when: videoLoader.item !== null
            }

            PlayerOverlayState {
                id: overlayState
                mounted: usbReader.mounted
                mediaCount: usbPlaylist.count
                errorText: playerController.errorText
                loading: videoLoader.item !== null && videoLoader.item.loading
                playbackStarted: videoLoader.item !== null && videoLoader.item.playbackStarted
            }

            // Empty/loading/error state; decoded video is rendered underneath.
            Column {
                id: emptyState
                anchors.centerIn: parent
                width: Math.max(0, parent.width - ui.u(40))
                visible: overlayState.visible
                spacing: ui.u(12)

                Rectangle {
                    anchors.horizontalCenter: parent.horizontalCenter
                    width: ui.u(72)
                    height: ui.u(72)
                    radius: ui.u(36)
                    color: rootWindow.isPlaying ? "#003A4D" : "#141923"
                    border.color: rootWindow.isPlaying ? "#00F0FF" : "#324158"
                    border.width: ui.u(2)

                    MediaIcon {
                        anchors.centerIn: parent
                        name: "play"
                        visible: overlayState.busy === false
                        color: rootWindow.isPlaying ? "#00F0FF" : "#71829D"
                        iconSize: ui.u(28)
                    }

                    BusyIndicator {
                        anchors.centerIn: parent
                        width: ui.u(48)
                        height: ui.u(48)
                        running: overlayState.busy
                        visible: running
                    }

                    MouseArea {
                        enabled: overlayState.canPlay
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: playerController.toggle()
                    }
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: overlayState.title
                    color: rootWindow.isPlaying ? "#00F0FF" : "#A0AEC0"
                    font.bold: true
                    font.pixelSize: ui.u(15)
                    font.letterSpacing: ui.u(1)
                }

                Text {
                    anchors.horizontalCenter: parent.horizontalCenter
                    text: overlayState.detail
                    width: emptyState.width
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: "#55657E"
                    font.pixelSize: ui.u(12)
                }
            }

            // Track information follows the transport controls instead of covering video.
            Rectangle {
                visible: rootWindow.controlsShown
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: ui.u(14)
                height: ui.u(28)
                width: Math.min(parent.width - ui.u(28), currentTrackText.implicitWidth + ui.u(32))
                radius: ui.u(6)
                color: "#18203099"
                border.color: "#2D3748"
                border.width: ui.u(1)

                Item {
                    anchors.fill: parent
                    anchors.leftMargin: ui.u(10)
                    anchors.rightMargin: ui.u(10)
                    Rectangle {
                        anchors.left: parent.left
                        anchors.verticalCenter: parent.verticalCenter
                        width: ui.u(6); height: ui.u(6); radius: ui.u(3)
                        color: rootWindow.isPlaying ? "#00F0FF" : "#888888"
                    }
                    Text {
                        id: currentTrackText
                        anchors.left: parent.left
                        anchors.leftMargin: ui.u(12)
                        anchors.right: parent.right
                        anchors.verticalCenter: parent.verticalCenter
                        elide: Text.ElideRight
                        text: playerController.currentTitle.length > 0 ? playerController.currentTitle : "No media selected"
                        color: "#E2E8F0"
                        font.pixelSize: ui.u(11)
                    }
                }
            }
        }

        // A static, non-interactive icon adds no per-frame animation over the video.
        MediaIcon {
            anchors.centerIn: parent
            iconSize: ui.u(64)
            name: clickFeedback.iconName
            color: "#FFFFFF"
            visible: clickFeedback.shown
        }

        // A video click dismisses the list first; otherwise it toggles playback.
        // The Drawer lives above this area, and footer buttons receive their own clicks.
        MouseArea {
            id: videoClickArea
            anchors.fill: parent
            z: 1
            acceptedButtons: Qt.LeftButton
            onClicked: {
                if (playlistDrawer.visible) {
                    playlistDrawer.close()
                } else if (rootWindow.isPlaying || overlayState.canPlay) {
                    clickFeedback.showForPlayback(!rootWindow.isPlaying)
                    playerController.toggle()
                }
            }
        }
    }

    // Plain, unwrapped multiline text measures the widest filename in the actual font.
    // Bold sizing also leaves room for the selected row. Keep this binding independent
    // of visibility: QML remeasures only when titles or font metrics change.
    Text {
        id: playlistTitleMeasure
        visible: false
        text: usbPlaylist.titles.join("\n")
        textFormat: Text.PlainText
        wrapMode: Text.NoWrap
        font.pixelSize: ui.u(13)
        font.bold: true
    }
    Text {
        id: playlistHeaderMeasure
        visible: false
        text: qsTr("Playlist") + "  " + usbPlaylist.count
        font.pixelSize: ui.u(15)
        font.bold: true
    }

    // Native Drawer supports edge dragging; no open/close animation during playback.
    Drawer {
        id: playlistDrawer
        edge: Qt.LeftEdge
        y: ui.u(14)
        // Include number/icon columns, row padding, scrollbar and outer padding.
        width: Math.min(ui.playlistDrawerWidth,
                        Math.max(playlistHeaderMeasure.implicitWidth + ui.u(64),
                                 playlistTitleMeasure.implicitWidth + ui.u(108)))
        // Show only the space needed by the heading and file rows; scroll longer lists.
        height: Math.min(Math.max(0, mainContent.height - ui.footerHeight), ui.u(84) + usbPlaylist.count * ui.u(52))
        padding: ui.u(10)
        modal: false
        dim: false
        interactive: true
        dragMargin: ui.u(20)
        // The non-modal toggle sits outside the drawer. Closing on outside press
        // would hide it before the button's clicked signal, reopening it immediately.
        // Video clicks are handled by videoClickArea; footer clicks stay independent.
        closePolicy: Popup.CloseOnEscape
        // Qt5 needs a transition object to commit Drawer.position, even at zero duration.
        enter: Transition { NumberAnimation { property: "position"; to: 1; duration: 0 } }
        exit: Transition { NumberAnimation { property: "position"; to: 0; duration: 0 } }
        background: Rectangle {
            // Alpha affects the background only, keeping file names fully legible.
            color: "#C4121622"
            radius: ui.u(10)
            border.color: "#60475870"
            border.width: ui.u(1)
        }

        // Build once before playback and retain delegates while hidden. Opening the
        // drawer then avoids recreating rows and uploading their text every time.
        contentItem: Loader {
            active: true
            sourceComponent: playlistComponent
        }
    }

    Connections {
        target: usbReader
        function onDisconnected() { playlistDrawer.close(); clickFeedback.dismiss() }
    }

    Component {
        id: playlistComponent
        // Native glyphs avoid first-use distance-field generation for these
        // labels, whose font size already follows the viewport scale.
        Item {
            ColumnLayout {
                anchors.fill: parent
                spacing: ui.u(12)

                RowLayout {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ui.u(34)
                    spacing: ui.u(10)

                    MediaIcon {
                        name: "folder"
                        color: "#67DCE7"
                        iconSize: ui.u(18)
                        Layout.preferredWidth: iconSize
                        Layout.preferredHeight: iconSize
                    }
                    Text {
                        renderType: Text.NativeRendering
                        Layout.fillWidth: true
                        text: qsTr("Playlist")
                        color: "#F1F5F9"
                        font.bold: true
                        font.pixelSize: ui.u(15)
                        elide: Text.ElideRight
                    }
                    // A quiet count badge replaces the former Close button.
                    Rectangle {
                        Layout.preferredWidth: fileCount.implicitWidth + ui.u(18)
                        Layout.preferredHeight: ui.u(24)
                        radius: ui.u(12)
                        color: "#303B4D"
                        Text {
                            renderType: Text.NativeRendering
                            id: fileCount
                            anchors.centerIn: parent
                            text: usbPlaylist.count
                            color: "#CBD5E1"
                            font.pixelSize: ui.u(11)
                        }
                    }
                }

                Rectangle {
                    Layout.fillWidth: true
                    Layout.preferredHeight: ui.u(1)
                    color: "#40506278"
                }

                ListView {
                    id: playlistView
                    Layout.minimumWidth: 0
                    Layout.minimumHeight: 0
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    model: usbPlaylist
                    currentIndex: rootWindow.currentTrackIndex
                    cacheBuffer: 0
                    reuseItems: true
                    spacing: ui.u(4)
                    boundsBehavior: Flickable.StopAtBounds
                    highlightMoveDuration: 0
                    highlightResizeDuration: 0

                    ScrollBar.vertical: ScrollBar {
                        id: playlistScrollBar
                        policy: ScrollBar.AsNeeded
                        width: ui.u(4)
                        background: Item {}
                        contentItem: Rectangle {
                            implicitWidth: ui.u(4)
                            radius: ui.u(2)
                            color: playlistScrollBar.pressed ? "#67DCE7" : "#607182"
                        }
                    }

                    delegate: ItemDelegate {
                        id: trackRow
                        readonly property bool selected: rootWindow.currentTrackIndex === index
                        width: playlistView.width - ui.u(10)
                        height: ui.u(48)
                        padding: ui.u(10)
                        hoverEnabled: true
                        Accessible.name: model.title

                        background: Rectangle {
                            radius: ui.u(6)
                            color: trackRow.down ? "#B32B4058"
                                   : trackRow.selected ? "#A326394C"
                                   : trackRow.hovered ? "#5033465C" : "transparent"

                            // Static selection marker; no continuously animated equalizer.
                            Rectangle {
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                width: ui.u(3)
                                height: parent.height * 0.5
                                radius: ui.u(1.5)
                                color: "#67DCE7"
                                visible: trackRow.selected
                            }
                        }
                        contentItem: RowLayout {
                            spacing: ui.u(10)
                            Text {
                                renderType: Text.NativeRendering
                                Layout.preferredWidth: ui.u(24)
                                text: index + 1
                                horizontalAlignment: Text.AlignHCenter
                                color: trackRow.selected ? "#67DCE7" : "#8394A7"
                                font.pixelSize: ui.u(11)
                            }
                            Text {
                                renderType: Text.NativeRendering
                                Layout.fillWidth: true
                                Layout.minimumWidth: 0
                                text: model.title
                                color: trackRow.selected ? "#E0FCFF" : "#E2E8F0"
                                font.bold: trackRow.selected
                                font.pixelSize: ui.u(13)
                                elide: Text.ElideMiddle
                                maximumLineCount: 1
                            }
                            MediaIcon {
                                name: rootWindow.isPlaying ? "pause" : "play"
                                color: "#67DCE7"
                                iconSize: ui.u(14)
                                Layout.preferredWidth: iconSize
                                Layout.preferredHeight: iconSize
                                visible: trackRow.selected
                            }
                        }
                        onClicked: {
                            playerController.selectTrack(index)
                            playlistDrawer.close()
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // BOTTOM CONTROL BAR
    // ==========================================
    // This non-visual region remains present while the bar is hidden. HoverHandler
    // observes pointer motion without consuming presses intended for the controls.
    Item {
        id: controlsRegion
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.bottom: parent.bottom
        height: ui.footerHeight + ui.u(24)
        z: 10

        HoverHandler {
            id: controlsHover
        }

        Rectangle {
            id: controlBar
            visible: rootWindow.controlsShown
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: ui.footerHeight
            // Preserve icon/text contrast without drawing an opaque band over video.
            color: "transparent"

            Item {
                id: timelineContent
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                anchors.margins: ui.u(12)
                height: ui.u(20)

                // Explicit footer row geometry avoids nested height-for-width recalculation.
                RowLayout {
                    anchors.top: parent.top
                    anchors.left: parent.left
                    anchors.right: parent.right
                    height: ui.u(20)
                    spacing: ui.u(12)

                    Text {
                        text: playerController.formatTime(rootWindow.controlsShown ? playerController.position : 0)
                        color: "#00F0FF"
                        font.bold: true
                        font.pixelSize: ui.u(12)
                        font.family: "DejaVu Sans Mono"
                    }

                    Slider {
                        id: progressSlider
                        implicitHeight: ui.u(20)
                        padding: ui.u(2)
                        Layout.fillWidth: true
                        from: 0
                        to: Math.max(1, (rootWindow.controlsShown ? playerController.duration : 0))
                        enabled: rootWindow.controlsShown && usbReader.mounted && playerController.seekable
                        value: rootWindow.currentProgress

                        onMoved: playerController.seekTo(value)

                        // Custom Track
                        background: Rectangle {
                            x: progressSlider.leftPadding
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            implicitWidth: ui.u(200)
                            implicitHeight: ui.u(6)
                            width: progressSlider.availableWidth
                            height: implicitHeight
                            radius: ui.u(3)
                            color: "#1C2433"

                            // Active Progress Bar Gradient
                            Rectangle {
                                width: progressSlider.visualPosition * parent.width
                                height: parent.height
                                radius: ui.u(3)
                                gradient: Gradient {
                                    orientation: Gradient.Horizontal
                                    GradientStop { position: 0.0; color: "#0077B6" }
                                    GradientStop { position: 1.0; color: "#00F0FF" }
                                }
                            }
                        }

                        // Custom Handle
                        handle: Rectangle {
                            x: progressSlider.leftPadding + progressSlider.visualPosition * (progressSlider.availableWidth - width)
                            y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                            implicitWidth: ui.u(16)
                            implicitHeight: ui.u(16)
                            radius: ui.u(8)
                            color: "#FFFFFF"
                            border.color: "#00F0FF"
                            border.width: ui.u(3)

                            // Glowing hover effect ring
                            Rectangle {
                                anchors.centerIn: parent
                                width: ui.u(26)
                                height: ui.u(26)
                                radius: ui.u(13)
                                color: "#00F0FF"
                                opacity: progressSlider.pressed ? 0.4 : (progressSlider.hovered ? 0.2 : 0)
                                Behavior on opacity { NumberAnimation { duration: 150 } }
                            }
                        }
                    }

                    Text {
                        text: playerController.formatTime((rootWindow.controlsShown ? playerController.duration : 0))
                        color: "#8E9EB5"
                        font.bold: true
                        font.pixelSize: ui.u(12)
                        font.family: "DejaVu Sans Mono"
                    }
                }

            }

            // Center every button between the timeline's lower edge and screen bottom.
            // Keeping a single shared area avoids independent bottom offsets.
            Item {
                id: buttonArea
                anchors.top: timelineContent.bottom
                anchors.bottom: parent.bottom
                anchors.left: parent.left
                anchors.right: parent.right

                Button {
                    id: playlistToggle
                    anchors.left: parent.left
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.leftMargin: ui.u(16)
                    width: ui.u(184)
                    height: ui.u(50)
                    padding: ui.u(10)
                    text: qsTr("Playlist")
                    font.pixelSize: ui.u(13)
                    hoverEnabled: true
                    Accessible.name: playlistDrawer.visible ? qsTr("Close playlist") : qsTr("Open playlist")
                    onClicked: playlistDrawer.visible ? playlistDrawer.close() : playlistDrawer.open()
                    background: Item {}
                    // Match Auto Next typography. Static content has no timeline bindings
                    // or continuous animation; the folder repaints only when its state changes.
                    contentItem: Item {
                        opacity: playlistToggle.down ? 0.75 : 1
                        Column {
                            id: playlistButtonLabels
                            anchors.left: parent.left
                            width: Math.min(parent.width - ui.u(32),
                                            Math.max(playlistPrimary.implicitWidth, playlistSecondary.implicitWidth))
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: ui.u(2)
                            Text {
                                id: playlistPrimary
                                width: parent.width
                                text: playlistToggle.text
                                elide: Text.ElideRight
                                color: playlistDrawer.visible || playlistToggle.hovered ? "#E0FCFF" : "#CBD5E1"
                                font.pixelSize: ui.u(12)
                                font.bold: true
                            }
                            Text {
                                id: playlistSecondary
                                width: parent.width
                                text: playlistDrawer.visible ? qsTr("HIDE PLAYLIST") : qsTr("%1 VIDEOS").arg(usbPlaylist.count)
                                elide: Text.ElideRight
                                color: playlistDrawer.visible ? "#67DCE7" : "#8292AA"
                                font.pixelSize: ui.u(8)
                                font.letterSpacing: ui.u(0.5)
                            }
                        }
                        MediaIcon {
                            id: playlistButtonIcon
                            anchors.left: playlistButtonLabels.right
                            anchors.leftMargin: ui.u(8)
                            anchors.verticalCenter: parent.verticalCenter
                            width: ui.u(24)
                            height: ui.u(24)
                            iconSize: ui.u(24)
                            name: "folder"
                            color: playlistDrawer.visible || playlistToggle.hovered ? "#67DCE7" : "#CBD5E1"
                        }
                    }
                }

                // Bottom-right Auto Next label and state switch without an outer button frame.
                Button {
                    id: autoNextSwitch
                    anchors.right: parent.right
                    anchors.verticalCenter: parent.verticalCenter
                    anchors.rightMargin: ui.u(16)
                    width: ui.u(184)
                    height: ui.u(50)
                    padding: ui.u(10)
                    checkable: true
                    checked: playerController.autoAdvance
                    hoverEnabled: true
                    Accessible.name: qsTr("Automatically play next video")
                    onToggled: playerController.autoAdvance = checked

                    background: Item {}

                    contentItem: Item {
                        opacity: autoNextSwitch.down ? 0.75 : 1
                        Column {
                            anchors.left: parent.left
                            anchors.right: toggleTrack.left
                            anchors.rightMargin: ui.u(10)
                            anchors.verticalCenter: parent.verticalCenter
                            spacing: ui.u(2)
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: qsTr("Auto next")
                                color: autoNextSwitch.checked ? "#E0FCFF" : "#CBD5E1"
                                font.pixelSize: ui.u(12)
                                font.bold: true
                            }
                            Text {
                                width: parent.width
                                elide: Text.ElideRight
                                text: autoNextSwitch.checked ? qsTr("PLAY NEXT VIDEO") : qsTr("STOP AT END")
                                color: autoNextSwitch.checked ? "#67DCE7" : "#8292AA"
                                font.pixelSize: ui.u(8)
                                font.letterSpacing: ui.u(0.5)
                            }
                        }

                        Rectangle {
                            id: toggleTrack
                            anchors.right: parent.right
                            anchors.verticalCenter: parent.verticalCenter
                            width: ui.u(34)
                            height: ui.u(20)
                            radius: ui.u(10)
                            color: autoNextSwitch.checked ? "#00B4D8" : "#3A485D"
                            Rectangle {
                                y: ui.u(3)
                                x: ui.u(autoNextSwitch.checked ? 17 : 3)
                                width: ui.u(14)
                                height: ui.u(14)
                                radius: ui.u(7)
                                color: "#FFFFFF"
                                Behavior on x { NumberAnimation { duration: 120 } }
                            }
                        }
                    }
                }

                // --- ICON-ONLY AUTOMOTIVE CONTROL BUTTONS ---
                Row {
                    enabled: usbReader.mounted && usbPlaylist.count > 0
                    opacity: enabled ? 1 : 0.4
                    anchors.horizontalCenter: parent.horizontalCenter
                    anchors.verticalCenter: parent.verticalCenter
                    height: ui.u(58)
                    spacing: ui.u(22)

                    // 1. Previous Track Button
                    Button {
                        id: prevBtn
                        padding: ui.u(6)
                        anchors.verticalCenter: parent.verticalCenter
                        Accessible.name: qsTr("Previous track")
                        implicitWidth: ui.u(46)
                        implicitHeight: ui.u(46)
                        hoverEnabled: true

                        // Invisible background retains the original touch/click target.
                        background: Item {}

                        contentItem: MediaIcon {
                            opacity: prevBtn.down ? 0.75 : 1
                            name: "previous"
                            color: prevBtn.hovered ? "#00F0FF" : "#CBD5E1"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            iconSize: ui.u(22)
                        }

                        onClicked: playerController.previous()
                    }

                    // 2. Fast Backward -10s
                    Button {
                        id: rewBtn
                        padding: ui.u(6)
                        anchors.verticalCenter: parent.verticalCenter
                        Accessible.name: qsTr("Rewind 10 seconds")
                        implicitWidth: ui.u(46)
                        implicitHeight: ui.u(46)
                        hoverEnabled: true

                        // Invisible background retains the original touch/click target.
                        background: Item {}

                        contentItem: MediaIcon {
                            opacity: rewBtn.down ? 0.75 : 1
                            name: "rewind"
                            color: rewBtn.hovered ? "#00F0FF" : "#CBD5E1"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            iconSize: ui.u(22)
                        }

                        onClicked: playerController.seekRelative(-10)
                    }

                    // 3. Larger Play / Pause icon without a background or decorative ring.
                    Button {
                        id: playPauseBtn
                        enabled: playerController.errorText.length === 0
                        padding: ui.u(6)
                        anchors.verticalCenter: parent.verticalCenter
                        Accessible.name: rootWindow.isPlaying ? qsTr("Pause") : qsTr("Play")
                        implicitWidth: ui.u(58)
                        implicitHeight: ui.u(58)
                        hoverEnabled: true

                        // Invisible background retains the original touch/click target.
                        background: Item {}

                        contentItem: MediaIcon {
                            opacity: playPauseBtn.down ? 0.75 : 1
                            name: rootWindow.isPlaying ? "pause" : "play"
                            color: playPauseBtn.hovered ? "#00F0FF" : "#FFFFFF"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            iconSize: ui.u(28)
                        }

                        onClicked: playerController.toggle()
                    }

                    // 4. Fast Forward +10s
                    Button {
                        id: fwdBtn
                        padding: ui.u(6)
                        anchors.verticalCenter: parent.verticalCenter
                        Accessible.name: qsTr("Forward 10 seconds")
                        implicitWidth: ui.u(46)
                        implicitHeight: ui.u(46)
                        hoverEnabled: true

                        // Invisible background retains the original touch/click target.
                        background: Item {}

                        contentItem: MediaIcon {
                            opacity: fwdBtn.down ? 0.75 : 1
                            name: "forward"
                            color: fwdBtn.hovered ? "#00F0FF" : "#CBD5E1"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            iconSize: ui.u(22)
                        }

                        onClicked: playerController.seekRelative(10)
                    }

                    // 5. Next Track Button
                    Button {
                        id: nextBtn
                        padding: ui.u(6)
                        anchors.verticalCenter: parent.verticalCenter
                        Accessible.name: qsTr("Next track")
                        implicitWidth: ui.u(46)
                        implicitHeight: ui.u(46)
                        hoverEnabled: true

                        // Invisible background retains the original touch/click target.
                        background: Item {}

                        contentItem: MediaIcon {
                            opacity: nextBtn.down ? 0.75 : 1
                            name: "next"
                            color: nextBtn.hovered ? "#00F0FF" : "#CBD5E1"
                            Behavior on color { ColorAnimation { duration: 120 } }
                            iconSize: ui.u(22)
                        }

                        onClicked: playerController.next()
                    }
                }
            }
        }
    }
}
