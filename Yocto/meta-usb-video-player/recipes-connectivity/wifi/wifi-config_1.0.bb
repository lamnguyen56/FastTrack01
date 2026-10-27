SUMMARY = "Pi 3B Wi-Fi and Ethernet network configuration"
LICENSE = "MIT"
LIC_FILES_CHKSUM = "file://${COMMON_LICENSE_DIR}/MIT;md5=0835ade698e0bcf8506ecda2f7b4f302"

# Optional ignored credentials file takes precedence over the public template.
FILESEXTRAPATHS:prepend := "${THISDIR}/files/private:"

SRC_URI = "file://wifi-wlan0.service file://20-wlan0.network file://25-eth0.network file://wpa_supplicant-wlan0.conf"
S = "${WORKDIR}"

inherit systemd features_check
REQUIRED_DISTRO_FEATURES = "systemd wifi"
COMPATIBLE_MACHINE = "raspberrypi3-64"

# Kernel 6.6 loads the WCC vendor module at runtime; module dependency scanning
# does not infer it. The kernel also needs regulatory.db from the -static package.
RDEPENDS:${PN} = "wpa-supplicant wpa-supplicant-cli wpa-supplicant-passphrase systemd iw linux-firmware-bcm43430 wireless-regdb-static kernel-module-brcmfmac kernel-module-brcmfmac-wcc"

SYSTEMD_SERVICE:${PN} = "wifi-wlan0.service"
SYSTEMD_AUTO_ENABLE = "enable"
CONFFILES:${PN} = "${sysconfdir}/wpa_supplicant/wpa_supplicant-wlan0.conf ${sysconfdir}/systemd/network/20-wlan0.network ${sysconfdir}/systemd/network/25-eth0.network"

do_install() {
    install -d ${D}${systemd_system_unitdir} ${D}${sysconfdir}/systemd/network ${D}${sysconfdir}/wpa_supplicant
    install -m 0644 ${WORKDIR}/wifi-wlan0.service ${D}${systemd_system_unitdir}/
    install -m 0644 ${WORKDIR}/20-wlan0.network ${D}${sysconfdir}/systemd/network/
    install -m 0644 ${WORKDIR}/25-eth0.network ${D}${sysconfdir}/systemd/network/
    install -m 0600 ${WORKDIR}/wpa_supplicant-wlan0.conf ${D}${sysconfdir}/wpa_supplicant/
    sed -i -e 's|@SBINDIR@|${sbindir}|g' ${D}${systemd_system_unitdir}/wifi-wlan0.service
}

FILES:${PN} += "${systemd_system_unitdir}/wifi-wlan0.service"
