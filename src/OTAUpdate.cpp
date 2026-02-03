#include "OTAUpdate.h"
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <HTTPClient.h>
#include <Update.h>
#include "ConfigFile.h"
#include "FlasherTask.h"

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
                
                // Simple version comparison (string inequality)
                if (latestVersion != FIRMWARE_VERSION) {
                    info.available = true;
                    Serial.printf("New version available: %s\n", latestVersion.c_str());
                } else {
                    Serial.println("Firmware is up to date.");
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
    
    return info;
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
                            if((written * 100 / total) % 10 == 0) {
                                Flasher.setStatus("OTA: " + String(written*100/total) + "%");
                            }
                        }
                        delay(1);
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
