#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ver="$("$ROOT/scripts/umurmur-version.sh" version)"
tag="$("$ROOT/scripts/umurmur-version.sh" tag)"
sem="$("$ROOT/scripts/umurmur-version.sh" semver)"
n="$("$ROOT/scripts/umurmur-version.sh" patch_count)"
[[ "$sem" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "bad semver: $sem"; exit 1; }
[[ "$n" =~ ^[0-9]+$ ]] || { echo "bad patch_count: $n"; exit 1; }
if [[ "$n" -eq 0 ]]; then
  [[ "$ver" == "esp32-$sem" && "$tag" == "esp32-$sem" ]] || { echo "mismatch empty patches"; exit 1; }
else
  [[ "$ver" == "esp32-$sem+p$n" && "$tag" == "esp32-$sem-p$n" ]] || { echo "mismatch patched"; exit 1; }
fi
echo "umurmur-version OK: ver=$ver tag=$tag"
