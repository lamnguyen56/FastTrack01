SUMMARY = "CAN Daemon - Platform-level SocketCAN bridge"
DESCRIPTION = "Configures can0 at 500kbps, bridges SocketCAN frames to Unix Domain Socket IPC for the Qt application"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../common/files:"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/ \
    file://can-daemon.service \
    file://setup_can.sh \
    file://Constants.h \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "can-daemon.service"
SYSTEMD_AUTO_ENABLE = "enable"

RDEPENDS:${PN} += "iproute2"

do_install:append() {
    install -d ${D}${bindir}
    install -m 0755 ${WORKDIR}/setup_can.sh ${D}${bindir}/setup_can.sh

    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/can-daemon.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/* ${systemd_system_unitdir}/*"
