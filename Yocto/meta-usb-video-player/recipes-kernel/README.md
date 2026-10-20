# Raspberry Pi 3B kernel and CAN boot configuration

Target: `raspberrypi3-64`, BSP `linux-raspberrypi` 6.6.

## Kernel

`linux/linux-raspberrypi_6.6.bbappend` applies `files/platform.cfg`. It builds CAN RAW, MCP2515, SPI BCM2835, USB host/storage and FAT32 support into the kernel. POSIX queues are enabled for planned IPC; current daemons still use Unix sockets and playlist files.

Use the stock BSP `mcp2515-can0.dtbo`. No custom DTS or kernel driver patch is needed. Pin mapping: SPI0 CE0 (GPIO8), MOSI GPIO10, MISO GPIO9, SCLK GPIO11, IRQ GPIO25 (physical pin 22). RS485 is independent.

## Oscillator must be confirmed

Edit `recipes-bsp/bootfiles/rpi-config_git.bbappend` and replace the empty `CAN_OSCILLATOR:raspberrypi3-64` with the actual HAT crystal frequency in Hz. For a **confirmed** 8 MHz crystal:

```bitbake
CAN_OSCILLATOR:raspberrypi3-64 = "8000000"
```

With the value empty, the CAN overlay is deliberately disabled and boot-config deployment logs a warning. SPI remains enabled. Positive numeric clock values enable the stock CAN0 overlay with GPIO25. Dual CAN is disabled. Clock frequency is independent of the 500 kbit/s bus bitrate configured by the CAN service.

Do not also load a custom MCP2515 overlay for the same CS. Reference files under `References/Hardware_BSP` are not consumed by these recipes.

## Build and target checks

```bash
bitbake -c kernel_configcheck virtual/kernel
bitbake usb-image
```

Check final kernel `.config`, the deployed `overlays/mcp2515-can0.dtbo`, and boot `config.txt`. After setting the oscillator, the CAN overlay line must contain the confirmed frequency and `interrupt=25`.

On the Pi:

```bash
dmesg | grep -Ei 'mcp251|spi|can'
systemctl status can-daemon usb-monitor
journalctl -u can-daemon -u usb-monitor -b
ip -details -statistics link show can0
candump can0
```

`setup_can.sh` waits up to 30 seconds for the interface, configures it once per service start, and fails explicitly when missing/configuration fails. systemd retries after 5 seconds with no start-rate limit. An absent controller does not prove an application error; check the overlay, physical wiring and clock first. Interface UP is not evidence of successful physical CAN RX/TX; test with the ESP32 at the same bitrate.

USB baseline: one USB partition, FAT32, read-only at `/media/usb0`. See `../recipes-platform/README.md` for daemon behavior and limitations.
