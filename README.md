# ESP32 Advanced Web Flasher

A powerful, web-based serial flasher running on an ESP32-S3. This tool allows you to flash firmware (`.bin`) files to other Expressif chips (ESP32, ESP32-S3, ESP8266, etc.) directly from your browser, without needing a PC or USB-to-Serial adapter.

## 🚀 Key Features

- **Web-Based Interface**: A clean, modern, and responsive UI for managing files and flashing operations.
- **Standalone Flasher**: Uses the ESP32-S3 as a host to flash target devices via UART.
- **File Manager**:
  - Upload firmware files (`.bin`) directly from your computer.
  - Manage files on the device (Download, Rename, Delete).
  - Supports storage via SD Card or internal SPIFFS.
- **Smart OTA Updates**:
  - Automatically checks for system updates from GitHub Releases.
  - Supports Semantic Versioning (only notifies for new versions).
  - One-click update from the Web UI.
- **Target Support**: capable of flashing:
  - ESP32
  - ESP32-S3
  - ESP32-C3
  - ESP32-S2
  - ESP8266
- **Dual WiFi Mode**: Works as an Access Point (SoftAP) and connects to your local Router (Station Mode) simultaneously.

## 🛠️ Installation

1.  **Download Latest Release**:
    Go to the [Releases Page](../../releases/latest) and download `firmware-vX.X.X.bin`.
2.  **Flash to Host ESP32**:
    Use [esptool.py](https://github.com/espressif/esptool) or the [Espressif Web Flasher](https://espressif.github.io/esptool-js/) to flash this binary to your host ESP32 device at address `0x10000` (or `0x0` depending on your bootloader setup).
    - _Note: If doing a fresh install, ensure you also flash the `bootloader.bin` and `partitions.bin` included in the release._

## 📖 Usage Guide

### 1. Connecting

- Power on the ESP32 Flasher.
- Connect your computer or phone to the WiFi Hotspot named:
  - **SSID**: `ESP32-Flasher`
  - **Password**: `12345678` (default)
- Open your browser and navigate to: `http://192.168.4.1`

### 2. Flashing a Target Device

1.  Connect the **Target ESP** to the **Host ESP** via UART:
    - **Host TX** -> **Target RX**
    - **Host RX** -> **Target TX**
    - **GND** -> **GND**
    - **IO0** (Target) -> **GND** (to put in boot mode)
    - **RST** (Target) -> **Host IO** (optional, for auto-reset)
2.  Open the **Web Interface**.
3.  Go to the **File Manager** and upload your Firmware (`.bin`) and Partition Table (`.bin`).
4.  Return to the Home Page.
5.  Select the **Target Chip** type (e.g., ESP32, ESP8266).
6.  Select the **Files** for each slot (Firmware, Partitions, etc.).
7.  Click **Start Flashing**.

### 3. System Updates (OTA)

- The device automatically checks for updates when connected to the internet.
- If a new version is available, a yellow banner will appear at the top of the dashboard.
- Click **Update Now** to wirelessly upgrade the flasher firmware.

## ⚙️ Configuration

You can modify `src/ConfigFile.h` to change default settings:

- `WIFI_SSID` / `WIFI_PASS`: Default SoftAP credentials.
- `STA_SSID` / `STA_PASS`: Router credentials for internet access (required for OTA).
- `GITHUB_REPO`: The repository to check for updates.

## 📄 License

This project is open-source. Feel free to modify and distribute.

## 🗺️ Roadmap / To-Do

- [ ] **Display Support**: Add support for lcd screens to show status and IP.
- [ ] **Input Keys**: Implement physical buttons for navigation and triggering actions.
- [ ] **FTP Support**: Add FTP server for easier file management.
