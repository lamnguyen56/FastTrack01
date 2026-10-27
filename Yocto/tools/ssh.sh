#!/usr/bin/env bash
# Resolve the Pi from the Ubuntu neighbor table, or use an explicit IPv4 address.
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

# This branch intentionally runs before neighbor-table discovery.
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
command -v ip >/dev/null || { echo "ERROR: The ip command is required (install the iproute2 package)." >&2; exit 1; }

# Match the lladdr column exactly; do not treat a substring match as a device match.
if ! OUTPUT="$(ip -4 neigh show)"; then
    echo "ERROR: Could not read the IPv4 neighbor table. Use: $0 192.168.1.30" >&2
    exit 1
fi
mapfile -t IPS < <(printf '%s\n' "$OUTPUT" | tr -d '\r' |
    awk -v mac="${MAC//-/:}" 'tolower($5) == mac && $1 ~ /^[0-9]+\.[0-9]+\.[0-9]+\.[0-9]+$/ { print $1 }' | sort -u)
if [[ ${#IPS[@]} -eq 0 ]]; then
    echo "ERROR: Pi MAC $MAC is not in the Ubuntu IPv4 neighbor table." >&2
    echo "Connect both devices to the same LAN and make sure the Pi has communicated with this computer." >&2
    echo "This lookup reads cached neighbors; it does not scan the subnet. Use an explicit IP if known." >&2
    exit 1
fi
if [[ ${#IPS[@]} -ne 1 ]]; then
    echo "ERROR: Multiple IPs match $MAC: ${IPS[*]}. Refresh the neighbor table or pass the deploy IP explicitly." >&2
    exit 1
fi
IP="${IPS[0]}"
if [[ "$PRINT_IP" == 1 ]]; then
    printf '%s\n' "$IP"
else
    echo "Found Raspberry Pi at $IP" >&2
    exec ssh "root@$IP"
fi
