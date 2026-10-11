#!/bin/sh
# ==============================================================================
# Script: setup_can.sh
# Purpose: Initialize SocketCAN interface (can0) with 500 kbps & auto-restart
# Target: Raspberry Pi / Linux Embedded Target with MCP2515
# ==============================================================================

set -e

CAN_IF="can0"
CAN_BITRATE="500000"
RESTART_MS="100"

echo "[CAN_INIT] Starting SocketCAN bring-up for $CAN_IF..."

# Check and load kernel module
if ! lsmod | grep -q "mcp251x"; then
    echo "[CAN_INIT] Loading mcp251x kernel module..."
    modprobe mcp251x || true
fi

# Bring down interface if already up
if ip link show "$CAN_IF" 2>/dev/null | grep -q "UP"; then
    echo "[CAN_INIT] Interface $CAN_IF is currently UP. Bringing it DOWN to reconfigure..."
    ip link set "$CAN_IF" down
fi

# Configure bitrate, restart-ms and bring UP
echo "[CAN_INIT] Setting bitrate=$CAN_BITRATE bps, restart-ms=$RESTART_MS ms on $CAN_IF..."
ip link set "$CAN_IF" type can bitrate "$CAN_BITRATE" restart-ms "$RESTART_MS"
ip link set "$CAN_IF" txqueuelen 1000
ip link set "$CAN_IF" up

echo "[CAN_INIT] SUCCESS: $CAN_IF is UP and operational!"
ip -details link show "$CAN_IF"
