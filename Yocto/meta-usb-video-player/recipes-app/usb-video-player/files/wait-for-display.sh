#!/bin/sh
# Wait for a connected DRM output with advertised modes and a primary device.
set -eu

# Alternate roots allow host tests without touching real display hardware.
sysfs_root="${DRM_SYSFS_ROOT:-/sys/class/drm}"
device_root="${DRM_DEVICE_ROOT:-/dev/dri}"
timeout="${DISPLAY_WAIT_SECONDS:-60}"
case "$timeout" in
    ''|*[!0-9]*) echo "Invalid DISPLAY_WAIT_SECONDS: $timeout" >&2; exit 2 ;;
esac
elapsed=0
echo "Waiting for a connected DRM display (timeout ${timeout}s)..."
while :; do
    for connector in "$sysfs_root"/card*-*; do
        [ -r "$connector/status" ] && [ -r "$connector/modes" ] || continue
        status=$(cat "$connector/status")
        [ "$status" = connected ] || continue
        # sysfs reports synthetic file sizes; inspect content rather than test -s.
        modes=$(cat "$connector/modes")
        [ -n "$modes" ] || continue
        name=${connector##*/}
        card=${name%%-*}
        [ -c "$device_root/$card" ] || continue
        echo "DRM display ready: $name ($device_root/$card)"
        exit 0
    done
    if [ "$elapsed" -ge "$timeout" ]; then
        echo "No usable DRM display detected; check display power, cable and EDID." >&2
        exit 1
    fi
    sleep 1
    elapsed=$((elapsed + 1))
done
