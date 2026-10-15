import QtQuick
import QtQuick.Controls
import QtQuick.Layouts

ApplicationWindow {
    id: rootWindow
    visible: true
    width: 1024
    height: 600
    minimumWidth: 900
    minimumHeight: 520
    title: qsTr("FastTrack Automotive Infotainment")
    color: "#0B0E14" // Deep Cockpit Dark

    // State properties for interactive demo
    property bool isPlaying: false
    property int currentTrackIndex: 0
    property int currentProgress: 35
    property bool isMuted: false

    // Lắng nghe tín hiệu CAN từ Tầng Platform (CanIpcClient)
    Connections {
        target: canClient
        function onCommandPlay() {
            rootWindow.isPlaying = true
        }
        function onCommandPause() {
            rootWindow.isPlaying = false
        }
        function onCommandNext() {
            rootWindow.currentTrackIndex = (rootWindow.currentTrackIndex + 1) % 5
        }
        function onCommandPrev() {
            rootWindow.currentTrackIndex = (rootWindow.currentTrackIndex - 1 + 5) % 5
        }
        function onCommandSeek(seconds) {
            rootWindow.currentProgress = Math.max(0, Math.min(100, rootWindow.currentProgress + seconds))
        }
    }

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
                Layout.preferredWidth: usbLayout.width + 20
                radius: 15
                color: "#1A202C"
                border.color: "#FF4D4F"
                border.width: 1

                RowLayout {
                    id: usbLayout
                    anchors.centerIn: parent
                    spacing: 6

                    Rectangle {
                        width: 8
                        height: 8
                        radius: 4
                        color: "#FF4D4F"
                    }

                    Text {
                        text: "USB: DISCONNECTED"
                        color: "#FFA39E"
                        font.pixelSize: 11
                        font.bold: true
                        font.letterSpacing: 0.5
                    }
                }
            }

            // CAN Bus Status Pill Badge
            Rectangle {
                height: 30
                Layout.preferredWidth: canLayout.width + 20
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
                        text: (typeof canClient !== "undefined" && canClient.connected) ? "CAN: can0 500k ACTIVE" : "CAN: WAITING DAEMON"
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

            // Central Video Placeholder Graphics
            ColumnLayout {
                anchors.centerIn: parent
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
                        onClicked: rootWindow.isPlaying = !rootWindow.isPlaying
                    }
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: rootWindow.isPlaying ? "ĐANG PHÁT VIDEO..." : "VIDEO PLAYBACK SURFACE"
                    color: rootWindow.isPlaying ? "#00F0FF" : "#A0AEC0"
                    font.bold: true
                    font.pixelSize: 15
                    font.letterSpacing: 1
                }

                Text {
                    Layout.alignment: Qt.AlignHCenter
                    text: "[ Chờ lệnh phát từ CANoe / Cắm USB để tải danh sách ]"
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
                width: currentTrackText.width + 20
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
                        text: playlistModel.get(rootWindow.currentTrackIndex).title
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
                        width: countText.width + 12
                        radius: 10
                        color: "#1B2436"
                        Text {
                            id: countText
                            anchors.centerIn: parent
                            text: "3 files"
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

                    model: ListModel {
                        id: playlistModel
                        ListElement { title: "demo_video_01.mp4"; duration: "03:45" }
                        ListElement { title: "demo_video_02.mp4"; duration: "05:12" }
                        ListElement { title: "automotive_h264.mkv"; duration: "02:30" }
                    }

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
                                    text: "H.264 / 1080p"
                                    color: "#64748B"
                                    font.pixelSize: 10
                                }
                            }

                            Text {
                                text: model.duration
                                color: "#8E9EB5"
                                font.pixelSize: 11
                            }
                        }

                        MouseArea {
                            id: mouseArea
                            anchors.fill: parent
                            hoverEnabled: true
                            cursorShape: Qt.PointingHandCursor
                            onClicked: {
                                rootWindow.currentTrackIndex = index
                                rootWindow.isPlaying = true
                            }
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
                    text: "01:15"
                    color: "#00F0FF"
                    font.bold: true
                    font.pixelSize: 12
                    font.family: "Consolas"
                }

                Slider {
                    id: progressSlider
                    Layout.fillWidth: true
                    from: 0
                    to: 100
                    value: rootWindow.currentProgress

                    onMoved: rootWindow.currentProgress = value

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
                    text: playlistModel.get(rootWindow.currentTrackIndex).duration
                    color: "#8E9EB5"
                    font.bold: true
                    font.pixelSize: 12
                    font.family: "Consolas"
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

                    onClicked: {
                        if (rootWindow.currentTrackIndex > 0)
                            rootWindow.currentTrackIndex--
                        else
                            rootWindow.currentTrackIndex = playlistModel.count - 1
                    }
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

                    onClicked: {
                        rootWindow.currentProgress = Math.max(0, rootWindow.currentProgress - 10)
                    }
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

                    onClicked: {
                        rootWindow.isPlaying = !rootWindow.isPlaying
                    }
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

                    onClicked: {
                        rootWindow.currentProgress = Math.min(100, rootWindow.currentProgress + 10)
                    }
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

                    onClicked: {
                        if (rootWindow.currentTrackIndex < playlistModel.count - 1)
                            rootWindow.currentTrackIndex++
                        else
                            rootWindow.currentTrackIndex = 0
                    }
                }
            }
        }
    }
}
