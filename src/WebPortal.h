#ifndef WEB_PORTAL_H
#define WEB_PORTAL_H

#include <ESPAsyncWebServer.h>
#include "SDStorage.h"

class WebPortal {
public:
    void begin();
    void loop();

private:
    static void handleUpload(AsyncWebServerRequest *request, String filename, size_t index, uint8_t *data, size_t len, bool final);
};

extern WebPortal WebManager;

#endif
