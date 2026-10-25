# STM32F407 CAN Button Controller

STM32F407 firmware sends physical button commands to the Raspberry Pi USB video player over CAN1 at 500 kbit/s. Status frames received from the Pi update the playback LED and button state.

## Documents

| Document | Contents |
|---|---|
| [Software setup](SOFTWARE_SETUP.md) | Clock, HAL integration, build and verification |
| [Pinout and wiring](docs/PINOUT_AND_WIRING.md) | MCU pins, transceiver, buttons, LEDs and CAN bus |
| [Software function blocks](docs/diagrams/software-function-blocks.mmd) | Modules, callbacks and interrupts |
| [Multi-click button state machine](docs/diagrams/button-state-machine.mmd) | Next/Previous and Seek recognition |
| [Pi CAN bring-up and testing](../Docs/CAN_BRINGUP_AND_TESTING.md) | Pi configuration and end-to-end checks |
| [Interface specification](../Docs/INTERFACE_SPECIFICATION.md) | Shared CAN/IPC packet formats |

## Controls

| Input | Single press/click | Double click |
|---|---|---|
| PE4 or onboard PA0 | Play/Pause request | No double-click action |
| PE5 | Next | Previous |
| PE6 | Seek forward 10 s | Seek backward 10 s |

External buttons are active-low; onboard PA0 is active-high. Play/Pause is triggered on the debounced press. PE5/PE6 wait for the second click before confirming a short single click. See the state machine for timing.

Play/Pause uses received playback state and a local toggle flag. Without current telemetry, the local state can differ from playback started through the Pi UI or Auto Next. No telemetry timeout is implemented. The Pi application currently has no periodic `sendStatus()` caller.

## Source ownership

| Module | Responsibility |
|---|---|
| [config.h](include/config.h) | Actual pin macros, CAN timing and button intervals |
| [can_protocol.h](include/can_protocol.h) | Command packing, checksum/counter and status unpacking |
| [button_manager.c](src/button_manager.c) | GPIO polling, debounce and click state machines |
| [can_driver.c](src/can_driver.c) | CAN1 initialization, mailbox submission and status RX callback |
| [led_indicator.c](src/led_indicator.c) | LED GPIO and pulse effects |
| [main.c](src/main.c) | Clock setup, callbacks and superloop |
| [stm32f4xx_it.c](src/stm32f4xx_it.c) | SysTick and CAN IRQ handlers |

This folder contains application source, not a complete CubeIDE project: startup assembly, linker script and STM32 HAL/CMSIS dependencies must come from the matching board project. Some source comments still mention PE0–PE2; the compiled macros select **PE4–PE6**.
