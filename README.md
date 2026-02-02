# ESP32-S3 Advanced Web + SD Flasher

## Overview

This project turns an ESP32-S3 into a standalone firmware programmer.
Features:

- **Web Portal**: Upload firmware via browser.
- **SD Card Storage**: Host `.bin` files on an SD card.
- **Progress Tracking**: Watch the flashing progress on the Web UI.

## Hardware Setup

**Host**: ESP32-S3 (Required for USB-CDC native serial + SD_MMC performance, though code falls back to SPI for generic pinouts).

**Connections**:

- **Target TX** <-> **GPIO 44** (RX)
- **Target RX** <-> **GPIO 43** (TX)
- **Target RST** <-> **GPIO 1**
- **Target IO0** <-> **GPIO 2**
- **SD Card**: Attached to SPI pins defined in `src/ConfigFile.h`.

## Software Setup

1.  Open `Advanced_Web_Flasher.ino` in Arduino IDE / VS Code.
2.  Install dependencies:
    - `ESPAsyncWebServer`
    - `AsyncTCP`
3.  Check `src/ConfigFile.h` to match your pinout.
4.  Upload to ESP32-S3.

## Usage

1.  Power up the ESP32-S3.
2.  Connect to WiFi AP: `ESP32-Flasher` (Password: `12345678`).
3.  Go to `http://192.168.4.1`.
4.  **Upload**: Select your firmware `.bin` file and click Upload.
5.  **Flash**: Click "Flash This" next to the uploaded file.
6.  Monitor status on the page or via Serial Monitor (115200).
