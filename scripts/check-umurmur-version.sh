#!/usr/bin/env bash
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
ver="$("$ROOT/scripts/umurmur-version.sh" version)"
sem="$("$ROOT/scripts/umurmur-version.sh" semver)"
n="$("$ROOT/scripts/umurmur-version.sh" patch_count)"
sha="$("$ROOT/scripts/umurmur-version.sh" sha)"
[[ "$sem" =~ ^[0-9]+\.[0-9]+\.[0-9]+$ ]] || { echo "bad semver: $sem"; exit 1; }
[[ "$n" =~ ^[0-9]+$ ]] || { echo "bad patch_count: $n"; exit 1; }
[[ "$sha" =~ ^[0-9a-f]+$ ]] || { echo "bad sha: $sha"; exit 1; }
# patch_count = number of patches/*.patch

if [[ "$n" -eq 0 ]]; then
  expect="esp32-${sem}+${sha}"
else
  expect="esp32-${sem}+${sha}.p${n}"
fi
[[ "$ver" == "$expect" ]] || {
  echo "mismatch: ver=$ver (want $expect)"
  exit 1
}
echo "umurmur-version OK: ver=$ver"
