#include "OTAUpdate.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "ConfigFile.h"
#include "FlasherTask.h"
#include <esp_task_wdt.h>

OTAUpdateClass OTAUpdate;

// GitHub API root
const char* GITHUB_API_URL = "https://api.github.com/repos/" GITHUB_REPO "/releases/latest";

UpdateInfo OTAUpdateClass::checkForUpdate() {
    UpdateInfo info = {false, "", "", "", ""};
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("WiFi not connected. Attempting to reconnect...");
        WiFi.begin(STA_SSID, STA_PASS);
        
        int timeout = 0;
        while(WiFi.status() != WL_CONNECTED && timeout < 20) { // 10 seconds (20 * 500ms)
            delay(500);
            esp_task_wdt_reset(); // Feed WDT
            Serial.print(".");
            timeout++;
        }
        Serial.println();
        
        if (WiFi.status() != WL_CONNECTED) {
            info.error = "WiFi Connection Failed";
            Serial.println("Error: " + info.error);
            Flasher.setStatus("OTA Error: " + info.error);
            return info;
        } else {
            Serial.print("Connected! IP: ");
            Serial.println(WiFi.localIP());
            Flasher.setStatus("WiFi Connected: " + WiFi.localIP().toString());
        }
    }

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure(); // Skip cert validation for simplicity

    Serial.println("Checking for updates...");
    
    if (http.begin(client, GITHUB_API_URL)) {
        http.setUserAgent("ESP32-Flasher"); // GitHub requires User-Agent
        int httpCode = http.GET();

        if (httpCode == HTTP_CODE_OK) {
            String payload = http.getString();
            JsonDocument doc;
            DeserializationError error = deserializeJson(doc, payload);

            if (!error) {
                String latestVersion = doc["tag_name"].as<String>();
                info.version = latestVersion;
                info.releaseNotes = doc["body"].as<String>();
                
                // Check if assets exist
                JsonArray assets = doc["assets"];
                for (JsonObject asset : assets) {
                    const char* name = asset["name"];
                    if (String(name).endsWith(".bin")) {
                        info.url = asset["browser_download_url"].as<String>();
                        break;
                    }
                }
                
                // Smart Version Comparison
                int comparison = compareVersions(latestVersion, FIRMWARE_VERSION);
                if (comparison > 0) {
                    info.available = true;
                    Serial.printf("New version available: %s (Current: %s)\n", latestVersion.c_str(), FIRMWARE_VERSION);
                } else {
                    info.available = false; // Explicitly ensure false if not newer
                    Serial.printf("Firmware is up to date (Current: %s, Cloud: %s)\n", FIRMWARE_VERSION, latestVersion.c_str());
                }
            } else {
                Serial.println("Error: JSON parsing failed");
            }
        } else {
            info.error = "HTTP API Error " + String(httpCode);
            Serial.printf("Error: %s\n", info.error.c_str());
        }
        http.end();
    } else {
        info.error = "Connection to GitHub API failed";
        Serial.println("Error: " + info.error);
    }
    
    _cachedInfo = info; // Cache result
    return info;
}

// Helper to compare semantic versions: 1 if v1 > v2, -1 if v1 < v2, 0 if equal
int OTAUpdateClass::compareVersions(String v1, String v2) {
    // Remove 'v' prefix if present
    if (v1.startsWith("v")) v1 = v1.substring(1);
    if (v2.startsWith("v")) v2 = v2.substring(1);

    int i1 = 0, i2 = 0;
    while (i1 < v1.length() || i2 < v2.length()) {
        int n1 = 0, n2 = 0;
        
        while (i1 < v1.length() && v1.charAt(i1) != '.') {
            n1 = n1 * 10 + (v1.charAt(i1) - '0');
            i1++;
        }
        
        while (i2 < v2.length() && v2.charAt(i2) != '.') {
            n2 = n2 * 10 + (v2.charAt(i2) - '0');
            i2++;
        }

        if (n1 > n2) return 1;
        if (n1 < n2) return -1;

        i1++;
        i2++;
    }
    return 0;
}

UpdateInfo OTAUpdateClass::getCachedUpdateInfo() {
    return _cachedInfo;
}

String OTAUpdateClass::performUpdate(String url) {
    if (url.length() == 0) return "Error: No download URL";
    
    Serial.println("Starting OTA Update...");
    Serial.println(url);
    Flasher.setStatus("OTA: Downloading...");

    HTTPClient http;
    WiFiClientSecure client;
    client.setInsecure();
    
    // Follow redirects is handled by HTTPClient usually, but verify
    http.setFollowRedirects(HTTPC_STRICT_FOLLOW_REDIRECTS);
    
    if (http.begin(client, url)) {
        int httpCode = http.GET();
        if (httpCode == HTTP_CODE_OK) {
            int contentLength = http.getSize();
            Serial.printf("Download size: %d\n", contentLength);
            Flasher.setStatus("OTA: Size " + String(contentLength) + " bytes");

            if(contentLength > 0) {
                bool canBegin = Update.begin(contentLength);
                if (canBegin) {
                    Flasher.setStatus("OTA: Begin OK. Downloading...");
                    
                    // Manual Stream Loop for Progress
                    WiFiClient *stream = http.getStreamPtr();
                    uint8_t buff[1024];
                    size_t written = 0;
                    size_t total = contentLength;
                    int lastProgress = -1;
                    
                    while(written < total) {
                        size_t size = stream->available();
                        if(size) {
                            int c = stream->readBytes(buff, ((size > sizeof(buff)) ? sizeof(buff) : size));
                            size_t ret = Update.write(buff, c);
                             if (ret != c) {
                                Flasher.setStatus("OTA Write Error!");
                                return "Error: Write mismatch";
                             }
                            written += c;
                            
                            // Log every 10%
                            int progress = (written * 100) / total;
                            if(progress != lastProgress) {
                                Flasher.setStatus("OTA: " + String(progress) + "%");
                                lastProgress = progress;
                            }
                        }
                        delay(1);
                        esp_task_wdt_reset(); // Feed WDT
                    }
                    
                    if (written == contentLength) {
                        Serial.println("Written : " + String(written) + " successfully");
                    } else {
                        Serial.println("Written only : " + String(written) + "/" + String(contentLength) + ". Retry?");
                    }

                    if (Update.end()) {
                        Serial.println("OTA done!");
                        if (Update.isFinished()) {
                            Serial.println("Update successfully completed. Rebooting...");
                            return "Success";
                        } else {
                            Serial.println("Update not finished? Something went wrong!");
                            return "Error: Update not finished";
                        }
                    } else {
                        String code = String(Update.getError());
                        Serial.println("Error Occurred. Error #: " + code);
                        Flasher.setStatus("OTA Error Code: " + code);
                        return "Error: " + code;
                    }
                } else {
                    Serial.println("Not enough space to begin OTA");
                    Flasher.setStatus("OTA Error: Not enough space");
                    return "Error: Not enough space";
                }
            } else {
                Flasher.setStatus("OTA Error: Content-Length is 0");
                return "Error: Content-Length is 0";
            }
        } else {
            Flasher.setStatus("OTA HTTP Error: " + String(httpCode));
            return "Error: HTTP Code " + String(httpCode);
        }
        http.end();
    } else {
        return "Error: Connection failed";
    }
    return "Unknown Error";
}
