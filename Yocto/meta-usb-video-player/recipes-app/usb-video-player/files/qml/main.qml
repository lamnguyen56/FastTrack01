import QtQuick 2.15
import QtQuick.Controls 2.15
import QtQuick.Layouts 1.15

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1024
    height: 600
    minimumWidth: 900
    minimumHeight: 520
    title: qsTr("FastTrack Automotive Infotainment")
    color: "#0B0E14" // Deep Cockpit Dark

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
        height: 56
        color: "#121620"

        // Bottom border gradient
        Rectangle {
            anchors.bottom: parent.bottom
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.3; color: "#00F0FF" }
                GradientStop { position: 0.7; color: "#0077B6" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        RowLayout {
            anchors.fill: parent
            anchors.leftMargin: 20
            anchors.rightMargin: 20
            spacing: 16

            // Automotive System Title
            RowLayout {
                spacing: 10
                Rectangle {
                    width: 32
                    height: 32
                    radius: 8
                    color: "#1A2333"
                    border.color: "#00F0FF"
                    border.width: 1

                    Text {
                        anchors.centerIn: parent
                        text: "🏎"
                        font.pixelSize: 16
                    }
                }

                ColumnLayout {
                    spacing: 0
                    Text {
                        text: "FASTTRACK IVI MEDIA"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 15
                        font.letterSpacing: 1.5
                    }
                    Text {
                        text: "CAN BUS CONTROLLED SYSTEM"
                        color: "#00F0FF"
                        font.pixelSize: 10
                        font.letterSpacing: 1
                    }
                }
            }

            Item { Layout.fillWidth: true }

            // USB Status Pill Badge
            Rectangle {
                height: 30
                Layout.preferredWidth: usbLayout.implicitWidth + 20
                radius: 15
                color: "#1A202C"
                border.color: usbReader.mounted ? "#00F0FF" : "#FF4D4F"
                border.width: 1

                RowLayout {
                    id: usbLayout
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: usbReader.mounted ? "#00F0FF" : "#FF4D4F"
                    }

                    Text {
                        text: usbReader.mounted ? "USB: MOUNTED" : "USB: DISCONNECTED"
                        color: usbReader.mounted ? "#00F0FF" : "#FFA39E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 0.5
                    }
                }
            }

            // CAN Bus Status Pill Badge
            Rectangle {
                height: 30
                Layout.preferredWidth: canLayout.implicitWidth + 20
                radius: 15
                color: (typeof canClient !== "undefined" && canClient.connected) ? "#122A2F" : "#2A1810"
                border.color: (typeof canClient !== "undefined" && canClient.connected) ? "#00F0FF" : "#FF9900"
                border.width: 1

                RowLayout {
                    id: canLayout
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
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
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 0.5
                    }
                }
            }
        }
    }

    // ==========================================
    // 2. MAIN CONTENT AREA
    // ==========================================
    RowLayout {
        anchors.top: headerBar.bottom
        anchors.bottom: controlBar.top
        anchors.left: parent.left
        anchors.right: parent.right
        anchors.margins: 14
        spacing: 14

        // --- Video Canvas Surface ---
        Rectangle {
            id: videoSurface
            Layout.fillWidth: true
            Layout.fillHeight: true
            radius: 12
            color: "#05070A"
            border.color: "#1E2638"
            border.width: 1
            clip: true

            Loader {
                id: videoLoader
                anchors.fill: parent
                anchors.margins: 2
                source: qtMajorVersion >= 6 ? "qrc:/ui/VideoSurfaceQt6.qml" : "qrc:/ui/VideoSurfaceQt5.qml"
            }

            // Cockpit HUD Corner brackets
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left
                width: 16; height: 2; color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.left: parent.left
                width: 2; height: 16; color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right
                width: 16; height: 2; color: "#00F0FF"
            }
            Rectangle {
                anchors.top: parent.top; anchors.right: parent.right
                width: 2; height: 16; color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left
                width: 16; height: 2; color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.left: parent.left
                width: 2; height: 16; color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.right: parent.right
                width: 16; height: 2; color: "#00F0FF"
            }
            Rectangle {
                anchors.bottom: parent.bottom; anchors.right: parent.right
                width: 2; height: 16; color: "#00F0FF"
            }

            // Empty/loading/error state; decoded video is rendered underneath.
            ColumnLayout {
                anchors.centerIn: parent
                width: parent.width - 40
                visible: playerController.errorText.length > 0 || videoLoader.item === null || videoLoader.item.hasVideo === false
                spacing: 12

                Rectangle {
                    Layout.alignment: Qt.AlignHCenter
                    width: 72
                    height: 72
                    radius: 36
                    color: rootWindow.isPlaying ? "#003A4D" : "#141923"
                    border.color: rootWindow.isPlaying ? "#00F0FF" : "#324158"
                    border.width: 2

                    Text {
                        anchors.centerIn: parent
                        text: rootWindow.isPlaying ? "▶" : "⏸"
                        color: rootWindow.isPlaying ? "#00F0FF" : "#71829D"
                        font.pixelSize: 28
                    }

                    MouseArea {
                        anchors.fill: parent
                        cursorShape: Qt.PointingHandCursor
                        onClicked: playerController.toggle()
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: playerController.errorText.length > 0 ? "PLAYBACK ERROR" : (usbPlaylist.count > 0 ? "PRESS PLAY" : "NO MEDIA")
                    color: rootWindow.isPlaying ? "#00F0FF" : "#A0AEC0"
                    font.bold: true
                    font.pixelSize: 15
                    font.letterSpacing: 1
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: playerController.errorText.length > 0 ? playerController.errorText : (usbPlaylist.count > 0 ? "Select a video or press Play" : (usbReader.mounted ? "Waiting for playlist / no supported media files" : "Insert a USB drive containing videos"))
                    Layout.fillWidth: true
                    horizontalAlignment: Text.AlignHCenter
                    wrapMode: Text.Wrap
                    color: "#55657E"
                    font.pixelSize: 12
                }
            }

            // Top-left Track info overlay
            Rectangle {
                anchors.top: parent.top
                anchors.left: parent.left
                anchors.margins: 14
                height: 28
                width: currentTrackText.implicitWidth + 20
                radius: 6
                color: "#18203099"
                border.color: "#2D3748"
                border.width: 1

                RowLayout {
                    anchors.centerIn: parent
                    spacing: 6
                    Rectangle {
                        width: 6; height: 6; radius: 3
                        color: rootWindow.isPlaying ? "#00F0FF" : "#888888"
                    }
                    Text {
                        id: currentTrackText
                        text: playerController.currentTitle.length > 0 ? playerController.currentTitle : "No media selected"
                        color: "#E2E8F0"
                        font.pixelSize: 11
                    }
                }
            }
        }

        // --- Playlist Drawer (Automotive Dark Glass Card) ---
        Rectangle {
            Layout.preferredWidth: 300
            Layout.fillHeight: true
            radius: 12
            color: "#121622"
            border.color: "#1E2638"
            border.width: 1

            ColumnLayout {
                anchors.fill: parent
                anchors.margins: 14
                spacing: 12

                // Header Playlist
                RowLayout {
                    Layout.fillWidth: true
                    Text {
                        text: "USB PLAYLIST"
                        color: "#FFFFFF"
                        font.bold: true
                        font.pixelSize: 13
                        font.letterSpacing: 1
                    }
                    Item { Layout.fillWidth: true }
                    Rectangle {
                        height: 20
                        width: countText.implicitWidth + 12
                        radius: 10
                        color: "#1B2436"
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: usbPlaylist.count + " files"
                            color: "#00F0FF"
                            font.pixelSize: 10
                        }
                    }
                }

                // Playlist Items
                ListView {
                    id: playlistView
                    Layout.fillWidth: true
                    Layout.fillHeight: true
                    clip: true
                    spacing: 8

                    model: usbPlaylist

                    delegate: Rectangle {
                        id: itemCard
                        width: playlistView.width
                        height: 50
                        radius: 8
                        color: rootWindow.currentTrackIndex === index ? "#19283E" : (mouseArea.containsMouse ? "#182030" : "#151B27")
                        border.color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#242F42"
                        border.width: 1

                        Behavior on color { ColorAnimation { duration: 150 } }

                        // Active Glowing Bar on Left
                        Rectangle {
                            anchors.left: parent.left
                            anchors.top: parent.top
                            anchors.bottom: parent.bottom
                            anchors.margins: 4
                            width: 3
                            radius: 1.5
                            color: "#00F0FF"
                            visible: rootWindow.currentTrackIndex === index
                        }

                        RowLayout {
                            anchors.fill: parent
                            anchors.leftMargin: 12
                            anchors.rightMargin: 12
                            spacing: 10

                            Text {
                                text: rootWindow.currentTrackIndex === index ? "▶" : "📁"
                                color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#718096"
                                font.pixelSize: 12
                            }

                            ColumnLayout {
                                Layout.fillWidth: true
                                spacing: 2
                                Text {
                                    text: model.title
                                    color: rootWindow.currentTrackIndex === index ? "#00F0FF" : "#E2E8F0"
                                    font.bold: rootWindow.currentTrackIndex === index
                                    font.pixelSize: 12
                                    elide: Text.ElideRight
                                    Layout.fillWidth: true
                                }
                                Text {
                                    text: model.filePath
                                    elide: Text.ElideMiddle
                                    Layout.fillWidth: true
                                    color: "#64748B"
                                    font.pixelSize: 10
                                }
                            }

                            Text {
                                text: rootWindow.currentTrackIndex === index ? playerController.formatTime(playerController.duration) : "--:--"
                                color: "#8E9EB5"
                                font.pixelSize: 11
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
        height: 100
        color: "#10141E"

        // Top border with glowing accent
        Rectangle {
            id: topBorder
            anchors.top: parent.top
            anchors.left: parent.left
            anchors.right: parent.right
            height: 1
            gradient: Gradient {
                orientation: Gradient.Horizontal
                GradientStop { position: 0.0; color: "transparent" }
                GradientStop { position: 0.5; color: "#25334A" }
                GradientStop { position: 1.0; color: "transparent" }
            }
        }

        ColumnLayout {
            anchors.fill: parent
            anchors.margins: 12
            spacing: 8

            // --- Custom Automotive Progress Slider ---
            RowLayout {
                Layout.fillWidth: true
                spacing: 12

                Text {
                    text: playerController.formatTime(playerController.position)
                    color: "#00F0FF"
                    font.bold: true
                    font.pixelSize: 12
                    font.family: "DejaVu Sans Mono"
                }

                Slider {
                    id: progressSlider
                    Layout.fillWidth: true
                    from: 0
                    to: Math.max(1, playerController.duration)
                    enabled: playerController.seekable
                    value: rootWindow.currentProgress

                    onMoved: playerController.seekTo(value)

                    // Custom Track
                    background: Rectangle {
                        x: progressSlider.leftPadding
                        y: progressSlider.topPadding + progressSlider.availableHeight / 2 - height / 2
                        implicitWidth: 200
                        implicitHeight: 6
                        width: progressSlider.availableWidth
                        height: implicitHeight
                        radius: 3
                        color: "#1C2433"

                        // Active Progress Bar Gradient
                        Rectangle {
                            width: progressSlider.visualPosition * parent.width
                            height: parent.height
                            radius: 3
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
                        implicitWidth: 16
                        implicitHeight: 16
                        radius: 8
                        color: "#FFFFFF"
                        border.color: "#00F0FF"
                        border.width: 3

                        // Glowing hover effect ring
                        Rectangle {
                            anchors.centerIn: parent
                            width: 26
                            height: 26
                            radius: 13
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
                    font.pixelSize: 12
                    font.family: "DejaVu Sans Mono"
                }
            }

            // --- ICON-ONLY AUTOMOTIVE CONTROL BUTTONS ---
            RowLayout {
                Layout.alignment: Qt.AlignHCenter
                spacing: 22

                // 1. Previous Track Button
                Button {
                    id: prevBtn
                    implicitWidth: 46
                    implicitHeight: 46
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 23
                        color: prevBtn.down ? "#0E1726" : (prevBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: prevBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: 1.5
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: Text {
                        text: "⏮"
                        color: prevBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: playerController.previous()
                }

                // 2. Fast Backward -10s
                Button {
                    id: rewBtn
                    implicitWidth: 46
                    implicitHeight: 46
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 23
                        color: rewBtn.down ? "#0E1726" : (rewBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: rewBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: 1.5
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: Text {
                        text: "⏪"
                        color: rewBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: playerController.seekRelative(-10)
                }

                // 3. Central Glowing PLAY / PAUSE Button (Larger & Illuminated)
                Button {
                    id: playPauseBtn
                    implicitWidth: 58
                    implicitHeight: 58
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 29
                        gradient: Gradient {
                            orientation: Gradient.Vertical
                            GradientStop { position: 0.0; color: playPauseBtn.hovered ? "#00E5FF" : (playPauseBtn.down ? "#0088AA" : "#00B4D8") }
                            GradientStop { position: 1.0; color: playPauseBtn.hovered ? "#0077B6" : (playPauseBtn.down ? "#005577" : "#005080") }
                        }
                        border.color: "#FFFFFF"
                        border.width: playPauseBtn.down ? 2 : 1.5

                        // Neon Aura Ring
                        Rectangle {
                            anchors.centerIn: parent
                            width: parent.width + 12
                            height: parent.height + 12
                            radius: width / 2
                            color: "transparent"
                            border.color: "#00F0FF"
                            border.width: 1.5
                            opacity: playPauseBtn.hovered ? 0.8 : (rootWindow.isPlaying ? 0.5 : 0.25)
                            Behavior on opacity { NumberAnimation { duration: 180 } }
                        }
                    }

                    contentItem: Text {
                        text: rootWindow.isPlaying ? "⏸" : "▶"
                        color: "#FFFFFF"
                        font.pixelSize: rootWindow.isPlaying ? 24 : 22
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: playerController.toggle()
                }

                // 4. Fast Forward +10s
                Button {
                    id: fwdBtn
                    implicitWidth: 46
                    implicitHeight: 46
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 23
                        color: fwdBtn.down ? "#0E1726" : (fwdBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: fwdBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: 1.5
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: Text {
                        text: "⏩"
                        color: fwdBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: playerController.seekRelative(10)
                }

                // 5. Next Track Button
                Button {
                    id: nextBtn
                    implicitWidth: 46
                    implicitHeight: 46
                    hoverEnabled: true

                    background: Rectangle {
                        radius: 23
                        color: nextBtn.down ? "#0E1726" : (nextBtn.hovered ? "#1E2B40" : "#151D2A")
                        border.color: nextBtn.hovered ? "#00F0FF" : "#28354A"
                        border.width: 1.5
                        Behavior on color { ColorAnimation { duration: 120 } }
                        Behavior on border.color { ColorAnimation { duration: 120 } }
                    }

                    contentItem: Text {
                        text: "⏭"
                        color: nextBtn.hovered ? "#00F0FF" : "#CBD5E1"
                        font.pixelSize: 18
                        horizontalAlignment: Text.AlignHCenter
                        verticalAlignment: Text.AlignVCenter
                    }

                    onClicked: playerController.next()
                }
            }
        }
    }
}
