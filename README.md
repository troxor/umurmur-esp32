# ESP32 uMurmur

Minimal Mumble server on ESP32

## Upstream uMurmur

This project consumes the latest release of [umurmur/umurmur](https://github.com/umurmur/umurmur) as a git submodule and applies patches on top of it for ESP32 compatibility.

## Requirements

- ESP32 or ESP32-S3 SoC microcontroller
- [ESP-IDF](https://docs.espressif.com/projects/esp-idf/) v5.x (ships **mbedTLS 3.x**)

## Installation

### Install from release zip

1. Download and extract the latest release zip.
2. Use a WebSerial browser-based ESP flasher such as [ESP Flasher App](https://espflasher.app). Upload **umurmur-esp-merged.bin**, accepting defaults in most cases.
3. On first boot, the device starts in SoftAP mode. SSID: **`umurmur-esp`** , Password: **`umurmur42`**
4. Open **http://192.168.4.1/**, set WiFi credentials and configuration fields. The board will restart, connect to the given SSID and listen for connections. 

### Install from source

If you want to configure uMurmur at build time, consider building the firmware yourself.

#### ESP-IDF Docker Image

All the tools necessary to build ESP-IDF projects are in a convenient Docker image.

```bash
docker run -it --rm \
  --user "$(id -u):$(id -g)" -e HOME=/tmp \
  -v "$PWD:/project" -w /project espressif/idf:v5.5 \
  idf.py ...
```

#### Configure, Compile, and Flash

Default wifi credentials and uMurmur configuration options can be configured in the `menuconfig` step.

```bash
git submodule --init

idf.py set-target esp32
idf.py menuconfig
idf.py build
idf.py -p /dev/ttyUSB0 flash monitor
```

## Configuration

### Values stored in NVS

Namespace: `umurmur`

| Key | Type | Meaning |
|-----|------|---------|
| `wifi_ssid` | string | STA SSID |
| `wifi_pass` | string | STA password |
| `password` | string | Mumble server password |
| `admin_password` | string | Admin password |
| `cert_pem` | blob | PEM certificate |
| `key_pem` | blob | PEM private key |
