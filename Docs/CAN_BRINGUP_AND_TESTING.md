# CAN Bring-up and Testing on Raspberry Pi

The STM32F4 button-to-CAN-to-UI path has been verified. This guide covers Pi-side checks using the existing STM32 firmware and wiring.

[Hardware connections](diagrams/can-hardware-connections.mmd) · [Command sequence](diagrams/can-command-sequence.mmd) · [Protocol](INTERFACE_SPECIFICATION.md)

## Configuration

| Item | Value or owner |
|---|---|
| Controller | MCP2515 on the RS485 CAN HAT; SPI CAN is independent of RS485 UART |
| SPI | SPI0, CE0; IRQ GPIO25 |
| Oscillator configured in recipe | `8000000` Hz; must match the actual module |
| Bitrate | 500000 bit/s |
| Bus-off restart | 100 ms |
| Driver | `CONFIG_CAN_MCP251X=y`, built into the kernel |

The overlay is configured in [rpi-config_git.bbappend](../Yocto/meta-usb-video-player/recipes-bsp/bootfiles/rpi-config_git.bbappend); kernel options are in [platform.cfg](../Yocto/meta-usb-video-player/recipes-kernel/linux/files/platform.cfg). `can-daemon.service` runs [setup_can.sh](../Yocto/meta-usb-video-player/recipes-platform/can-daemon/files/setup_can.sh) before starting the daemon.

A built-in driver does not appear in `lsmod`. A custom DTS is unnecessary when the BSP's existing MCP2515 overlay matches the hardware.

## 1. Check the interface on the Pi

```sh
ip -details -statistics link show can0
systemctl status can-daemon.service --no-pager -l
dmesg | grep -Ei 'mcp251|spi|can'
```

Expect bitrate 500000, no BUS-OFF state and no continuously increasing error counters.

## 2. Check frames from STM32

```sh
candump can0,100:7FF
```

Press each STM32 button and compare frame `0x100` with [INTERFACE_SPECIFICATION.md](INTERFACE_SPECIFICATION.md). This verifies physical reception, not application behavior.

## 3. Check daemon-to-application control

Prepare a USB drive with a verified 720p video, start the application and view logs:

```sh
journalctl -u can-daemon.service -f -o cat
```

In another terminal:

```sh
journalctl -u usb-video-player.service -f -o cat
```

Check Play, Pause, Next, Previous and both Seek directions on the UI. The daemon does not log every frame; use `candump` for traffic and application logs for checksum/playback errors.

To test the local receive path, send a Next command from the Pi:

```sh
cansend can0 100#0300000205000000
```

The daemon can receive locally sent frames through SocketCAN loopback. This does not replace the STM32-to-Pi test; physical transmission still needs a peer to acknowledge frames.

## 4. Troubleshooting

| Symptom | Check first |
|---|---|
| No `can0` | Overlay, oscillator, SPI/CS/IRQ wiring and MCP2515 probe logs |
| BUS-OFF or increasing RX/TX errors | Both bitrates, CANH/CANL, common ground and termination |
| Frames arrive but UI does not respond | ID/DLC, checksum, IPC sockets and application state |
| `Checksum mismatch` | Byte 4 must equal the sum of bytes 0–3 modulo 256 |
| No `0x101` status | Confirm that the application calls `sendStatus()`; no automatic status timer is connected |

Change configuration only after identifying the cause. Supply voltage or SPI frequency changes are not universal fixes for probe errors.

[System checklist](SYSTEM_TESTING.md) · [Build and deployment](BUILD_AND_DEPLOYMENT.md)
