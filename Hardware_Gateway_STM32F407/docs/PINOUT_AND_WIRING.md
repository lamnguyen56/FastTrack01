# Pinout and Wiring

MCU signal names below match [config.h](../include/config.h). Board header positions and power pins depend on the exact board revision; use its schematic rather than copying header numbers from another kit.

## STM32 CAN transceiver

| STM32 signal | Transceiver signal | Direction |
|---|---|---|
| PB9, CAN1_TX | TXD / TX input | STM32 to transceiver |
| PB8, CAN1_RX | RXD / RX output | Transceiver to STM32 |
| GND | GND | Common reference |
| Correct module supply | VCC | Use the actual transceiver/module specification |

A CAN transceiver is required between MCU pins and CANH/CANL. A SN65HVD230 module and a VP1050/TJA1050 module have different supply/logic requirements; they are not interchangeable power connections. Confirm the fitted module, its RXD level and the MCU input conditions before connection. The firmware does not identify the transceiver model.

The default is PB8/PB9. `USE_CAN1_PB8_PB9=0` selects PD0/PD1; the board/MSP configuration and wiring must match.

## Raspberry Pi CAN HAT

Use the Raspberry Pi RS485 CAN HAT's 40-pin connector and documented power arrangement. Do not assume a generic 5 V MCP2515 module has Pi-compatible SPI levels.

| CAN function | BCM GPIO | Pi physical pin |
|---|---|---|
| SPI0 CE0 | GPIO8 | 24 |
| MISO | GPIO9 | 21 |
| MOSI | GPIO10 | 19 |
| SCLK | GPIO11 | 23 |
| Interrupt | GPIO25 | 22 |
| Ground | GND | 6, or another ground pin |

The oscillator value in the Pi overlay must match the HAT crystal; see the [Pi configuration guide](../../Docs/CAN_BRINGUP_AND_TESTING.md). RS485 UART is independent of this CAN connection.

Connect CANH to CANH, CANL to CANL and a common ground between the two non-isolated bench nodes. Use one 120-ohm termination at each physical end of the bus. With power removed, two such terminations in parallel measure approximately 60 ohms across CANH/CANL.

See the [system CAN connection diagram](../../Docs/diagrams/can-hardware-connections.mmd).

## Buttons

| Pin | Input configuration | Action |
|---|---|---|
| PE4 | Active-low, internal pull-up | Play/Pause |
| PE5 | Active-low, internal pull-up | Single: Next; double: Previous |
| PE6 | Active-low, internal pull-up | Single: Seek +10 s; double: Seek -10 s |
| PA0 | Active-high, no internal pull resistor | Onboard Play/Pause alternative |

Connect each external button between its PE pin and GND. PA0 assumes the Discovery board's external bias/button circuit; another board must provide a defined idle level. Use the actual board schematic to check peripheral conflicts.

## LEDs

| Pin | Discovery LED | Actual indication |
|---|---|---|
| PD12 | Green, LD4 | Command/click activity pulse |
| PD13 | Orange, LD3 | At least one debounced button is pressed |
| PD14 | Red, LD5 | Initialization, mailbox or HAL CAN error indication |
| PD15 | Blue, LD6 | Most recently received playback state is Playing |

Green pulses are triggered both by successful mailbox submission and by multi-click recognition; they do not confirm bus acknowledgement or Pi execution. Red does not uniquely identify a wiring fault. Blue has no telemetry-age timeout and can retain an old state.

[Overview](../README.md) · [Software setup](../SOFTWARE_SETUP.md)
