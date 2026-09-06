#!/usr/bin/env bash
# Derive product version from upstream umurmur semver + git SHA + local patches.
#
# Form:  esp32-<semver>+<shortsha>[.p<N>]
#   <semver>    upstream (e.g. 0.4.1)
#   <shortsha>  git rev-parse --short HEAD (this repo)
#   .p<N>       count of patches/*.patch (omit suffix when empty)
#
# After the prefix, <semver>+… is SemVer build-metadata shaped.
# GitHub Release tags use this same string (git allows '+').
set -euo pipefail
ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UM_CMAKE="${ROOT}/third_party/umurmur/CMakeLists.txt"
PATCH_DIR="${ROOT}/patches"
mode="${1:-version}"

semver="$(sed -nE 's/^project\(umurmurd VERSION ([0-9]+\.[0-9]+\.[0-9]+).*/\1/p' "${UM_CMAKE}" | head -n1)"
if [[ -z "${semver}" ]]; then
  echo "error: could not parse VERSION from ${UM_CMAKE}" >&2
  exit 1
fi

shopt -s nullglob
patches=("${PATCH_DIR}"/*.patch)
n="${#patches[@]}"

sha="unknown"
git_root() { git -c safe.directory='*' -C "${ROOT}" "$@"; }
if git_root rev-parse --is-inside-work-tree >/dev/null 2>&1; then
  sha="$(git_root rev-parse --short HEAD)"
fi

meta="+${sha}"
if [[ "${n}" -gt 0 ]]; then
  meta+=".p${n}"
fi
full="esp32-${semver}${meta}"

case "${mode}" in
  semver) echo "${semver}" ;;
  patch_count) echo "${n}" ;;
  sha) echo "${sha}" ;;
  version) echo "${full}" ;;
  *)
    echo "usage: $0 {semver|patch_count|sha|version}" >&2
    exit 2
    ;;
esac
