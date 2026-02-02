#include "OTAUpdate.h"
#include <WiFi.h>
#include <HTTPClient.h>
#include <Update.h>
#include "ConfigFile.h"

OTAUpdateClass OTAUpdate;

// GitHub API root
const char* GITHUB_API_URL = "https://api.github.com/repos/" GITHUB_REPO "/releases/latest";

UpdateInfo OTAUpdateClass::checkForUpdate() {
    UpdateInfo info = {false, "", "", ""};
    
    if (WiFi.status() != WL_CONNECTED) {
        Serial.println("Error: WiFi not connected");
        return info;
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
            Serial.printf("Error: HTTP GET failed, code %d\n", httpCode);
        }
        http.end();
    } else {
        Serial.println("Error: Unable to connect to GitHub API");
    }
    
    return info;
}

String OTAUpdateClass::performUpdate(String url) {
    if (url.length() == 0) return "Error: No download URL";
    
    Serial.println("Starting OTA Update...");
    Serial.println(url);

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

            if(contentLength > 0) {
                bool canBegin = Update.begin(contentLength);
                if (canBegin) {
                    size_t written = Update.writeStream(*http.getStreamPtr());
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
                        Serial.println("Error Occurred. Error #: " + String(Update.getError()));
                        return "Error: " + String(Update.getError());
                    }
                } else {
                    Serial.println("Not enough space to begin OTA");
                    return "Error: Not enough space";
                }
            } else {
                return "Error: Content-Length is 0";
            }
        } else {
            return "Error: HTTP Code " + String(httpCode);
        }
        http.end();
    } else {
        return "Error: Connection failed";
    }
    return "Unknown Error";
}
