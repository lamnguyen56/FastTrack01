# System Architecture

## Diagram index

| Diagram | Purpose |
|---|---|
| [Component architecture](diagrams/component-architecture.mmd) | Layers, processes and component connections |
| [Software function blocks](diagrams/software-function-blocks.mmd) | Module responsibilities and relationships |
| [USB state machine](diagrams/usb-state-machine.mmd) | Mount, scan, publication and cleanup |
| [Playback state machine](diagrams/playback-state-machine.mmd) | Load, Play/Pause, EOS, Auto Next and errors |
| [Data flow](diagrams/data-flow.mmd) | Data sources, processing, storage and outputs |
| [System flowchart](diagrams/system-flowchart.mmd) | Boot, services and event loops |
| [Playback sequence](diagrams/playback-sequence.mmd) | Reset, readiness, Play and EOS ordering |
| [USB sequence](diagrams/usb-sequence.mmd) | USB service, playlist and application interaction |
| [CAN command sequence](diagrams/can-command-sequence.mmd) | CAN frame to IPC, controller and UI |
| [CAN hardware connections](diagrams/can-hardware-connections.mmd) | STM32F4, CAN HAT and Pi connections |

The state machines describe logical behavior derived from the source; they are not shared state enums. USB and playback states are independent. Losing CAN connectivity does not imply a USB or media failure.

Sequence diagrams show interaction and callback ordering, not measured timing. The source defines these scheduling conditions:

| Event | Configuration or condition |
|---|---|
| Playlist file event | 100 ms debounce, restarted by each new event |
| Mount/playlist fallback check | 1000 ms poll timer |
| Session creation and Play request | `Qt.callLater`, after the current callback, without a fixed delay |
| Actual playback | Media is ready, source/generation checks pass and the backend reports Playing |

Scan, load and decode times depend on USB storage, media files and system load. Timer intervals are not end-to-end latency guarantees.

## Layers

| Layer | Components | Responsibility |
|---|---|---|
| Hardware | Pi 3B, USB, CAN HAT, display and external STM32F4 | Store media, display output and generate commands |
| Kernel/BSP | USB storage, VFAT, MCP251x, SocketCAN, VC4/KMS | Expose block devices, `can0` and display interfaces |
| Platform | `usb-monitor`, `can-daemon` | Mount/scan USB and bridge CAN commands to IPC |
| Application | `usb-video-player` | Playlist, playback control and QML UI |
| Build | Yocto, `meta-usb-video-player`, tools | Package processes and build the image |

## Processes on the Pi

| Process | Main modules | Responsibility |
|---|---|---|
| `usb-monitor` | HotplugMonitor, StorageController, MountManager, MediaScanner, PlaylistWriter | Detect USB events, mount, scan and publish the playlist |
| `can-daemon` | SocketCan, ProtocolCodec, IpcChannel | Forward commands from `0x100`; send status frames when IPC data is available |
| `usb-video-player` | UsbPlaylistReader, UsbPlaylistModel, CanIpcClient, PlayerController, QML | Read the playlist, receive commands, play media and update the UI |

`PlayerController` handles commands from both UI buttons and CAN. The application does not mount USB storage or read frames directly from `can0`.

## Main flows

### USB

[USB sequence](diagrams/usb-sequence.mmd)

`usb-monitor` mounts a FAT32 partition at `/media/usb0`, scans media and publishes a playlist. The application reads this file and checks the mount independently. Removal clears the playlist and stops playback with a nonmodal UI notification. The notice disappears after 6 seconds or when dismissed; cleanup does not wait for user confirmation.

A new playlist selects the first item without automatically requesting playback. Explicit track selection and Next/Previous request playback; the initial selection alone does not.

### CAN

[CAN command sequence](diagrams/can-command-sequence.mmd) · [Hardware connections](diagrams/can-hardware-connections.mmd)

STM32 sends button commands. MCP251x exposes `can0`; `can-daemon` decodes frames and sends Unix datagrams to the application. The application validates the checksum and calls `PlayerController`.

The reverse status path has packet definitions, a send function and a codec. The current application does not call `sendStatus()` periodically, so automatic telemetry is not complete.

### Playback

Qt Multimedia provides QML `MediaPlayer` and `VideoOutput`. On the Qt5 target, GStreamer is the backend beneath Qt Multimedia. `HardwareDecodePolicy` allows only `v4l2h264dec` for automatic video decoding and disables software video fallback; audio decoder ranks are unchanged. Media errors stop the current session, including audio, and show an error. There is no fixed resolution precheck: acceptance depends on decoder caps and backend errors.

`PlayerController` requests playback changes. Backend callbacks report actual state, position, duration and errors to the controller and UI. Auto Next defaults to enabled for each application session; the setting is not persisted.

## Startup and source layout

systemd manages the three services. `setup_can.sh` prepares CAN; `wait-for-display.sh` checks display readiness before the application starts. USB and CAN services do not wait for Wi-Fi.

Source is maintained in [meta-usb-video-player](../Yocto/meta-usb-video-player/):

- `recipes-app/`: application C++ and QML.
- `recipes-platform/`: USB/CAN services and tests.
- `recipes-common/common/`: shared `Constants.h` and logger.
- `recipes-kernel/`, `recipes-bsp/`, `recipes-qt/`: platform configuration.
- `recipes-core/images/usb-image.bb`: project image.

[Interfaces](INTERFACE_SPECIFICATION.md) · [Build and deployment](BUILD_AND_DEPLOYMENT.md)

## Source references

| Behavior | Source |
|---|---|
| USB selection, scan and cleanup | [StorageController.cpp](../Yocto/meta-usb-video-player/recipes-platform/usb-monitor/files/src/StorageController.cpp) |
| Playlist validation and watch/poll timers | [UsbPlaylistReader.cpp](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/files/src/Usb/UsbPlaylistReader.cpp) |
| CAN checksum and command dispatch | [CanIpcClient.cpp](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/files/src/CanClient/CanIpcClient.cpp) |
| Selection, Auto Next and replay | [PlayerController.cpp](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/files/src/Player/PlayerController.cpp) |
| Qt5 session lifetime and error handling | [VideoSurfaceQt5.qml](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/files/qml/VideoSurfaceQt5.qml) |
| Video decoder policy | [HardwareDecodePolicy.cpp](../Yocto/meta-usb-video-player/recipes-app/usb-video-player/files/src/Player/HardwareDecodePolicy.cpp) |
