# Keep Qt context wrapping compatible with the image's GStreamer GL API.
FILESEXTRAPATHS:prepend := "${THISDIR}/files:"
SRC_URI += " \
    file://0001-gst-use-explicit-api-for-wrapped-gl-context.patch \
    file://0002-gst-share-qt-display-in-context-queries.patch \
    file://0003-gst-negotiate-2d-textures-for-qt-quick.patch \
    file://0004-quick-defer-last-frame-gl-snapshot-until-flush.patch \
"
