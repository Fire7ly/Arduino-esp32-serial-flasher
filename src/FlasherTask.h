#ifndef FLASHER_TASK_H
#define FLASHER_TASK_H

#include <Arduino.h>
#include <vector>
#include "esp-loader/esp_loader.h"

struct FlashFile {
    String name;
    uint32_t address;
};

class FlasherTask {
public:
    void begin();
    bool flashFirmware(String targetName, std::vector<FlashFile> files);
    bool isFlashing();
    int getProgress();
    String getStatus();
    void setStatus(String msg);

private:
    static void flasherTask(void *pvParameters);
};

extern FlasherTask Flasher;

#endif
