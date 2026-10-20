#!/usr/bin/env bash
# Build and deploy one Yocto ARM64 process over SSH.
set -euo pipefail

usage() {
    echo "Usage: $0 [--logs] <usb|can|app> [[user@]host]"
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
case "${1:-}" in
    usb|usb-monitor) PROCESS=usb-monitor ;;
    can|can-daemon) PROCESS=can-daemon ;;
    app|ubs-video-player) PROCESS=ubs-video-player ;;
    *) usage >&2; exit 2 ;;
esac
shift
SERVICE="${PROCESS}.service"
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
echo "Building $PROCESS for the target..."
bitbake "$PROCESS"
bitbake -e "$PROCESS" > "$ENV_FILE"
IMAGE_DIR="$(sed -nE 's/^(export )?D="(.*)"$/\2/p' "$ENV_FILE")"
TARGET_BINDIR="$(sed -nE 's/^(export )?bindir="(.*)"$/\2/p' "$ENV_FILE")"
# BitBake prefixes exported variables with "export "; accept either form above.
if [[ -z "$IMAGE_DIR" || -z "$TARGET_BINDIR" ]]; then
    echo "ERROR: Cannot read D/bindir from bitbake -e for $PROCESS." >&2
    exit 1
fi
BINARY="${IMAGE_DIR}${TARGET_BINDIR}/${PROCESS}"
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
REMOTE_FILE="$(ssh "$TARGET" 'test "$(id -u)" = 0 && command -v systemctl >/dev/null && mktemp /tmp/process.deploy.XXXXXXXX')"
if [[ ! "$REMOTE_FILE" =~ ^/tmp/process\.deploy\.[a-zA-Z0-9]+$ ]]; then
    REMOTE_FILE=""
    echo "ERROR: Cannot allocate upload file; connect as root." >&2
    exit 1
fi
echo "Uploading $BINARY to $TARGET..."
scp "$BINARY" "${TARGET}:${REMOTE_FILE}"

# The quoted heredoc runs on the Pi, without expanding host variables.
ssh "$TARGET" "sh -s -- '$REMOTE_FILE' '$PROCESS'" <<'REMOTE'
set -eu
upload="$1"
process="$2"
# Validate remotely as well before using the process in paths or unit names.
case "$process" in usb-monitor|can-daemon|ubs-video-player) ;; *) exit 2 ;; esac
service="${process}.service"
binary="/usr/bin/$process"
backup="${binary}.previous"
staged="${binary}.new"
# One lock covers all processes because USB updates also stop the player.
lock=/run/process-deploy.lock
was_active=0
player_active=0
replaced=0
committed=0

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
            systemctl stop "$service" || true
            if cp -p "$backup" "$staged" && mv -f "$staged" "$binary"; then
                echo "Previous binary restored." >&2
            else
                echo "ERROR: Rollback failed; recover $backup using the serial console." >&2
            fi
        fi
        if [ "$was_active" = 1 ]; then
            systemctl start "$service" || true
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

if systemctl is-active --quiet "$service"; then was_active=1; fi
# Only USB needs the player stopped to release open media files before unmount.
if [ "$process" = usb-monitor ] && systemctl is-active --quiet ubs-video-player.service; then
    player_active=1
fi

# Prepare files before stopping services. Rename on the same filesystem is atomic.
test -f "$binary"
chmod 0755 "$upload"
cp "$upload" "$staged"
cp -p "$binary" "$backup.new"
mv -f "$backup.new" "$backup"
if [ "$player_active" = 1 ]; then
    systemctl stop ubs-video-player.service
fi
systemctl stop "$service"
mv -f "$staged" "$binary"
replaced=1
systemctl start "$service"
sleep 2
systemctl is-active --quiet "$service"
committed=1
echo "$process updated; backup: $backup"
REMOTE
REMOTE_FILE=""
echo "Deployment completed."
if [[ "$FOLLOW_LOGS" == 1 ]]; then
    ssh "$TARGET" "journalctl -u $SERVICE -f -o cat"
else
    echo "Logs: ssh $TARGET 'journalctl -u $SERVICE -f -o cat'"
fi
