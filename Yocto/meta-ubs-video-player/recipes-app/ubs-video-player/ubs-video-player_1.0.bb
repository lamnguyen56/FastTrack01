SUMMARY = "UBS Automotive Video Player - Qt HMI Application"
DESCRIPTION = "Cockpit HMI video player built with Qt/QML, communicates with can-daemon via Unix Domain Socket"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# App/ is maintained at the FastTrack01 repository root.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:${THISDIR}/../../recipes-platform/common/files:${THISDIR}/../../../..:"

SRC_URI = " \
    file://App/ \
    file://Constants.h \
    file://ubs-video-player.service \
"

S = "${WORKDIR}/App"

inherit cmake_qt5 systemd

DEPENDS = " \
    qtdeclarative \
    qtmultimedia \
"

RDEPENDS:${PN} += " \
    qtbase-plugins \
    qtdeclarative-qmlplugins \
    qtquickcontrols2-qmlplugins \
    qtmultimedia-plugins \
    qtmultimedia-qmlplugins \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    can-daemon \
    usb-monitor \
"

SYSTEMD_SERVICE:${PN} = "ubs-video-player.service"
SYSTEMD_AUTO_ENABLE = "enable"

# Đảm bảo Constants.h sẵn sàng trong include path của App khi biên dịch
do_configure:prepend() {
    mkdir -p ${WORKDIR}/App/common
    cp ${WORKDIR}/Constants.h ${WORKDIR}/App/common/
}

do_install:append() {
    install -d ${D}${systemd_system_unitdir}
    install -m 0644 ${WORKDIR}/ubs-video-player.service ${D}${systemd_system_unitdir}/
}

FILES:${PN} += " \
    ${bindir}/ubs-video-player \
    ${systemd_system_unitdir}/ubs-video-player.service \
"
