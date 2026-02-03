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
};

extern OTAUpdateClass OTAUpdate;

#endif
