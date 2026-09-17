#!/usr/bin/env bash
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
VERSION="$("${ROOT}/scripts/umurmur-version.sh" version)"
IDF_IMAGE="${IDF_IMAGE:-espressif/idf:v6.1}"
TARGETS=(esp32 esp32s3)

idf() {
  docker run --rm \
    --user "$(id -u):$(id -g)" -e HOME=/tmp \
    -v "${ROOT}:/project" -w /project \
    "${IDF_IMAGE}" \
    bash -lc "$*"
}

cd "${ROOT}"
for target in "${TARGETS[@]}"; do
  build_dir="build-${target}"
  zip_name="umurmur-${target}-${VERSION}.zip"

  echo "==> ${target}: configure + build"
  idf "idf.py -B ${build_dir} set-target ${target} && idf.py -B ${build_dir} build"

  echo "==> ${target}: merge"
  idf "cd ${build_dir} && esptool.py --chip ${target} merge_bin -o umurmur-${target}-merged.bin @flash_args"

  echo "==> ${target}: zip ${zip_name}"
  ( cd "${build_dir}" && zip "../${zip_name}" \
      bootloader/bootloader.bin \
      partition_table/partition-table.bin \
      umurmur-esp.bin \
      umurmur-${target}-merged.bin \
      flash_args \
      flasher_args.json )

  echo "wrote ${zip_name}"
done
