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
| `systemd-networkd` | Obtains an IPv4 address and DNS information through DHCP |
| `systemd-resolved` | Resolves names using the network's DNS configuration |
| `20-wlan0.network` | Selects wlan0 and enables IPv4 DHCP with its MAC as client ID |
| `kernel-module-brcmfmac` | SDIO Wi-Fi driver, enabled by the BSP kernel |
| `kernel-module-brcmfmac-wcc` | Firmware vendor support loaded dynamically by the driver |
| `linux-firmware-bcm43430` | Radio firmware and board data |
| `wireless-regdb-static` | Kernel-loaded regulatory.db and signature |

`ubs-image.bb` installs `wifi-config` and enables OpenSSH/SFTP through
`ssh-server-openssh`. DHCP/DNS use the existing systemd build configuration.
USB and CAN services do not wait for Wi-Fi to connect.

## Configure credentials before building

Run from the repository's `Yocto` directory in WSL. Create a private override
without overwriting an existing one:

```sh
cd ~/projects/FastTrack01/Yocto
wifi_dir=meta-ubs-video-player/recipes-connectivity/wifi
mkdir -p "$wifi_dir/files/private"
if [ ! -e "$wifi_dir/files/private/wpa_supplicant-wlan0.conf" ]; then
    cp "$wifi_dir/files/wpa_supplicant-wlan0.conf" "$wifi_dir/files/private/"
fi
chmod 600 "$wifi_dir/files/private/wpa_supplicant-wlan0.conf"
nano "$wifi_dir/files/private/wpa_supplicant-wlan0.conf"
```

Replace `ssid` and `psk`, and change `disabled=1` to `disabled=0`:

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
bitbake ubs-image
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

Expect `wpa_state=COMPLETED` and an IPv4 address. The project board currently uses
MAC `b8:27:eb:db:99:c4`; `192.168.1.30` is its observed DHCP address, not a guaranteed
address for every boot or board. A router DHCP reservation can keep the address
stable for that MAC. The Pi itself continues to use DHCP.

Set a root password with `passwd` on the serial console or provision an SSH public
key. This development image uses debug-tweaks and does not set a fixed password.
Check the SSH socket/service if connections fail:

```sh
systemctl status sshd.socket sshd.service --no-pager -l
```

A socket-activated SSH service may start only when a client connects.

## Connect and deploy from the laptop

From `Yocto` in WSL, use the Pi's actual address:

```sh
bash tools/ssh.sh 192.168.1.30
```

After initializing the build shell, deploy a selected process from the build
directory:

```sh
bash ../tools/deploy-process.sh usb 192.168.1.30
bash ../tools/deploy-process.sh can 192.168.1.30
bash ../tools/deploy-process.sh --logs app 192.168.1.30
```

See [the tools README](../../../tools/README.md#deploy-one-process) for the full
workflow, backups, rollback and binary-only deployment limits.

Omitting the address makes `ssh.sh` resolve `PI_MAC` from the Windows ARP cache
through PowerShell. This is cache lookup, not an active subnet scan. It depends
on Windows interop and an existing entry. Explicit IP connections bypass this
Windows dependency. See [SSH access](../../../tools/README.md#ssh-access) for
MAC overrides and WSL troubleshooting.

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
credentials. For WSL `Exec format error` or `UtilAcceptVsock`, use an explicit IP
with ssh.sh; these errors concern Windows interop during MAC lookup, not the
Pi's SSH server. Confirm with a direct `ssh root@192.168.1.30` attempt.
