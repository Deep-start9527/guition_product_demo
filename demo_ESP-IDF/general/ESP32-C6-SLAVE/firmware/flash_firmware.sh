#!/usr/bin/env bash
set -euo pipefail

if [[ $# -ne 1 ]]; then
    printf 'Usage: %s PORT\n' "$0" >&2
    exit 2
fi

firmware_dir=$(cd -- "$(dirname -- "$0")" && pwd)
cd "$firmware_dir"
python -m esptool --chip esp32c6 -p "$1" -b 460800 \
    --before default-reset --after hard-reset write-flash \
    0x0 esp32c6-hosted-2.12.13.bin
