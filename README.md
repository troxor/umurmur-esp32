# ESP32 uMurmur

Minimal Mumble server on ESP32

## Upstream uMurmur

This project consumes [umurmur/umurmur](https://github.com/umurmur/umurmur) as a git submodule:

- Path: `third_party/umurmur`
- Local core deltas: `patches/*.patch` (applied by `scripts/apply-umurmur-patches.sh` during CMake configure)

```bash
git submodule update --init
```

### Bump upstream

Pin the submodule to an upstream **release tag** (e.g. `v0.4.1`). The release workflow also does this automatically (daily / manual).

```bash
git -C third_party/umurmur fetch --tags origin
git -C third_party/umurmur checkout v0.4.1
git add third_party/umurmur
./scripts/apply-umurmur-patches.sh   # refresh patches/ if this fails
```

The configure-time patch step runs `git reset --hard` and `git clean -fd` in the submodule, destroying any hand edits there.

## Versioning

`UMURMUR_VERSION` comes from `scripts/umurmur-version.sh`:

| Part | Source |
|------|--------|
| `esp32-` | product prefix (full string is a product label, not pure SemVer) |
| `0.4.1` | upstream semver |
| `+<shortsha>` | this repo’s `git rev-parse --short HEAD` (SemVer **build metadata**) |
| `.pM` | only if `patches/*.patch` is non-empty — local patch count |

Examples: `esp32-0.4.1+a1b2c3d`, `esp32-0.4.1+a1b2c3d.p1`.

GitHub Release tags use the **same** string (including `+`).

### Firmware releases

`.github/workflows/release.yml` (cron + `workflow_dispatch`):

1. Check out the latest stable `umurmur/umurmur` release tag into the submodule (commit + push if the pin moved).
2. Apply `patches/`, derive the version string.
3. Build **esp32** firmware in `espressif/idf:v5.5`.
4. Publish a flashable zip via [`softprops/action-gh-release`](https://github.com/softprops/action-gh-release) (skipped if that version tag already exists).

ESP32-S3 release artifacts are deferred.

## Requirements

- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v5.x (ships **mbedTLS 3.x**)
- ESP32 or ESP32-S3

## Build / flash

### Docker

```bash
docker run -it --rm \
  --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:/project" -w /project espressif/idf:v5.5 \
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
