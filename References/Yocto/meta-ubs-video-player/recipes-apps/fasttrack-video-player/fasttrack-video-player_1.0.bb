SUMMARY = "Automotive USB Video Player Controlled via CAN"
DESCRIPTION = "Qt5/QML In-Vehicle Infotainment Media Player with SocketCAN Interface"
HOMEPAGE = "https://github.com/fasttrack/fasttrack-video-player"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "qtbase qtdeclarative qtmultimedia qtserialbus gstreamer1.0 udev"

SRC_URI = " \
    file://src/ \
    file://fasttrack-video-player.service \
    file://setup_can.sh \
"

S = "${WORKDIR}/src"

inherit cmake_qt5 systemd

SYSTEMD_SERVICE:${PN} = "fasttrack-video-player.service"
SYSTEMD_AUTO_ENABLE = "enable"

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/setup_can.sh ${D}${bindir}/setup_can.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/fasttrack-video-player.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/* ${systemd_system_unitdir}/*"
RDEPENDS:${PN} += "qtbase-plugins qtdeclarative-qmlplugins qtmultimedia-plugins gstreamer1.0-plugins-good"
