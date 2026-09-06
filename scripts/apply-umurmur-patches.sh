#!/usr/bin/env bash
# Reset third_party/umurmur to the pinned commit and apply patches/*.patch in order.
set -euo pipefail

ROOT="$(cd "$(dirname "${BASH_SOURCE[0]}")/.." && pwd)"
UMURMUR="${ROOT}/third_party/umurmur"
PATCH_DIR="${ROOT}/patches"

# Docker/CI bind-mounts often trip git's "dubious ownership" check.
git_um() { git -c safe.directory='*' -C "${UMURMUR}" "$@"; }

if [[ ! -d "${UMURMUR}/.git" && ! -f "${UMURMUR}/.git" ]]; then
  echo "error: ${UMURMUR} is not a git submodule checkout" >&2
  exit 1
fi

git_um reset --hard
git_um clean -fd

shopt -s nullglob
patches=("${PATCH_DIR}"/*.patch)
if ((${#patches[@]} == 0)); then
  echo "warning: no patches in ${PATCH_DIR}" >&2
  exit 0
fi

for p in "${patches[@]}"; do
  echo "Applying $(basename "$p")"
  git_um apply "$(realpath "$p")"
done

echo "umurmur patches applied OK"
