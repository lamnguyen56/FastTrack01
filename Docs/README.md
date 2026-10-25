# FastTrack01 — Project Documentation

A Raspberry Pi 3B USB video player with a Qt5/QML interface and CAN commands from STM32F4 buttons.

## Reading guide

| Document | Contents |
|---|---|
| [Architecture](SYSTEM_ARCHITECTURE.md) | Layers, processes, diagrams and system behavior |
| [Interfaces](INTERFACE_SPECIFICATION.md) | CAN frames, Unix sockets and the USB playlist |
| [Build and deployment](BUILD_AND_DEPLOYMENT.md) | Build an image, update a process and view logs |
| [CAN testing on the Pi](CAN_BRINGUP_AND_TESTING.md) | Configuration and STM32-to-UI checks |
| [Testing](SYSTEM_TESTING.md) | Functional checklist and test records |
| [Original OJT requirements](OJT_USB_Video_Player_CAN_Qt5_QML.pptx) | Project brief and deliverables; preserved in the original language |

## Project scope

- Target: Raspberry Pi 3B, Yocto Scarthgap, machine `raspberrypi3-64`.
- UI: Qt5/QML, Qt Multimedia playback and EGLFS/KMS display output.
- USB: one FAT32 partition, mounted read-only to create a playlist.
- CAN: STM32F4 buttons control the player through the Pi CAN HAT.

See [SYSTEM_TESTING.md](SYSTEM_TESTING.md) for test procedures and records.

## Diagrams

[SYSTEM_ARCHITECTURE.md](SYSTEM_ARCHITECTURE.md) indexes the component architecture, function blocks, state machines, data flow, system flowchart and interaction sequences.

Mermaid sources are stored in [diagrams/](diagrams/). Open a `.mmd` file and run **Mermaid: Preview Diagram** in VS Code. Update these source files when the architecture changes.
