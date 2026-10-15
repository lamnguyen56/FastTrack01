SUMMARY = "UBS Automotive Video Player Reference Image (Scarthgap)"
DESCRIPTION = "Target Linux image combining Platform Daemons (CAN, USB) and Qt Cockpit HMI"
LICENSE = "MIT"

inherit core-image

# Cấu hình tính năng của image: hỗ trợ đồ họa splash, ssh (dropbear), gstreamer, systemd
IMAGE_FEATURES += " \
    splash \
    ssh-server-dropbear \
    package-management \
    hwcodecs \
"

# ==============================================================================
# DANH SÁCH GÓI CÀI ĐẶT TRÊN IMAGE (PLATFORM + APP + MULTIMEDIA)
# ==============================================================================
IMAGE_INSTALL:append = " \
    \
    /* --- TẦNG NỀN TẢNG (PLATFORM DAEMONS & DRIVERS) --- */ \
    can-daemon \
    usb-monitor \
    can-utils \
    iproute2 \
    udev \
    util-linux \
    dosfstools \
    e2fsprogs \
    \
    /* --- TẦNG ỨNG DỤNG (QT HMI APPLICATION) --- */ \
    ubs-video-player \
    \
    /* --- THƯ VIỆN ĐỒ HỌA & RUNTIME QT --- */ \
    qtbase \
    qtbase-plugins \
    qtdeclarative \
    qtdeclarative-qmlplugins \
    qtmultimedia \
    qtmultimedia-plugins \
    qtmultimedia-qmlplugins \
    \
    /* --- HỆ THỐNG MULTIMEDIA GSTREAMER (HARDWARE ACCEL DECODING) --- */ \
    gstreamer1.0 \
    gstreamer1.0-plugins-base \
    gstreamer1.0-plugins-good \
    gstreamer1.0-plugins-bad \
    gstreamer1.0-libav \
    \
    /* --- SYSTEM & DEBUGGING TOOLS --- */ \
    systemd \
    htop \
    nano \
"

# Đảm bảo phân vùng rootfs có đủ dung lượng chứa video cache và Qt libraries
IMAGE_ROOTFS_EXTRA_SPACE = "524288"
