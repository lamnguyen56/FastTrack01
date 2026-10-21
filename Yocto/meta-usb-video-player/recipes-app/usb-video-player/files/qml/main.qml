import QtQuick 2.15
import QtQuick.Window 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

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
        anchors.bottom: controlBar.top
        anchors.bottomMargin: ui.u(12)
        width: Math.min(rootWindow.width * 0.80, ui.u(640))
        unitScale: ui.scale
        z: 100
    }

    Connections {
        target: usbReader
        function onDisconnected() { usbNotice.show() }
        function onMountedChanged() {
            if (usbReader.mounted) {
                usbNotice.dismiss()
            }
        }
    }

    // All state comes from the actual USB model and media engine.
    readonly property bool isPlaying: playerController.playing
    readonly property int currentTrackIndex: playerController.currentIndex
    readonly property real currentProgress: playerController.position

    // ==========================================
    // 1. HEADER BAR - COCKPIT HUD STYLE
    // ==========================================
    Rectangle {
        id: headerBar
        anchors.top: parent.top
        anchors.left: parent.left
        anchors.right: parent.right
        height: ui.headerHeight
        color: "#121620"

        // Bottom border gradient
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: ui.u(1)
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.3; color: "#00F0FF" }
                GradientStop { position: 0.7; color: "#0077B6" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Item {
            anchors.fill: parent
            anchors.leftMargin: ui.u(20)
            anchors.rightMargin: ui.u(20)

            // Automotive System Title
            RowLayout {
                anchors.left: parent.left
                anchors.top: ui.portrait ? parent.top : undefined
                anchors.topMargin: ui.u(12)
                anchors.verticalCenter: ui.portrait ? undefined : parent.verticalCenter
                spacing: ui.u(10)
                Rectangle {
                    Layout.preferredWidth: ui.u(32)
                    Layout.preferredHeight: ui.u(32)
                    radius: ui.u(8)
                    color: "#1A2333"
                    border.color: "#00F0FF"
                    border.width: ui.u(1)

                    MediaIcon {
                        anchors.centerIn: parent
                        name: "car"
                        color: "#00F0FF"
                        iconSize: ui.u(22)
                    }
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "FASTTRACK IVI MEDIA"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: ui.u(15)
                        font.letterSpacing: ui.u(1.5)
                    }
                    Text {
                        text: "CAN BUS CONTROLLED SYSTEM"
                        color: "#00F0FF"
                        font.pixelSize: ui.u(10)
                        font.letterSpacing: ui.u(1)
                    }
                }
            }

            Row {
                anchors.right: parent.right
                anchors.bottom: ui.portrait ? parent.bottom : undefined
                anchors.bottomMargin: ui.u(12)
                anchors.verticalCenter: ui.portrait ? undefined : parent.verticalCenter
                spacing: ui.u(16)

                // USB Status Pill Badge
                Rectangle {
                    height: ui.u(30)
                    implicitWidth: usbLayout.implicitWidth + ui.u(20)
                    implicitHeight: ui.u(30)
                    radius: ui.u(15)
                    color: "#1A202C"
                    border.color: usbReader.mounted ? "#00F0FF" : "#FF4D4F"
                    border.width: ui.u(1)

                    Row {
                        id: usbLayout
                        anchors.centerIn: parent
                        spacing: ui.u(6)

                        Rectangle {
                            width: ui.u(8)
                            height: ui.u(8)
                            anchors.verticalCenter: parent.verticalCenter
                            radius: ui.u(4)
                            color: usbReader.mounted ? "#00F0FF" : "#FF4D4F"
                        }

                        Text {
                            text: usbReader.mounted ? "USB: MOUNTED" : "USB: DISCONNECTED"
                            color: usbReader.mounted ? "#00F0FF" : "#FFA39E"
                            font.pixelSize: ui.u(11)
                            font.bold: true
                            font.letterSpacing: ui.u(0.5)
                        }
                    }
                }

                // CAN Bus Status Pill Badge
                Rectangle {
                    height: ui.u(30)
                    implicitWidth: canLayout.implicitWidth + ui.u(20)
                    implicitHeight: ui.u(30)
                    radius: ui.u(15)
                    color: (typeof canClient !== "undefined" && canClient.connected) ? "#122A2F" : "#2A1810"
                    border.color: (typeof canClient !== "undefined" && canClient.connected) ? "#00F0FF" : "#FF9900"
                    border.width: ui.u(1)

                    Row {
                        id: canLayout
                        anchors.centerIn: parent
                        spacing: ui.u(6)

                        Rectangle {
                            width: ui.u(8)
                            height: ui.u(8)
                            anchors.verticalCenter: parent.verticalCenter
                            radius: ui.u(4)
                            color: (typeof canClient !== "undefined" && canClient.connected) ? "#00F0FF" : "#FF9900"

                            // Pulsing glow animation
                            SequentialAnimation on opacity {
                                loops: Animation.Infinite
                                NumberAnimation { from: 0.3; to: 1.0; duration: 800 }
                                NumberAnimation { from: 1.0; to: 0.3; duration: 800 }
                            }
                        }

                        Text {
                            text: (typeof canClient !== "undefined" && canClient.connected) ? "CAN: IPC LISTENING" : "CAN: IPC UNAVAILABLE"
                            color: (typeof canClient !== "undefined" && canClient.connected) ? "#00F0FF" : "#FF9900"
                            font.pixelSize: ui.u(11)
                            font.bold: true
                            font.letterSpacing: ui.u(0.5)
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // 2. MAIN CONTENT AREA
    // ==========================================
    GridLayout {
        id: mainContent
        columns: ui.portrait ? 1 : 2
        anchors.top: headerBar.bottom
        anchors.bottom: controlBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: ui.u(14)
        rowSpacing: ui.panelGap
        columnSpacing: ui.panelGap

        // --- Video Canvas Surface ---
        Rectangle {
            id: videoSurface
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            Layout.preferredWidth: ui.portrait ? mainContent.width : (mainContent.width - ui.panelGap) * (1 - ui.playlistWidthFraction)
            Layout.preferredHeight: ui.portrait ? (mainContent.height - ui.panelGap) * ui.portraitVideoFraction : mainContent.height
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: ui.u(12)
            color: "#05070A"
            border.color: "#1E2638"
            border.width: ui.u(1)
            clip: true

            Loader {
                id: videoLoader
                anchors.fill: parent
                anchors.margins: ui.u(2)
                source: qtMajorVersion >= 6 ? "qrc:/ui/VideoSurfaceQt6.qml" : "qrc:/ui/VideoSurfaceQt5.qml"
            }

            // Cockpit HUD Corner brackets
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left
                width: ui.u(16); height: ui.u(2); color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left
                width: ui.u(2); height: ui.u(16); color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right
                width: ui.u(16); height: ui.u(2); color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right
                width: ui.u(2); height: ui.u(16); color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left
                width: ui.u(16); height: ui.u(2); color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left
                width: ui.u(2); height: ui.u(16); color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.right: parent.right
                width: ui.u(16); height: ui.u(2); color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.right: parent.right
                width: ui.u(2); height: ui.u(16); color: "#00F0FF"
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

            // Top-left Track info overlay
            Rectangle {
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

        // --- Playlist Drawer (Automotive Dark Glass Card) ---
        Rectangle {
            Layout.minimumWidth: 0
            Layout.minimumHeight: 0
            Layout.preferredWidth: ui.portrait ? mainContent.width : (mainContent.width - ui.panelGap) * ui.playlistWidthFraction
            Layout.maximumWidth: Layout.preferredWidth
            Layout.preferredHeight: ui.portrait ? (mainContent.height - ui.panelGap) * (1 - ui.portraitVideoFraction) : mainContent.height
            Layout.fillHeight: true
            radius: ui.u(12)
            color: "#121622"
            border.color: "#1E2638"
            border.width: ui.u(1)

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: ui.u(14)
                spacing: ui.u(12)

                // Header Playlist
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "USB PLAYLIST"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: ui.u(13)
                        font.letterSpacing: ui.u(1)
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        implicitHeight: ui.u(20)
                        implicitWidth: countText.implicitWidth + ui.u(12)
                        radius: ui.u(10)
                        color: "#1B2436"
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: usbPlaylist.count + " files"
                            color: "#00F0FF"
                            font.pixelSize: ui.u(10)
                        }
                    }
                }

                // Playlist Items
                ListView {
                    id: playlistView
                    Layout.minimumWidth: 0
                    Layout.minimumHeight: 0
                    Layout.preferredWidth: ui.u(1)
                    Layout.preferredHeight: ui.u(1)
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: ui.u(8)

                    model: usbPlaylist

                    delegate: Rectangle {
                        id: itemCard
                        width: playlistView.width
                        height: ui.u(50)
                        radius: ui.u(8)
                        color: rootWindow.currentTrackIndex === index ? "#19283E" : (mouseArea.containsMouse ? "#182030" : "#151B27")
                        border.color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#242F42"
                        border.width: ui.u(1)

                        Behavior on color { ColorAnimation { duration: 150 } }

                        // Active Glowing Bar on Left
                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: ui.u(4)
                            width: ui.u(3)
                            radius: ui.u(1.5)
                            color: "#00F0FF"
                            visible: rootWindow.currentTrackIndex === index
                        }

                        // Fixed card geometry: long names and duration changes only elide text.
                        Item {
                            anchors.fill: parent
                            anchors.leftMargin: ui.u(12)
                            anchors.rightMargin: ui.u(12)

                            MediaIcon {
                                id: trackIcon
                                anchors.left: parent.left
                                anchors.verticalCenter: parent.verticalCenter
                                name: rootWindow.currentTrackIndex === index ? "play" : "folder"
                                color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#718096"
                                iconSize: ui.u(16)
                            }

                            Column {
                                anchors.left: trackIcon.right
                                anchors.leftMargin: ui.u(10)
                                anchors.right: trackTime.left
                                anchors.rightMargin: ui.u(10)
                                anchors.verticalCenter: parent.verticalCenter
                                spacing: ui.u(2)
                                Text {
                                    width: parent.width
                                    text: model.title
                                    color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#E2E8F0"
                                    font.bold: rootWindow.currentTrackIndex === index
                                    font.pixelSize: ui.u(12)
                                    elide: Text.ElideRight
                                }
                                Text {
                                    width: parent.width
                                    text: model.filePath
                                    elide: Text.ElideMiddle
                                    color: "#64748B"
                                    font.pixelSize: ui.u(10)
                                }
                            }

                            Text {
                                id: trackTime
                                anchors.right: parent.right
                                anchors.verticalCenter: parent.verticalCenter
                                width: ui.u(56)
                                horizontalAlignment: Text.AlignRight
                                elide: Text.ElideLeft
                                text: rootWindow.currentTrackIndex === index ? playerController.formatTime(playerController.duration) : "--:--"
                                color: "#8E9EB5"
                                font.pixelSize: ui.u(11)
                            }
                        }
                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: playerController.selectTrack(index)
                        }
                    }
                }
            }
        }
    }

    // ==========================================
    // 3. BOTTOM CONTROL BAR - AUTOMOTIVE COCKPIT
    // ==========================================
    Rectangle {
        id: controlBar
        anchors.bottom: parent.bottom
        anchors.left: parent.left
        anchors.right: parent.right
        height: ui.footerHeight
        color: "#10141E"

        // Independent bottom-right policy card; keep transport controls centered.
        Button {
            id: autoNextSwitch
            anchors.right: parent.right
            anchors.bottom: parent.bottom
            anchors.rightMargin: ui.u(16)
            anchors.bottomMargin: ui.u(10)
            width: ui.u(184)
            height: ui.u(50)
            padding: ui.u(10)
            checkable: true
            checked: playerController.autoAdvance
            hoverEnabled: true
            Accessible.name: qsTr("Automatically play next video")
            onToggled: playerController.autoAdvance = checked

            background: Rectangle {
                radius: ui.u(10)
                color: autoNextSwitch.checked ? "#102B35" : "#161E2C"
                border.width: ui.u(1)
                border.color: autoNextSwitch.checked ? "#00B4D8" : "#334155"
                opacity: autoNextSwitch.down ? 0.75 : 1
                Behavior on color { ColorAnimation { duration: 120 } }
            }

            contentItem: Item {
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

        // Top border with glowing accent
        Rectangle {
            id: topBorder
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: ui.u(1)
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: "#25334A" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        Item {
            anchors.fill: parent
            anchors.margins: ui.u(12)

            // Explicit footer row geometry avoids nested height-for-width recalculation.
            RowLayout {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.right: parent.right
                height: ui.u(20)
                spacing: ui.u(12)

                Text {
                    text: playerController.formatTime(playerController.position)
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
                    to: Math.max(1, playerController.duration)
                    enabled: usbReader.mounted && playerController.seekable
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
                    text: playerController.formatTime(playerController.duration)
                    color: "#8E9EB5"
                    font.bold: true
                    font.pixelSize: ui.u(12)
                    font.family: "DejaVu Sans Mono"
                }
            }

            // --- ICON-ONLY AUTOMOTIVE CONTROL BUTTONS ---
            Row {
                enabled: usbReader.mounted && usbPlaylist.count > 0
                opacity: enabled ? 1 : 0.4
                anchors.horizontalCenter: parent.horizontalCenter
                anchors.bottom: parent.bottom
                anchors.bottomMargin: ui.portrait ? ui.u(52) : 0
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

                    background: Rectangle {
                        radius: ui.u(23)
                        color: prevBtn.down ? "#0E1726" : (prevBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: prevBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: ui.u(1.5)
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: MediaIcon {
                        name: "previous"
                        color: prevBtn.hovered ? "#00F0FF" : "#CBD5E1"
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

                    background: Rectangle {
                        radius: ui.u(23)
                        color: rewBtn.down ? "#0E1726" : (rewBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: rewBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: ui.u(1.5)
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: MediaIcon {
                        name: "rewind"
                        color: rewBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        iconSize: ui.u(22)
                    }

                    onClicked: playerController.seekRelative(-10)
                }

                // 3. Central Glowing PLAY / PAUSE Button (Larger & Illuminated)
                Button {
                    id: playPauseBtn
                    enabled: playerController.errorText.length === 0
                    padding: ui.u(6)
                    anchors.verticalCenter: parent.verticalCenter
                    Accessible.name: rootWindow.isPlaying ? qsTr("Pause") : qsTr("Play")
                    implicitWidth: ui.u(58)
                    implicitHeight: ui.u(58)
                    hoverEnabled: true

                    background: Rectangle {
                        radius: ui.u(29)
                        gradient: Gradient {
                            orientation: Gradient.Vertical
                            GradientStop { position: 0.0; color: playPauseBtn.hovered ? "#00E5FF" : (playPauseBtn.down ? "#0088AA" : "#00B4D8") }
                            GradientStop { position: 1.0; color: playPauseBtn.hovered ? "#0077B6" : (playPauseBtn.down ? "#005577" : "#005080") }
                        }
                        border.color: "#FFFFFF"
                        border.width: ui.u(playPauseBtn.down ? 2 : 1.5)

                        // Neon Aura Ring
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + ui.u(12)
                            height: parent.height + ui.u(12)
                            radius: width / 2
                            color: "transparent"
                            border.color: "#00F0FF"
                            border.width: ui.u(1.5)
                            opacity: playPauseBtn.hovered ? 0.8 : (rootWindow.isPlaying ? 0.5 : 0.25)
                            Behavior on opacity { NumberAnimation { duration: 180 } }
                        }
                    }

                    contentItem: MediaIcon {
                        name: rootWindow.isPlaying ? "pause" : "play"
                        color: "#FFFFFF"
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

                    background: Rectangle {
                        radius: ui.u(23)
                        color: fwdBtn.down ? "#0E1726" : (fwdBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: fwdBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: ui.u(1.5)
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: MediaIcon {
                        name: "forward"
                        color: fwdBtn.hovered ? "#00F0FF" : "#CBD5E1"
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

                    background: Rectangle {
                        radius: ui.u(23)
                        color: nextBtn.down ? "#0E1726" : (nextBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: nextBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: ui.u(1.5)
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: MediaIcon {
                        name: "next"
                        color: nextBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        iconSize: ui.u(22)
                    }

                    onClicked: playerController.next()
                }
            }
        }
    }
}
