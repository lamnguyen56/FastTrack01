SUMMARY = "Shared USB video player protocol definitions and logging headers"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

SRC_URI = "file://CMakeLists.txt file://Constants.h file://Logger.h"
S = "${WORKDIR}"

inherit cmake

# Header-only build dependency: headers and exported CMake target belong to -dev.
# No runtime package needs to be installed in the image.
ALLOW_EMPTY:${PN} = "1"
FILES:${PN}-dev += "${libdir}/cmake/Common"
