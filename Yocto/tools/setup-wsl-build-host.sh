#!/usr/bin/env bash
set -Eeuo pipefail

# =============================================================================
# Script: setup-wsl-build-host.sh
# Purpose: Prepare Ubuntu 22.04 WSL2 as a Yocto build host
# Target: Yocto Project Kirkstone
# =============================================================================

readonly SCRIPT_NAME="$(basename "$0")"

log() {
    printf '[INFO] %s\n' "$*"
}

warn() {
    printf '[WARN] %s\n' "$*" >&2
}

die() {
    printf '[ERROR] %s\n' "$*" >&2
    exit 1
}

# -----------------------------------------------------------------------------
# 1. Check execution environment
# -----------------------------------------------------------------------------

if [[ "${EUID}" -eq 0 ]]; then
    die "Do not run this script with sudo. Run it as a normal user."
fi

if [[ ! -f /etc/os-release ]]; then
    die "Cannot identify the operating system."
fi

# shellcheck disable=SC1091
source /etc/os-release

if [[ "${ID:-}" != "ubuntu" ]]; then
    die "This script supports Ubuntu only."
fi

if [[ "${VERSION_ID:-}" != "22.04" ]]; then
    die "Expected Ubuntu 22.04; detected ${PRETTY_NAME:-unknown}."
fi

log "Operating system: ${PRETTY_NAME}"

if grep -qiE 'microsoft|wsl' /proc/sys/kernel/osrelease; then
    log "WSL environment detected."
else
    warn "WSL was not detected. Continuing with Ubuntu host setup."
fi

if [[ "$(uname -m)" != "x86_64" ]]; then
    warn "Host architecture is $(uname -m), not x86_64."
fi

# -----------------------------------------------------------------------------
# 2. Check sudo access
# -----------------------------------------------------------------------------

command -v sudo >/dev/null 2>&1 ||
    die "sudo is not installed."

log "Checking sudo access..."
sudo -v

# -----------------------------------------------------------------------------
# 3. Update package index
# -----------------------------------------------------------------------------

log "Updating APT package index..."
sudo apt-get update

# -----------------------------------------------------------------------------
# 4. Install Yocto build dependencies
# -----------------------------------------------------------------------------

readonly -a REQUIRED_PACKAGES=(
    build-essential
    chrpath
    cpio
    debianutils
    diffstat
    file
    gawk
    gcc
    git
    iputils-ping
    libacl1
    liblz4-tool
    locales
    python3
    python3-git
    python3-jinja2
    python3-pexpect
    python3-pip
    python3-subunit
    socat
    texinfo
    unzip
    wget
    xz-utils
    zstd
)

log "Installing Yocto build dependencies..."
sudo env DEBIAN_FRONTEND=noninteractive \
    apt-get install -y --no-install-recommends \
    "${REQUIRED_PACKAGES[@]}"

# -----------------------------------------------------------------------------
# 5. Configure UTF-8 locale
# -----------------------------------------------------------------------------

log "Configuring en_US.UTF-8 locale..."

sudo locale-gen en_US.UTF-8
sudo update-locale LANG=en_US.UTF-8

# -----------------------------------------------------------------------------
# 6. Verify essential tools
# -----------------------------------------------------------------------------

log "Checking installed tools..."

for tool in \
    gcc \
    g++ \
    make \
    git \
    python3 \
    perl \
    tar \
    gzip \
    bzip2 \
    xz \
    wget
do
    if command -v "${tool}" >/dev/null 2>&1; then
        printf '  [OK] %s: %s\n' \
            "${tool}" "$(command -v "${tool}")"
    else
        warn "Tool not found: ${tool}"
    fi
done

# -----------------------------------------------------------------------------
# 7. Display environment information
# -----------------------------------------------------------------------------

echo
log "Host setup completed."
echo
echo "Ubuntu      : ${PRETTY_NAME}"
echo "Architecture: $(uname -m)"
echo "Kernel      : $(uname -r)"
echo "Python      : $(python3 --version)"
echo "Git         : $(git --version)"
echo
echo "Next steps:"
echo "  1. Reopen the terminal if locale changes are not visible."
echo "  2. Prepare the Yocto repositories and project layers."
echo "  3. Run init-build.sh to initialize the build environment."
echo
