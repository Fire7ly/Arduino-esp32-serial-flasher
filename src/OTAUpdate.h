#ifndef OTA_UPDATE_H
#define OTA_UPDATE_H

#include <Arduino.h>
#include <ArduinoJson.h>

struct UpdateInfo {
    bool available;
    String version;
    String url;
    String releaseNotes;
    String error;
};

class OTAUpdateClass {
public:
    UpdateInfo checkForUpdate();
    String performUpdate(String url);
    UpdateInfo getCachedUpdateInfo();

private:
    UpdateInfo _cachedInfo;
    int compareVersions(String v1, String v2);
};

extern OTAUpdateClass OTAUpdate;

#endif
