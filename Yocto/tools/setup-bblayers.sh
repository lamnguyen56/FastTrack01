#!/usr/bin/env bash
set -euo pipefail

# Locate the project and template directories.
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PROJECT_DIR="$(dirname "${SCRIPT_DIR}")"
POKY_DIR="${PROJECT_DIR}/poky"
TEMPLATE="${SCRIPT_DIR}/bblayers.conf.template"
# Determine the active build directory.
if [[ -n "${BUILDDIR:-}" ]]; then
    BUILD_DIR="${BUILDDIR}"
elif [[ -f "$(pwd)/conf/bblayers.conf" ]]; then
    BUILD_DIR="$(pwd)"
else
    echo "ERROR: Build directory not found."
    echo "Run this script after source oe-init-build-env."
    exit 1
fi

CONF="${BUILD_DIR}/conf/bblayers.conf"

# Verify the template exists.
if [[ ! -f "${TEMPLATE}" ]]; then
    echo "ERROR: Template not found: ${TEMPLATE}"
    exit 1
fi

# Verify that the expected layer repositories exist.
for layer in \
    "${POKY_DIR}/meta" \
    "${POKY_DIR}/meta-poky" \
    "${POKY_DIR}/meta-yocto-bsp" \
    "${PROJECT_DIR}/meta-raspberrypi" \
    "${PROJECT_DIR}/meta-openembedded/meta-oe" \
    "${PROJECT_DIR}/meta-qt5" \
    "${PROJECT_DIR}/meta-ubs-video-player"
do
    if [[ ! -f "${layer}/conf/layer.conf" ]]; then
        echo "ERROR: Layer configuration not found: ${layer}/conf/layer.conf"
        exit 1
    fi
done

# Install the template as the complete bblayers.conf.
# Re-running this operation does not append duplicate layer entries.
mkdir -p "$(dirname "${CONF}")"
cp "${TEMPLATE}" "${CONF}"

echo "bblayers.conf updated successfully."
echo "Build directory: ${BUILD_DIR}"
echo "Configuration: ${CONF}"
