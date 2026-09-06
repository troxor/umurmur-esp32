# ESP32 uMurmur

Minimal Mumble server on ESP32

## Upstream uMurmur

This project consumes the latest release of [umurmur/umurmur](https://github.com/umurmur/umurmur) as a git submodule and applies patches on top of it for ESP32 compatibility.

## Requirements

- ESP32 or ESP32-S3 SoC microcontroller
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v5.x (ships **mbedTLS 3.x**)

## Build / flash

### ESP-IDF Docker Image

All the tools necessary to build ESP-IDF projects are in a convenient Docker image.

```bash
docker run -it --rm \
  --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:/project" -w /project espressif/idf:v5.5 \
  idf.py ...
```

### Build and Flash

```bash
git submodule --init

idf.py set-target esp32        # or esp32s3
idf.py menuconfig              # See "uMurmur ESP" section
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

### NVS Config

Namespace: `umurmur`

| Key | Type | Meaning |
|-----|------|---------|
| `wifi_ssid` | string | STA SSID |
| `wifi_pass` | string | STA password |
| `password` | string | Mumble server password |
| `admin_password` | string | Admin password |
| `cert_pem` | blob | PEM certificate (auto-generated ECDSA P-256 on first boot if empty) |
| `key_pem` | blob | PEM private key (paired with `cert_pem`) |
