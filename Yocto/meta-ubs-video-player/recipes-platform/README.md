# CAN and USB platform services

## CAN

`can-daemon.service` runs `setup_can.sh` before opening SocketCAN. The script waits for `can0`, fails on timeout or a failed `ip` command, and does not depend on lsmod/modprobe because the project driver is built-in. Defaults: 500 kbit/s, restart-ms 100, interface wait 30 seconds. The service has a 45-second startup timeout and retries after 5 seconds without a start-rate limit.

Environment overrides supported by the script: `CAN_IF`, `CAN_BITRATE`, `CAN_RESTART_MS`, `CAN_WAIT_SECONDS`. Keep the startup timeout longer than the chosen wait. CAN protocol/IPC remains unchanged.

## USB

The daemon subscribes to udev before enumerating existing block devices. Only partitions with a USB-device ancestor are candidates. It manages the first eligible FAT32 partition and ignores other partitions while active. Whole-disk filesystems without a partition table and exFAT/NTFS are not supported by this baseline.

Mounting uses Linux syscalls with read-only, nodev, nosuid and noexec flags. Mount failures do not trigger scans. It refuses to hide existing files at the mount point, stack unrelated mounts, adopt read-write mounts, or mount a device already used elsewhere (including a USB root filesystem).

The main thread receives hotplug events while a worker scans recursively. Removal is matched against the selected partition's sysfs path. Only that partition's removal clears the playlist and requests cancellation. Cancelled/error scan results are never published. Filesystem error-code APIs handle missing devices without uncaught filesystem exceptions; symlinks are skipped. Blocking kernel I/O can still delay a worker's exit on a faulty device.

Physical removal uses lazy detach of the owned mount; this is cleanup after an unexpected unplug, not safe eject. Normal service stop attempts a regular unmount and logs errors such as open files. Stop playback before a planned service restart/eject. No eject handshake with the player has been implemented.

IPC remains `/tmp/usb_playlist.txt`, preserving its inode for the existing watcher. Startup/shutdown/removal clear it. POSIX message-queue migration and atomic playlist snapshots are separate work.

## Module boundaries

Each daemon keeps `main.cpp` as an entry point only. Headers expose module APIs; implementations live in matching `.cpp` files. The recipe fetches `file://src/` and CMake lists each translation unit explicitly.

| USB module | Responsibility |
|---|---|
| `UsbApplication` | Startup, signal handling and event loop |
| `HotplugMonitor` | Own libudev resources, enumerate partitions, emit device value objects |
| `StorageController` | Coordinate one active USB, mount/scan/removal and publication |
| `MountManager` | Mount ownership checks and mount/unmount syscalls |
| `MediaScanner` | Media filtering, worker job, cancellation and scan results |
| `PlaylistWriter` | Publish/clear the existing playlist-file IPC |
| `UsbDevice` | Device/event values shared between modules, without udev pointers |

| CAN module | Responsibility |
|---|---|
| `CanApplication` | Startup, signal handling and CAN/IPC bridge loop |
| `SocketCan` | Own the CAN socket and send/receive frames |
| `IpcChannel` | Own the existing Unix datagram socket |
| `ProtocolCodec` | Decode command frames and encode big-endian status |
| `setup_can.sh` | Configure interface before the daemon starts |

The refactor does not switch IPC transports or introduce a new checksum/counter policy. Missing peers still use nonblocking operations. Sockets and udev objects are released by their owning modules.

## Host tests

The combined runner builds both real daemon executables with `-Wall -Wextra -Werror` and registers three CTest cases. It does not start the daemons, configure real interfaces or mount devices. Test assertions remain enabled in Release builds.

```bash
cmake -S tests -B /tmp/ubs-platform-tests -DCMAKE_BUILD_TYPE=Debug
cmake --build /tmp/ubs-platform-tests -j2
ctest --test-dir /tmp/ubs-platform-tests --output-on-failure
```

The commands above run from `recipes-platform/`. Alternatively, run individual tests below. On Linux with a C++17 compiler, Python 3 and libudev development files, from this directory:

```bash
python3 can-daemon/tests/setup_test.py
g++ -std=c++17 -Wall -Wextra -Werror -pthread \
    usb-monitor/tests/scan_test.cpp usb-monitor/files/src/MediaScanner.cpp \
    -o /tmp/usb-scan-test
/tmp/usb-scan-test
g++ -std=c++17 -Wall -Wextra -Werror -Icommon/files \
    can-daemon/tests/codec_test.cpp can-daemon/files/src/ProtocolCodec.cpp \
    -o /tmp/can-codec-test
/tmp/can-codec-test
```

Tests cover immediate/late/absent CAN interfaces, failed CAN configuration, media filtering/sort, symlink skipping, cancellation missing/empty scan directories, CAN field mapping and status endianness. Hardware hotplug, physical CAN, service startup and actual mounts still need target validation.
