FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:raspberrypi3-64 = " \
    file://platform.cfg \
"
