#!/usr/bin/env bash
set -euo pipefail

project_dir=$(cd -- "$(dirname -- "$0")" && pwd)
build_dir="$project_dir/build"
firmware_dir="$project_dir/../firmware"

if [[ -z "${IDF_PATH:-}" || -z "${IDF_PYTHON_ENV_PATH:-}" ]]; then
    printf 'Activate ESP-IDF v6.1 before running this script.\n' >&2
    exit 2
fi

cd "$project_dir"
idf_cmd=("$IDF_PYTHON_ENV_PATH/bin/python" "$IDF_PATH/tools/idf.py")

"${idf_cmd[@]}" \
    -DIDF_TARGET=esp32c6 \
    -DSDKCONFIG="$project_dir/sdkconfig.esp32c6-hosted" \
    -B "$build_dir" build

"${idf_cmd[@]}" -B "$build_dir" merge-bin \
    --output "$build_dir/esp32c6-hosted-2.12.13.bin"

mkdir -p "$firmware_dir/bootloader" "$firmware_dir/partition_table"
cp "$build_dir/bootloader/bootloader.bin" "$firmware_dir/bootloader/bootloader.bin"
cp "$build_dir/partition_table/partition-table.bin" "$firmware_dir/partition_table/partition-table.bin"
cp "$build_dir/ota_data_initial.bin" "$firmware_dir/ota_data_initial.bin"
cp "$build_dir/network_adapter.bin" "$firmware_dir/network_adapter.bin"
cp "$build_dir/esp32c6-hosted-2.12.13.bin" "$firmware_dir/esp32c6-hosted-2.12.13.bin"
cp "$build_dir/flash_args" "$firmware_dir/flash_args"
cp "$build_dir/flasher_args.json" "$firmware_dir/flasher_args.json"

cd "$project_dir/.."
sha256sum \
    firmware/bootloader/bootloader.bin \
    firmware/partition_table/partition-table.bin \
    firmware/ota_data_initial.bin \
    firmware/network_adapter.bin \
    firmware/esp32c6-hosted-2.12.13.bin > firmware/SHA256SUMS

printf 'ESP32-C6 firmware updated in %s\n' "$firmware_dir"
