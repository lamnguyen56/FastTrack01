# Stateful V4L2 codecs for bcm2835-codec on Pi 3 (not stateless v4l2codecs).
PACKAGECONFIG:append:raspberrypi3-64 = " v4l2"

# Register GstGLVideoItem and build the Qt5 OpenGL sink.
PACKAGECONFIG:append:raspberrypi3-64 = " qt5"
# EGLFS owns the display; qmlglsink does not need Qt Wayland integration.
QT5WAYLANDDEPENDS:raspberrypi3-64 = ""
EXTRA_OEMESON:append:raspberrypi3-64 = " -Dqt-egl=enabled -Dqt-wayland=disabled -Dqt-x11=disabled"
