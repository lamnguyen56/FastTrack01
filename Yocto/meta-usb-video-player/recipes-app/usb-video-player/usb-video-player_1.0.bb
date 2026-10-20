SUMMARY = "USB Automotive Video Player - Qt HMI Application"
DESCRIPTION = "Qt/QML player with USB playlist and CAN IPC integration"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# All application sources are maintained inside this recipe's files directory.
SRC_URI = " \
    file://CMakeLists.txt \
    file://src/ \
    file://qml/ \
    file://usb-video-player.service \
    file://wait-for-display.sh \
"
S = "${WORKDIR}"

inherit cmake_qt5 systemd

DEPENDS = "common qtdeclarative qtmultimedia"

RDEPENDS:${PN} += " \
    qtbase-plugins \
    ttf-dejavu-sans \
    ttf-dejavu-sans-mono \
    qtdeclarative-qmlplugins \
    qtquickcontrols2-qmlplugins \
    qtmultimedia-plugins \
    qtmultimedia-qmlplugins \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    can-daemon \
    usb-monitor \
"

SYSTEMD_SERVICE:${PN} = "usb-video-player.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${bindir} ${D}${systemd_system_unitdir}
    install -m 0755 ${WORKDIR}/wait-for-display.sh ${D}${bindir}/
    install -m 0644 ${WORKDIR}/usb-video-player.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/usb-video-player ${bindir}/wait-for-display.sh ${systemd_system_unitdir}/usb-video-player.service"
