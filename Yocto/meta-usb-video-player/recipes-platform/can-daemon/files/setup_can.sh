#!/bin/sh
# Configure the interface only after the kernel has registered it.
set -eu

CAN_IF="${CAN_IF:-can0}"
CAN_BITRATE="${CAN_BITRATE:-500000}"
RESTART_MS="${CAN_RESTART_MS:-100}"
WAIT_SECONDS="${CAN_WAIT_SECONDS:-30}"

for value in "$CAN_BITRATE" "$RESTART_MS" "$WAIT_SECONDS"; do
    case "$value" in
        ''|*[!0-9]*) echo "[CAN_INIT] Invalid numeric setting: $value" >&2; exit 1 ;;
    esac
done

# The project builds mcp251x into the kernel. Do not rely on lsmod/modprobe.
elapsed=0
until ip link show dev "$CAN_IF" >/dev/null 2>&1; do
    if [ "$elapsed" -ge "$WAIT_SECONDS" ]; then
        echo "[CAN_INIT] $CAN_IF not found after ${WAIT_SECONDS}s; check overlay, oscillator, wiring and dmesg" >&2
        exit 1
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done

ip link set dev "$CAN_IF" down
ip link set dev "$CAN_IF" type can bitrate "$CAN_BITRATE" restart-ms "$RESTART_MS"
ip link set dev "$CAN_IF" txqueuelen 1000
ip link set dev "$CAN_IF" up
ip -details -statistics link show dev "$CAN_IF"
echo "[CAN_INIT] $CAN_IF configured and UP; physical TX/RX still requires a bus test."
