SUMMARY = "USB Monitor Daemon - Platform-level USB hotplug & media scanner"
DESCRIPTION = "Listens for USB insertion/removal via libudev, automounts and scans for media files"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

DEPENDS = "udev"

FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../common/files:"

SRC_URI = " \
    file://CMakeLists.txt \
    file://src/main.cpp \
    file://usb-monitor.service \
    file://Constants.h \
"

S = "${WORKDIR}"

inherit cmake systemd

SYSTEMD_SERVICE:${PN} = "usb-monitor.service"
SYSTEMD_AUTO_ENABLE = "enable"

RDEPENDS:${PN} += "eudev util-linux dosfstools exfat-utils"

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/usb-monitor.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += "${bindir}/* ${systemd_system_unitdir}/*"
