FILESEXTRAPATHS:prepend := "${THISDIR}/files:"

SRC_URI:append:raspberrypi3-64 = " \
    file://platform.cfg \
"

# Probe the VideoCore codec before Qt/GStreamer scans available decoders.
KERNEL_MODULE_AUTOLOAD:append:raspberrypi3-64 = " bcm2835-codec"
