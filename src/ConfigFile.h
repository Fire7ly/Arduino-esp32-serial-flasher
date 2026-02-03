#ifndef CONFIG_FILE_H
#define CONFIG_FILE_H

// --- WiFi Configuration ---
// For now hardcoded, can be moved to a JSON config on SD later
#define WIFI_SSID "ESP32-Flasher"
#define WIFI_PASS "12345678"

// --- Storage Configuration ---
// Comment out the line below to use SPIFFS (Internal Flash) instead of SD Card
// #define USE_SD_CARD 

// --- Web Portal Configuration ---
// Comment out the line below to DISABLE the Web Portal and WiFi
#define ENABLE_WEB_PORTAL 

// --- OTA Configuration ---
#if __has_include("Version.h")
  #include "Version.h"
#endif

#ifndef FIRMWARE_VERSION
  #define FIRMWARE_VERSION "v1.0.0" 
#endif
#define GITHUB_REPO "Fire7ly/Arduino-esp32-serial-flasher"

// --- Internet (Station) Configuration for OTA ---
// Set these to your router's credentials to allow firmware updates
#define STA_SSID "test"
#define STA_PASS "test12345"

// --- SD Card Configuration ---
// ESP32-S3 SD_MMC Pins (Default for 1-bit mode if needed, or 4-bit)
// Using SD_MMC is faster than SPI.
// Slot 1: CLK: 36, CMD: 35, D0: 37, D1: 38, D2: 33, D3: 34 (on many boards)
// Check your specific board pinout! 
// Fallback to SPI if SD_MMC is tricky on generic boards.
// Let's use SPI for maximum compatibility for now unless speed is critical.
// --- SD Card Configuration ---
// WARNING: Do NOT use pins 10-13 or 26-32 as they are connected to internal Flash/PSRAM on most ESP32-S3 modules.
// We are using safe GPIOs for SPI.
#define SD_CS_PIN    4 
#define SD_MOSI_PIN  5
#define SD_MISO_PIN  7
#define SD_SCK_PIN   6

// --- Target UART Configuration ---
#define TARGET_TX_PIN  18  // Connect to Target RX
#define TARGET_RX_PIN  17  // Connect to Target TX
#define TARGET_RST_PIN 1   // Connect to Target RST
#define TARGET_BOOT_PIN 2  // Connect to Target GPIO0

// --- Flasher Settings ---
#define FLASHER_BAUD_RATE 115200
#define FLASHER_HIGHER_BAUD 230400 // or 460800, 921600

#endif
