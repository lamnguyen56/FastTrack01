#!/usr/bin/env bash
# Build and deploy the Yocto ARM64 usb-monitor executable over SSH.
set -euo pipefail

usage() {
    echo "Usage: $0 [--logs] [[user@]host]"
    echo "Run from an initialized Yocto build shell. --logs follows the journal after deployment."
}

FOLLOW_LOGS=0
if [[ "${1:-}" == "--logs" ]]; then
    FOLLOW_LOGS=1
    shift
fi
if [[ "${1:-}" == "--help" || "${1:-}" == "-h" ]]; then
    usage
    exit 0
fi
if [[ $# -gt 1 || ( $# -eq 1 && ! "$1" =~ ^([a-zA-Z0-9_][a-zA-Z0-9_-]*@)?[a-zA-Z0-9][a-zA-Z0-9.-]*$ ) ]]; then
    usage >&2
    exit 2
fi
TARGET="${1:-}"
if [[ -n "$TARGET" && "$TARGET" != *@* ]]; then
    TARGET="root@$TARGET"
fi
for tool in bitbake ssh scp readelf; do
    command -v "$tool" >/dev/null || { echo "ERROR: Missing $tool; initialize the Yocto build shell first." >&2; exit 1; }
done

ENV_FILE="$(mktemp)"
REMOTE_FILE=""
cleanup() {
    rm -f -- "$ENV_FILE"
    if [[ -n "$REMOTE_FILE" ]]; then
        ssh -o ConnectTimeout=10 "$TARGET" "rm -f -- '$REMOTE_FILE'" </dev/null || true
    fi
}
trap cleanup EXIT
trap 'exit 130' INT
trap 'exit 143' TERM

# Resolve the recipe output instead of hard-coding TMPDIR, tune or recipe version.
echo "Building usb-monitor for the target..."
bitbake usb-monitor
bitbake -e usb-monitor > "$ENV_FILE"
IMAGE_DIR="$(sed -n 's/^D="\(.*\)"$/\1/p' "$ENV_FILE")"
TARGET_BINDIR="$(sed -n 's/^bindir="\(.*\)"$/\1/p' "$ENV_FILE")"
BINARY="${IMAGE_DIR}${TARGET_BINDIR}/usb-monitor"
if [[ -z "$IMAGE_DIR" || "$TARGET_BINDIR" != /usr/bin || ! -f "$BINARY" ]]; then
    echo "ERROR: Expected recipe executable not found: $BINARY" >&2
    exit 1
fi
if ! LC_ALL=C readelf -h "$BINARY" | grep -q 'Machine:.*AArch64'; then
    echo "ERROR: Refusing to deploy an executable that is not AArch64." >&2
    exit 1
fi

# Reuse ssh.sh's MAC lookup when no explicit destination was provided.
if [[ -z "$TARGET" ]]; then
    SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
    TARGET="root@$(bash "$SCRIPT_DIR/ssh.sh" --ip)"
    echo "Found Raspberry Pi: $TARGET"
fi

# Check access and allocate an isolated upload path before stopping any service.
REMOTE_FILE="$(ssh "$TARGET" 'test "$(id -u)" = 0 && command -v systemctl >/dev/null && mktemp /tmp/usb-monitor.deploy.XXXXXXXX')"
if [[ ! "$REMOTE_FILE" =~ ^/tmp/usb-monitor\.deploy\.[a-zA-Z0-9]+$ ]]; then
    REMOTE_FILE=""
    echo "ERROR: Cannot allocate upload file; connect as root." >&2
    exit 1
fi
echo "Uploading $BINARY to $TARGET..."
scp "$BINARY" "${TARGET}:${REMOTE_FILE}"

# The quoted heredoc runs on the Pi, without expanding host variables.
ssh "$TARGET" "sh -s -- '$REMOTE_FILE'" <<'REMOTE'
set -eu
upload="$1"
binary=/usr/bin/usb-monitor
backup=/usr/bin/usb-monitor.previous
staged=/usr/bin/usb-monitor.new
lock=/run/usb-monitor-deploy.lock
usb_active=0
player_active=0
replaced=0
committed=0

# Serialize updates so concurrent deployments cannot overwrite each other's backup.
if ! mkdir "$lock"; then
    echo "ERROR: Another deployment is running ($lock)." >&2
    exit 1
fi
finish() {
    result=$?
    trap - EXIT HUP INT TERM
    if [ "$committed" = 0 ]; then
        echo "Deployment failed; restoring the previous state." >&2
        if [ "$replaced" = 1 ]; then
            systemctl stop usb-monitor.service || true
            if cp -p "$backup" "$staged" && mv -f "$staged" "$binary"; then
                echo "Previous binary restored." >&2
            else
                echo "ERROR: Rollback failed; recover $backup using the serial console." >&2
            fi
        fi
        if [ "$usb_active" = 1 ]; then
            systemctl start usb-monitor.service || true
        fi
    fi
    if [ "$player_active" = 1 ]; then
        systemctl start ubs-video-player.service || result=1
    fi
    rm -f "$upload" "$staged" "$backup.new"
    rmdir "$lock"
    exit "$result"
}
trap finish EXIT
trap 'exit 129' HUP
trap 'exit 130' INT
trap 'exit 143' TERM

if systemctl is-active --quiet usb-monitor.service; then usb_active=1; fi
if systemctl is-active --quiet ubs-video-player.service; then player_active=1; fi

# Prepare the replacement and backup on the destination filesystem before downtime.
test -f "$binary"
chmod 0755 "$upload"
cp "$upload" "$staged"
cp -p "$binary" "$backup.new"
mv -f "$backup.new" "$backup"
if [ "$player_active" = 1 ]; then
    systemctl stop ubs-video-player.service
fi
systemctl stop usb-monitor.service
mv -f "$staged" "$binary"
replaced=1
systemctl start usb-monitor.service
sleep 2
systemctl is-active --quiet usb-monitor.service
committed=1
echo "usb-monitor updated; backup: $backup"
REMOTE
REMOTE_FILE=""
echo "Deployment completed."
if [[ "$FOLLOW_LOGS" == 1 ]]; then
    ssh "$TARGET" 'journalctl -u usb-monitor.service -f -o cat'
else
    echo "Logs: ssh $TARGET 'journalctl -u usb-monitor.service -f -o cat'"
fi
