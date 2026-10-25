# Software Setup and Verification

Use the source in this folder with an STM32F407VG project and matching STM32F4 HAL/CMSIS files. Wiring is documented separately in [Pinout and wiring](docs/PINOUT_AND_WIRING.md).

## Clock and CAN configuration

The supplied `SystemClock_Config()` expects an 8 MHz HSE crystal. Match the real board clock source before running it.

| Setting | Source value |
|---|---|
| PLL M / N / P / Q | 8 / 336 / 2 / 7 |
| SYSCLK / HCLK | 168 MHz / 168 MHz |
| APB1 / APB2 | 42 MHz / 84 MHz |
| CAN1 mode | Normal |
| CAN1 RX / TX | PB8 / PB9, AF9 |
| Prescaler / BS1 / BS2 / SJW | 6 / 11 TQ / 2 TQ / 1 TQ |
| Bitrate | `42000000 / (6 * (1 + 11 + 2)) = 500000` bit/s |
| Automatic retransmission / bus-off recovery | Enabled |
| RX FIFO0 IRQ / SCE IRQ priority | 2 / 3 |

`CAN_Driver_Init()` configures an accept-all filter into FIFO0. The receive callback processes status ID `0x101` with DLC 8. Do not describe this as an ID-specific hardware filter.

## Integrate with a board project

1. Create an STM32F407VG board project in CubeIDE or your existing toolchain. Include the correct startup, linker script, HAL drivers and CMSIS support.
2. Add this folder's `include/` to include paths and compile the source from `src/`. Use the matching `stm32f4xx_hal_conf.h` configuration.
3. Keep exactly one definition of `main`, `SystemClock_Config`, `hcan1`, IRQ handlers and HAL CAN callbacks. The supplied code already owns these; do not also compile generated copies of `main.c`, `can.c` or the same interrupt handlers.
4. Check MSP ownership. The supplied `HAL_CAN_MspInit()` is weak; a generated strong implementation overrides it and must enable CAN1/GPIO clocks, AF9 pins and both CAN interrupts with the settings above.
5. Build and flash through ST-Link. Resolve toolchain/HAL-version integration errors before testing the bus.

If merging into an existing generated application instead, preserve the initialization and callback order in [main.c](src/main.c), and retain one owner for each peripheral and interrupt. Do not paste a second, independent CAN implementation into the project.

## Execution flow

`main()` initializes HAL, clocks, LEDs, buttons and CAN, then registers command/status callbacks. The superloop scans buttons and updates LED effects, checks CAN errors every 250 ms and calls `HAL_Delay(2)`.

Button input is polled; CAN reception uses interrupts. `LED_Init()` performs two 80 ms on/off cycles, totaling about 320 ms before button/CAN initialization. No startup or end-to-end latency has been measured.

## Button timing

| Parameter | Value and effect |
|---|---|
| Debounce | 50 ms stable input |
| Double-click window | 350 ms after the first debounced release |
| Long hold | At least 500 ms: emit single-click action on release |
| Cooldown | 200 ms; multi-click lockout waits for a stable release |

PE4 and PA0 have separate debounce/cooldown tracking. PE5 and PE6 each have their own five-state FSM. Extra presses during lockout are ignored; a new sequence can begin after lockout ends.

## Verify with the Pi

Use the configured Yocto image and [Pi CAN guide](../Docs/CAN_BRINGUP_AND_TESTING.md). On the Pi:

```sh
systemctl status can-daemon.service --no-pager -l
ip -details -statistics link show can0
candump -tz can0,100:7FF
```

After a fresh STM32 reset, an accepted Play request normally produces `01 00 00 01 02 00 00 00`: the packer increments the initial counter from zero to one. Check all six actions, checksum and counter wrap `15 -> 0`.

To test status reception independently of automatic telemetry:

```sh
cansend can0 101#01030064012C02FF
```

This represents Playing, track 3, current time 100 s, duration 300 s and USB ready. The blue LED should turn on. Send Stopped to turn it off:

```sh
cansend can0 101#0000000000000000
```

These commands inject test status; they do not prove the application sends telemetry. For end-to-end testing, connect a display and USB media, then verify the UI response to each button.

## Troubleshooting

| Symptom | Check |
|---|---|
| No commands | Correct firmware flashed, actual PE4–PE6 macros, input levels and callback registration |
| CAN errors or no Pi reception | Both bitrates, clock/oscillator, transceiver power, wiring and termination |
| Green pulses but no UI response | Green is activity feedback, not proof of CAN acknowledgement or UI handling; inspect `candump` and application logs |
| Blue LED stays off | Inject a known `0x101` frame and inspect the CAN RX callback/IRQ path |
| Play/Pause differs from UI state | Telemetry may be absent or stale; local toggling is not authoritative playback feedback |
| CAN initialization fails after integration | Check generated MSP overrides, duplicate peripheral initialization and HAL configuration |
