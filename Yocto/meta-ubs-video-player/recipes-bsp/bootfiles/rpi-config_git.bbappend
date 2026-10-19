# Stock BSP overlay: SPI0 CE0, GPIO25. RS485 is independent.
ENABLE_SPI_BUS:raspberrypi3-64 = "1"
CAN0_INTERRUPT_PIN:raspberrypi3-64 = "25"

# Unknown until the actual HAT crystal is checked. Set to its frequency in Hz.
# Example only for a confirmed 8 MHz crystal: "8000000".
CAN_OSCILLATOR:raspberrypi3-64 = ""
ENABLE_CAN:raspberrypi3-64 = "${@'1' if d.getVar('CAN_OSCILLATOR').strip() else '0'}"
ENABLE_DUAL_CAN:raspberrypi3-64 = "0"

python __anonymous() {
    if d.getVar('MACHINE') == 'raspberrypi3-64':
        clock = d.getVar('CAN_OSCILLATOR').strip()
        if clock and (not clock.isascii() or not clock.isdigit() or int(clock) <= 0):
            bb.fatal('CAN_OSCILLATOR must be a positive integer frequency in Hz')
}

do_deploy:prepend:raspberrypi3-64() {
    if [ -z "${CAN_OSCILLATOR}" ]; then
        bbwarn "CAN overlay disabled: confirm the HAT crystal and set CAN_OSCILLATOR in rpi-config_git.bbappend"
    fi
}
