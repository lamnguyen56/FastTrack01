# Wi-Fi and SSH on Raspberry Pi 3B

This recipe configures Wi-Fi for Pi 3B with its BCM43430 radio. Pi 3B+ needs a
different firmware package and is not covered by this configuration. Use a
2.4 GHz WPA2-Personal access point. Connect the laptop to the same LAN and disable
AP/client isolation if enabled.

## Components

| Component | Purpose |
|---|---|
| `wpa-supplicant` | Association and WPA2 authentication on wlan0 |
| `wifi-wlan0.service` | Starts and supervises wpa_supplicant at boot |
| `systemd-networkd` | Obtains Wi-Fi IPv4 through DHCP and assigns Ethernet a fixed IPv4 address |
| `systemd-resolved` | Resolves names using the network's DNS configuration |
| `20-wlan0.network` | Selects wlan0 and enables IPv4 DHCP with its MAC as client ID |
| `25-eth0.network` | Assigns Ethernet `192.168.50.2/24` for a direct PC connection |
| `kernel-module-brcmfmac` | SDIO Wi-Fi driver, enabled by the BSP kernel |
| `kernel-module-brcmfmac-wcc` | Firmware vendor support loaded dynamically by the driver |
| `linux-firmware-bcm43430` | Radio firmware and board data |
| `wireless-regdb-static` | Kernel-loaded regulatory.db and signature |

`usb-image.bb` installs `wifi-config` and enables OpenSSH/SFTP through
`ssh-server-openssh`. Wi-Fi uses DHCP; Ethernet is statically configured at
`192.168.50.2/24` without a gateway so it can connect directly to a PC. USB and
CAN services do not wait for Wi-Fi to connect.

## Configure credentials before building

Run from the repository's `Yocto` directory on Ubuntu. The private override at
`meta-usb-video-player/recipes-connectivity/wifi/files/private/wpa_supplicant-wlan0.conf`
is already configured locally for the supplied access point and is Git-ignored.
It is not included in the public template or committed to the repository.
Protect the file with:

```sh
cd ~/projects/FastTrack01/Yocto
wifi_dir=meta-usb-video-player/recipes-connectivity/wifi
chmod 600 "$wifi_dir/files/private/wpa_supplicant-wlan0.conf"
```

To change networks later, edit the private file. Its format is:

```ini
country=VN
ctrl_interface=/run/wpa_supplicant
update_config=0

network={
    ssid="YOUR_SSID"
    psk="YOUR_PASSWORD"
    key_mgmt=WPA-PSK
    disabled=0
}
```

Use the regulatory country where the board operates. The quoted passphrase must
contain 8 to 63 characters; escape quotes and backslashes if present. The private
file has priority over the public template. Without it, the example network stays
disabled. The private directory is Git-ignored. Credentials are installed with
mode 0600 but remain in build output and the image; keep these artifacts private.

In an initialized Yocto build shell:

```sh
bitbake usb-image
```

Deploy the image to provision networking, firmware, modules and SSH. Later
executable-only updates can use SSH without reflashing.

## Verify on the Pi serial console

```sh
systemctl status wifi-wlan0.service systemd-networkd.service systemd-resolved.service --no-pager -l
wpa_cli -i wlan0 status
ip -4 addr show wlan0
cat /sys/class/net/wlan0/address
```

Expect `wpa_state=COMPLETED` and an IPv4 address. Ethernet should have the fixed
address `192.168.50.2` when the new image is running:

```sh
ip -4 addr show eth0
```

The project board currently uses
MAC `b8:27:eb:db:99:c4`; `192.168.1.30` is its observed DHCP address, not a guaranteed
address for every boot or board. A router DHCP reservation can keep the Wi-Fi
address stable for that MAC; Ethernet uses its separate fixed address.

Set a root password with `passwd` on the serial console or provision an SSH public
key. This development image uses debug-tweaks and does not set a fixed password.
Check the SSH socket/service if connections fail:

```sh
systemctl status sshd.socket sshd.service --no-pager -l
```

A socket-activated SSH service may start only when a client connects.

## Connect and deploy from the laptop

From `Yocto` on Ubuntu, connect using the Pi's Ethernet address:

```sh
bash tools/ssh.sh 192.168.50.2
```

Connect the PC's Ethernet port directly to the Pi. Configure the PC Ethernet
interface as `192.168.50.1/24` (no gateway) in Ubuntu's network settings. The Pi
and PC then use the fixed addresses `192.168.50.2` and `192.168.50.1` respectively.
In Ubuntu, open **Settings → Network → Wired → gear icon → IPv4**, choose
**Manual**, enter address `192.168.50.1` and netmask `255.255.255.0`, leave gateway
and DNS empty, then apply. The target must have booted the newly built image with
OpenSSH enabled. Set a root password with `passwd` on the Pi or provision an SSH
public key.

After initializing the build shell, deploy a selected process from the build
directory:

```sh
bash ../tools/deploy-process.sh usb 192.168.50.2
bash ../tools/deploy-process.sh can 192.168.50.2
bash ../tools/deploy-process.sh --logs app 192.168.50.2
```

See [the tools README](../../../tools/README.md#deploy-one-process) for the full
workflow, backups, rollback and binary-only deployment limits.

Omitting the address makes `ssh.sh` resolve `PI_MAC` from Ubuntu's IPv4 neighbor
table. For a direct Ethernet connection, passing `192.168.50.2` is predictable
and avoids MAC discovery. See
[SSH access](../../../tools/README.md#ssh-access) for MAC overrides and details.

## Change Wi-Fi on an installed Pi

Using the serial console, edit:

```sh
vi /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
chmod 600 /etc/wpa_supplicant/wpa_supplicant-wlan0.conf
systemctl restart wifi-wlan0.service
```

The service reconnects on reboot. Restarting it interrupts any Wi-Fi SSH session.
Target edits survive reboot but are replaced by a new image flash. Update the
private build configuration too if subsequent images should use the new network.

## Troubleshooting

### wlan0 does not exist

```sh
ip link show
dmesg | grep -Ei 'brcm|firmware|cfg80211|mmc'
lsmod | grep brcmfmac
```

The BSP enables CONFIG_BRCMFMAC=m and CONFIG_BRCMFMAC_SDIO=y. The WCC vendor module
is loaded at runtime and is not inferred as a normal module dependency. Errors
such as `mod=wcc: failed` and `brcmf_fwvid_attach failed` require checking that
`kernel-module-brcmfmac-wcc` is installed. For `regulatory.db` not found, check
`wireless-regdb-static`; `wireless-regdb` alone contains the legacy CRDA database.

### wlan0 exists but remains disconnected

```sh
wpa_cli -i wlan0 list_networks
journalctl -u wifi-wlan0.service -b -n 50 --no-pager
wpa_cli -i wlan0 scan
sleep 3
wpa_cli -i wlan0 scan_results
```

If the configured network is `[DISABLED]`, set `disabled=0` in the configuration
and restart the Wi-Fi service. Otherwise check SSID, WPA2 password, AP signal and
2.4 GHz availability. Do not publish the configuration file with its password.

### Wi-Fi is completed but no IPv4 address is assigned

```sh
networkctl status wlan0
journalctl -u systemd-networkd.service -b -n 50 --no-pager
ip -4 addr show wlan0
```

Check that the AP provides DHCP and that `/etc/systemd/network/20-wlan0.network`
is installed. Authentication and DHCP are separate steps; a completed WPA state
does not by itself prove DHCP succeeded.

### IP exists but SSH fails

Check the laptop's network, AP client isolation, SSH socket/service and root
credentials. For Ethernet, check `ip -4 addr show eth0` on the Pi and ensure the
PC Ethernet interface is `192.168.50.1/24`. Confirm SSH access with
`ssh root@192.168.50.2`.
