#include <Arduino.h>
#include <SPI.h>
#include "src/ConfigFile.h"
#include "src/FlasherTask.h"
#include "src/SDStorage.h"

#ifdef ENABLE_WEB_PORTAL
  #include <WiFi.h>
  #include "src/WebPortal.h"
#endif

#ifdef ENABLE_WEB_PORTAL
void setupWiFi() {
    Serial.println("Setting up WiFi...");
    WiFi.mode(WIFI_AP_STA);

    // 1. Access Point (For User Control)
    Serial.print("Starting AP: ");
    Serial.println(WIFI_SSID);
    WiFi.softAP(WIFI_SSID, WIFI_PASS);
    
    IPAddress IP = WiFi.softAPIP();
    Serial.print("AP IP address: ");
    Serial.println(IP);

    // 2. Station (For OTA Updates)
    Serial.print("Connecting to Internet (STA): ");
    Serial.println(STA_SSID);
    WiFi.begin(STA_SSID, STA_PASS);
    // We don't block here. OTA will check connection status later.
}
#endif


void setup() {
    Serial.begin(115200);
    delay(1000);
    Serial.println("\n--- ESP32-S3 Advanced Web Flasher ---");

    // Initialize SD Card
    if (!SDStorage.begin()) {
        Serial.println("Warning: SD Init Failed! Web features requiring SD will not work.");
        // We continue anyway so WiFi/WebPortal can utilize what they can (or allow upload?)
    }
    
    // Initialize Flasher Task
    Flasher.begin();

    #ifdef ENABLE_WEB_PORTAL
      // Setup WiFi
      setupWiFi();

      // Start Web Portal
      WebManager.begin();
    #else
      Serial.println("Web Portal Disabled via config.");
    #endif

    Serial.println("Ready.");
}

void loop() {
    #ifdef ENABLE_WEB_PORTAL
      // Web server handle client is needed if not async, but we will likely use AsyncWebServer
      WebManager.loop(); // Call loop just in case, though Async usually handles itself
    #endif
    delay(100);
}