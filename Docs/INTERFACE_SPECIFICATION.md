# Interface Specification

Shared definitions are in [Constants.h](../Yocto/meta-usb-video-player/recipes-common/common/files/Constants.h). [ProtocolCodec.cpp](../Yocto/meta-usb-video-player/recipes-platform/can-daemon/files/src/ProtocolCodec.cpp) converts between CAN frames and IPC packets.

## CAN command — ID `0x100`

STM32 to Pi, standard CAN, default bitrate 500 kbit/s. STM32 sends DLC 8; the Pi codec currently accepts DLC 5 through 8.

| Byte | Field | Value |
|---|---|---|
| 0 | Command | `0`: idle, `1`: Play, `2`: Pause, `3`: Next, `4`: Previous |
| 1 | Seek | `0`: none, `5`: forward, `6`: backward |
| 2 | Seek step | Seconds; the application uses 10 seconds when this is zero |
| 3 | Rolling counter | STM32 increments within `0..15` |
| 4 | Checksum | `(B0 + B1 + B2 + B3) & 0xFF` |
| 5–7 | Reserved | STM32 writes zero |

The application validates the checksum. The daemon codec checks ID/DLC and copies the packet. Duplicate suppression using the rolling counter is not implemented.

Example: Next with counter 2 is `03 00 00 02 05 00 00 00`.

## CAN status — ID `0x101`

Pi to STM32, DLC 8. This is the implemented packet format, not confirmation that the application transmits it periodically.

| Byte | Content |
|---|---|
| 0 | Playback: `0` stopped, `1` playing, `2` paused, `3` error |
| 1 | Playlist index |
| 2–3 | Current time in seconds, big-endian |
| 4–5 | Duration in seconds, big-endian |
| 6 | USB: `0` disconnected, `1` scanning, `2` ready |
| 7 | Alive counter, `0..255` |

`CanIpcClient::sendStatus()` and the IPC-to-CAN path exist. The application still needs to invoke this function and verify its transmission interval. `CAN_STATUS_INTERVAL_MS` is 100 ms, but the constant does not create a timer.

## CAN IPC on the Pi

| Direction | Receiving endpoint | Payload |
|---|---|---|
| `can-daemon` to application | `/tmp/qt_app_ipc.sock` | `CanCommandPacket`, 5 bytes |
| Application to `can-daemon` | `/tmp/can_ipc.sock` | `CanStatusPacket`, 8 bytes |

Unix domain datagram sockets carry packed structs between processes on the same Pi. The codec converts time fields to big-endian for CAN transmission. This path uses neither D-Bus nor POSIX message queues.

## USB IPC

| Item | Value |
|---|---|
| Mount point | `/media/usb0` |
| Playlist | `/tmp/usb_playlist.txt` |
| Format | UTF-8, one absolute path per line |
| Writer | `usb-monitor`, using atomic file replacement |
| Reader | `UsbPlaylistReader`, using file/directory watching and fallback polling |

The application accepts readable, nonsymlink files within the mounted USB filesystem, with extensions `mp4`, `mkv`, `avi`, `mov` or `webm`. File extension eligibility does not guarantee codec support. An empty playlist represents no media or a released USB session. Mount state is checked independently of playlist-file existence.

The current mount implementation uses VFAT in read-only mode. exFAT and NTFS are not supported by this mount path.

[Architecture](SYSTEM_ARCHITECTURE.md) · [CAN testing](CAN_BRINGUP_AND_TESTING.md)
