import QtQuick 2.15

/** Font-independent icons drawn in a centered 24-by-24 coordinate space. */
Canvas {
    id: icon

    /** Supported names: play, pause, previous, next, rewind, forward, folder, car. */
    property string name: "play"
    property color color: "#FFFFFF"
    property real iconSize: 24

    implicitWidth: iconSize
    implicitHeight: iconSize

    onNameChanged: requestPaint()
    onColorChanged: requestPaint()
    onIconSizeChanged: requestPaint()
    onWidthChanged: requestPaint()
    onHeightChanged: requestPaint()

    /** Fill a triangle; reversing its x coordinates points it to the left. */
    function triangle(context, left, right) {
        context.beginPath()
        context.moveTo(left, 5)
        context.lineTo(right, 12)
        context.lineTo(left, 19)
        context.closePath()
        context.fill()
    }

    onPaint: {
        var context = getContext("2d")
        context.clearRect(0, 0, width, height)

        // Keep controls centered even when Button stretches its content item.
        var size = Math.min(iconSize, width, height)
        if (size <= 0) {
            return
        }

        context.save()
        context.translate((width - size) / 2, (height - size) / 2)
        context.scale(size / 24, size / 24)
        context.fillStyle = color

        if (name === "play") {
            triangle(context, 6, 20)
        } else if (name === "pause") {
            context.fillRect(5, 5, 5, 14)
            context.fillRect(14, 5, 5, 14)
        } else if (name === "previous") {
            context.fillRect(4, 5, 3, 14)
            triangle(context, 20, 8)
        } else if (name === "next") {
            triangle(context, 4, 16)
            context.fillRect(17, 5, 3, 14)
        } else if (name === "rewind") {
            triangle(context, 12, 2)
            triangle(context, 22, 12)
        } else if (name === "forward") {
            triangle(context, 2, 12)
            triangle(context, 12, 22)
        } else if (name === "folder") {
            context.fillRect(3, 5, 8, 4)
            context.fillRect(3, 8, 18, 11)
        } else if (name === "car") {
            context.beginPath()
            context.moveTo(3, 11)
            context.lineTo(6, 5)
            context.lineTo(18, 5)
            context.lineTo(21, 11)
            context.closePath()
            context.fill()
            context.fillRect(2, 11, 20, 7)
            context.fillRect(4, 18, 4, 3)
            context.fillRect(16, 18, 4, 3)
        }

        context.restore()
    }
}
