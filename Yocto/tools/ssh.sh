#!/usr/bin/env bash
# Resolve the Pi by MAC, or bypass Windows interop with an explicit IPv4 address.
set -euo pipefail

usage() {
    echo "Usage: $0 [--ip] [IPv4]"
    echo "Without IPv4, resolve PI_MAC. PI_IP also bypasses discovery."
}

PRINT_IP=0
if [[ "${1:-}" == --ip ]]; then
    PRINT_IP=1
    shift
fi
if [[ "${1:-}" == --help ]]; then
    usage
    exit 0
fi
if [[ $# -gt 1 ]]; then
    usage >&2
    exit 2
fi

# This branch intentionally runs before any Windows command or MAC discovery.
IP="${1:-${PI_IP:-}}"
if [[ -n "$IP" ]]; then
    if [[ ! "$IP" =~ ^([0-9]{1,3}\.){3}[0-9]{1,3}$ ]]; then
        echo "ERROR: Invalid IPv4: $IP" >&2
        exit 2
    fi
    IFS=. read -r -a OCTETS <<< "$IP"
    for OCTET in "${OCTETS[@]}"; do
        if (( 10#$OCTET > 255 )); then
            echo "ERROR: Invalid IPv4: $IP" >&2
            exit 2
        fi
    done
    if [[ "$PRINT_IP" == 1 ]]; then
        printf '%s\n' "$IP"
    else
        exec ssh "root@$IP"
    fi
    exit 0
fi

MAC="${PI_MAC:-b8-27-eb-db-99-c4}"
MAC="${MAC//:/-}"
MAC="${MAC,,}"
if [[ ! "$MAC" =~ ^([0-9a-f]{2}-){5}[0-9a-f]{2}$ ]]; then
    echo "ERROR: Invalid PI_MAC: $MAC" >&2
    exit 2
fi
command -v powershell.exe >/dev/null || { echo "ERROR: Windows powershell.exe is required; run from WSL." >&2; exit 1; }

# Match the MAC column exactly; do not treat a substring match as a device match.
if ! OUTPUT="$(powershell.exe -NoProfile -NonInteractive -Command 'arp.exe -a')"; then
    echo "ERROR: Windows interop failed. Use: $0 192.168.1.30" >&2
    exit 1
fi
mapfile -t IPS < <(printf '%s\n' "$OUTPUT" | tr -d '\r' |
    awk -v mac="$MAC" 'tolower($2) == mac && $1 ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/ { print $1 }' | sort -u)
if [[ ${#IPS[@]} -eq 0 ]]; then
    echo "ERROR: Pi MAC $MAC is not in the Windows ARP cache." >&2
    echo "Check that the Pi is online on the same LAN. This lookup reads cached neighbours; it does not scan the subnet." >&2
    exit 1
fi
if [[ ${#IPS[@]} -ne 1 ]]; then
    echo "ERROR: Multiple IPs match $MAC: ${IPS[*]}. Refresh the ARP cache or pass the deploy IP explicitly." >&2
    exit 1
fi
IP="${IPS[0]}"
if [[ "$PRINT_IP" == 1 ]]; then
    printf '%s\n' "$IP"
else
    echo "Found Raspberry Pi at $IP" >&2
    exec ssh "root@$IP"
fi
