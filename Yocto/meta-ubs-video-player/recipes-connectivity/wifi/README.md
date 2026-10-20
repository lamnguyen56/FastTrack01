# Wi-Fi and SSH development on Raspberry Pi 3B

The image installs wpa_supplicant (association), systemd-networkd (IPv4 DHCP),
OpenSSH/SFTP, the brcmfmac and brcmfmac-wcc kernel modules, the static wireless regulatory database and BCM43430
firmware. This targets Pi 3B, not Pi 3B+ (which needs BCM43455 firmware).
No CAN or USB service waits for Wi-Fi to become available.

## Configure the access point before building

Pi 3B uses 2.4 GHz Wi-Fi. Use a WPA2-Personal access point with client/AP
isolation disabled so the laptop can reach the Pi. Both join the same LAN.

From this directory, create a local, Git-ignored configuration:

```sh
mkdir -p files/private
cp files/wpa_supplicant-wlan0.conf files/private/
chmod 600 files/private/wpa_supplicant-wlan0.conf
```

Edit `files/private/wpa_supplicant-wlan0.conf`: replace `ssid` and `psk` with
the actual values and change `disabled=1` to `disabled=0`. Keep `country=VN`
only when operating in Vietnam. The passphrase must contain 8 to 63 characters;
escape quotes and backslashes if present. This private file overrides the public
template. Without it the example network remains disabled.

Credentials are installed with mode 0600, but are still present in the generated
image and build output. Keep those artifacts private as well.

In the initialized Yocto build shell:

```sh
bitbake ubs-image
```

Deploy this image once to provision the kernel modules, firmware, Wi-Fi and SSH.
Subsequent daemon-only changes can be copied through SSH without reflashing.

## Check the first boot over the serial console

```sh
systemctl status wifi-wlan0.service systemd-networkd.service sshd.socket
journalctl -u wifi-wlan0.service -u systemd-networkd.service -b --no-pager
wpa_cli -i wlan0 status
ip -4 addr show wlan0
cat /sys/class/net/wlan0/address
```

Expect `wpa_state=COMPLETED` and an IPv4 address. If wlan0 is missing, inspect
`dmesg | grep -Ei 'brcm|firmware|cfg80211'` and `lsmod | grep brcmfmac`.
The driver is installed as a module and loaded by device discovery; the existing
BSP kernel already enables CONFIG_BRCMFMAC=m and CONFIG_BRCMFMAC_SDIO=y.

Use the wlan0 MAC printed above to locate the DHCP lease in the AP/router UI.
Reserve that IP for the MAC there. A MAC address alone does not resolve an IP;
`arp -a` only lists neighbours already present in the local cache.

Set a root password with `passwd` on the serial console before SSH login, or
provision an SSH public key. The current development image uses debug-tweaks;
this setup does not add any hard-coded password.

From the laptop (replace the example IP with the actual DHCP address):

```sh
ssh root@192.168.1.50
```

OpenSSH may listen through `sshd.socket` and start sshd on demand. If connection
fails, check `systemctl status sshd.socket sshd.service` and AP client isolation.

## Change Wi-Fi on an already provisioned Pi

Edit `/etc/wpa_supplicant/wpa_supplicant-wlan0.conf` on the Pi, then run:

```sh
chmod 600 /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
systemctl restart wifi-wlan0.service
```

The enabled service reconnects automatically after reboot. Restarting it over
Wi-Fi interrupts the SSH session; use the serial console when changing networks.

## Update only usb-monitor

Run the deployment script from an initialized Yocto build shell in WSL:

```sh
bash ../tools/deploy-usb-monitor.sh root@192.168.1.50

# Deploy and follow the service journal (Ctrl+C exits the log view):
bash ../tools/deploy-usb-monitor.sh --logs root@192.168.1.50
```

Replace the IP with the Pi's address. The script builds `usb-monitor`, reads its
actual recipe image directory from BitBake, checks for AArch64, uploads through
SCP, backs up the installed executable, and atomically replaces it. It stops the
player first if active and starts it again afterward. SSH authentication uses
your normal password/key configuration; SSH keys avoid repeated password prompts.

If deployment fails, the script attempts to restore the previous executable and
service state. It checks that usb-monitor remains active for two seconds after
starting; this does not replace functional USB tests. Inspect journal output and
plug/unplug a real USB drive after deployment.

The backup remains at `/usr/bin/usb-monitor.previous`. For manual rollback on Pi:

```sh
systemctl stop ubs-video-player.service
systemctl stop usb-monitor.service
cp -p /usr/bin/usb-monitor.previous /usr/bin/usb-monitor.new && mv /usr/bin/usb-monitor.new /usr/bin/usb-monitor
systemctl start usb-monitor.service
systemctl start ubs-video-player.service
```

Binary-only updates work when the installed libraries and ABI remain compatible.
Changes to shared libraries, kernel, device tree, runtime dependencies or service
units need the corresponding files/packages deployed too. A later image flash
replaces manual changes, so keep source changes in the recipe repository.

## Kernel 6.6 runtime dependencies

The WCC firmware vendor support is a separate module, loaded dynamically by
brcmfmac. Install kernel-module-brcmfmac-wcc as well as kernel-module-brcmfmac.
A missing WCC module can cause brcmf_fwvid_attach to fail before wlan0 appears.
Use wireless-regdb-static for the kernel-loaded regulatory.db and its signature;
the wireless-regdb package alone supplies the legacy CRDA database.

## Find the Pi by MAC

The deployment script reuses `tools/ssh.sh` when its destination is omitted:

```sh
bash ../tools/deploy-usb-monitor.sh --logs
```

The default MAC is `b8:27:eb:db:99:c4`. Override it if needed:

```sh
PI_MAC="b8:27:eb:db:99:c4" bash ../tools/deploy-usb-monitor.sh --logs
```

Use `bash ../tools/ssh.sh` for an interactive SSH session, or
`bash ../tools/ssh.sh --ip` to print only the discovered address. The resolver
reads Windows' ARP cache from WSL; it does not actively scan the subnet. The Pi
must be visible in that cache. Missing or ambiguous entries fail explicitly;
you can still pass an IP directly to deploy-usb-monitor.sh.
