#!/usr/bin/env bash
set -euo pipefail

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
TEMPLATE="${SCRIPT_DIR}/local.conf.template"

if [[ -n "${BUILDDIR:-}" ]]; then
    CONF="${BUILDDIR}/conf/local.conf"
else
    CONF="$(pwd)/conf/local.conf"
fi

BEGIN_MARKER="# >>> YOCTO_USER_CONFIG (managed by setup-build.sh) >>>"
END_MARKER="# <<< YOCTO_USER_CONFIG (managed by setup-build.sh) <<<"

if [[ ! -f "${TEMPLATE}" ]]; then
    echo "ERROR: Template not found: ${TEMPLATE}"
    exit 1
fi

if [[ ! -f "${CONF}" ]]; then
    echo "ERROR: local.conf not found: ${CONF}"
    echo "Run this script after source oe-init-build-env."
    exit 1
fi

# Validate markers before changing anything, then append or replace once.
python3 - "${CONF}" "${TEMPLATE}" "${BEGIN_MARKER}" "${END_MARKER}" <<'PY'
from pathlib import Path
import sys

conf = Path(sys.argv[1])
template = Path(sys.argv[2])
begin = sys.argv[3]
end = sys.argv[4]

text = conf.read_text()
block = begin + "\n" + template.read_text().rstrip() + "\n" + end
if begin not in text and end not in text:
    updated = text.rstrip("\n") + "\n\n" + block + "\n"
else:
    if text.count(begin) != 1 or text.count(end) != 1:
        raise SystemExit("ERROR: Config markers are incomplete or duplicated. Check local.conf.")
    start = text.index(begin)
    finish = text.index(end)
    if finish < start:
        raise SystemExit("ERROR: Config markers are out of order. Check local.conf.")
    updated = text[:start] + block + text[finish + len(end):]
conf.write_text(updated)
PY
echo "Applied Yocto user config block."

echo "Config file: ${CONF}"
