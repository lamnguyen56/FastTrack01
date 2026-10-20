#!/usr/bin/env bash
# Resolve the Pi's IPv4 address from the Windows ARP cache when running in WSL.
set -euo pipefail

MAC="${PI_MAC:-b8-27-eb-db-99-c4}"
MAC="${MAC//:/-}"
MAC="${MAC,,}"
if [[ ! "$MAC" =~ ^([0-9a-f]{2}-){5}[0-9a-f]{2}$ ]]; then
    echo "ERROR: Invalid PI_MAC: $MAC" >&2
    exit 2
fi
if [[ $# -gt 1 || ( $# -eq 1 && "$1" != --ip && "$1" != --help ) ]]; then
    echo "Usage: $0 [--ip | --help]" >&2
    exit 2
fi
if [[ "${1:-}" == --help ]]; then
    echo "Usage: $0 [--ip]"
    echo "Default: find the Pi by PI_MAC and open SSH. --ip: print only its IP."
    exit 0
fi
command -v powershell.exe >/dev/null || { echo "ERROR: Windows powershell.exe is required; run from WSL." >&2; exit 1; }

# Match the MAC column exactly; do not treat a substring match as a device match.
OUTPUT="$(powershell.exe -NoProfile -NonInteractive -Command 'arp.exe -a')"
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
if [[ "${1:-}" == --ip ]]; then
    printf '%s\n' "$IP"
else
    echo "Found Raspberry Pi at $IP" >&2
    exec ssh "root@$IP"
fi
