# System Testing and Test Records

Test the `main` branch: USB, CAN, UI and Qt Multimedia playback.

## Preparation

- Connect a display and a FAT32 USB drive containing verified H.264 720p videos.
- Use the working STM32F4 and CAN HAT configuration.
- Record the commit, image and deployed binary before each test run.

## Target checklist

| Area | Action | Expected result |
|---|---|---|
| Boot | Boot with and without USB | UI starts, services run and state is correct |
| USB | Insert a drive with media or an empty drive | Read-only mount and a correct playlist or empty state |
| USB | Remove during playback, then reconnect | Playback stops, removal is shown and stale entries disappear |
| Playback | Play/Pause, select tracks, Next/Previous | Video/audio and selection agree |
| Playback | Seek near the start/end | Position stays within the known duration |
| Playback | Enable/disable Auto Next | Enabled: play the next track. Disabled: retain the final frame and replay on Play |
| Playback | Open corrupt or unsupported media | Show an error; audio-only output is not a successful video test |
| CAN | Send all six actions from STM32 | Behavior matches UI controls |
| CAN | Send bad checksums, invalid ID/DLC and repeated commands | Record actual behavior; do not assume counter-based duplicate filtering |
| Robustness | Repeat track changes, USB insertion/removal and long playback | No crashes/restarts; monitor CPU/RSS/CMA and kernel logs |
| Status | Observe `0x101` | Pass only when the application sends status and STM32 receives correct values |

See [CAN_BRINGUP_AND_TESTING.md](CAN_BRINGUP_AND_TESTING.md) for CAN procedures.

## Build-host tests

Host tests cover logic, not physical drivers, decoding or display output. Install the module's documented dependencies first.

From the repository root:

```sh
cmake -S Yocto/meta-usb-video-player/recipes-platform/tests -B /tmp/platform-tests
cmake --build /tmp/platform-tests -j2
ctest --test-dir /tmp/platform-tests --output-on-failure --no-tests=error
```

UI/core tests are in [recipes-app/usb-video-player/tests](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/tests/). See the [application README](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/README.md) for instructions.

## Test records

Record the commit, hardware/image, media file, reproduction steps, expected/actual results and log paths. Mark each case Pass, Fail or Not tested. A successful build is not a functional test.

```sh
journalctl -u usb-video-player.service -b --no-pager -o cat > /tmp/player-test.log
dmesg > /tmp/kernel-test.log
```

Copy logs to the host before rebooting.

## Items requiring verification

Periodic telemetry, rolling-counter duplicate handling and CAN timeout behavior still require implementation or verification; do not assume they are present.
