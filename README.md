# ESP32 uMurmur

Minimal Mumble server on ESP32

## Upstream uMurmur

This project consumes [umurmur/umurmur](https://github.com/umurmur/umurmur) as a git submodule:

- Path: `third_party/umurmur`
- Local core deltas: `patches/*.patch` (applied by `scripts/apply-umurmur-patches.sh` during CMake configure)

```bash
git submodule update --init --recursive
```

### Bump upstream

```bash
git -C third_party/umurmur fetch origin
git -C third_party/umurmur checkout <new-tag-or-sha>
git add third_party/umurmur
# Update the Pin line above to the new SHA.
./scripts/apply-umurmur-patches.sh   # fix/refresh patches/ if this fails
```

The configure-time patch step runs `git reset --hard` and `git clean -fd` in the submodule, destroying any hand edits there.

Prefer the automated path: `.github/workflows/watch-upstream.yml` checks `umurmur/umurmur` stable releases daily (or on demand) and opens a bump PR. Use the manual steps below only when you need to pin a specific tag/SHA outside that flow.

## Versioning

`UMURMUR_VERSION` is derived at build time by `scripts/umurmur-version.sh`:

- Base: upstream semver from `third_party/umurmur/CMakeLists.txt` (`project(... VERSION ...)`)
- Prefix: `esp32-`
- If `patches/*.patch` is non-empty: append `+pN` (N = patch file count)

Examples: `esp32-0.4.1`, `esp32-0.4.1+p1`.

GitHub Release tags use a hyphen instead of `+` (`esp32-0.4.1-p1`) so tags stay URL-friendly.

### Automated upstream follow

- `.github/workflows/watch-upstream.yml` — daily/manual check of `umurmur/umurmur` stable releases; opens a bump PR (draft if patches fail to apply).
- `.github/workflows/release.yml` — after merge to `master` (paths that affect firmware), builds **esp32** firmware and publishes a GitHub Release.

ESP32-S3 release artifacts are deferred.

## Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v5.x (ships **mbedTLS 3.x**)
- ESP32 or ESP32-S3

## Build / flash

### Docker

```bash
docker run -it --rm \
  --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:/project" -w /project espressif/idf:v5.3.2 \
  idf.py ...
```

### Installation with ESP-IDF Framework

```bash
idf.py set-target esp32        # or esp32s3
idf.py menuconfig              # See "uMurmur ESP" section
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## ESP32 NVS Configuration

Namespace: `umurmur`

| Key | Type | Meaning |
|-----|------|---------|
| `wifi_ssid` | string | STA SSID |
| `wifi_pass` | string | STA password |
| `password` | string | Mumble server password |
| `admin_password` | string | Admin password |
| `cert_pem` | blob | PEM certificate (auto-generated ECDSA P-256 on first boot if empty) |
| `key_pem` | blob | PEM private key (paired with `cert_pem`) |
