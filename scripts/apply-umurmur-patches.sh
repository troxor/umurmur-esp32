#!/usr/bin/env bash
# Reset third_party/umurmur to the pinned commit and apply patches/*.patch in order.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UMURMUR="${ROOT}/third_party/umurmur"
PATCH_DIR="${ROOT}/patches"

if [[ ! -d "${UMURMUR}/.git" && ! -f "${UMURMUR}/.git" ]]; then
  echo "error: ${UMURMUR} is not a git submodule checkout" >&2
  exit 1
fi

git -C "${UMURMUR}" reset --hard
git -C "${UMURMUR}" clean -fd

shopt -s nullglob
patches=("${PATCH_DIR}"/*.patch)
if ((${#patches[@]} == 0)); then
  echo "warning: no patches in ${PATCH_DIR}" >&2
  exit 0
fi

for p in "${patches[@]}"; do
  echo "Applying $(basename "$p")"
  git -C "${UMURMUR}" apply "$(realpath "$p")"
done

echo "umurmur patches applied OK"
