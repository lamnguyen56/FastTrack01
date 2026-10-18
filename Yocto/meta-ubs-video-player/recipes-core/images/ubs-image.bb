LICENSE = "MIT"

# Reuse the minimal boot image; application recipes own runtime dependencies.
require recipes-core/images/core-image-minimal.bb

SUMMARY = "UBS USB/CAN/Qt minimal image"
DESCRIPTION = "Minimal Linux image with USB/CAN services and the Qt HMI"

IMAGE_INSTALL:append = " \
    ubs-video-player \
    can-utils \
    usbutils \
"

