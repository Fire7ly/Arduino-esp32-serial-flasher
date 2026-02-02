#ifndef SD_STORAGE_H
#define SD_STORAGE_H

#include "ConfigFile.h"

#ifdef USE_SD_CARD
    #include <SD.h>
    #include <SPI.h>
#else
    #include <SPIFFS.h>
#endif
#include <vector>

class SDManager {
public:
    bool begin();
    std::vector<String> listFiles(const char * dirname);
    File openFile(const char * path);
    void printCardInfo();

private:
    void listDir(fs::FS &fs, const char * dirname, uint8_t levels, std::vector<String> &fileList);
};

extern SDManager SDStorage;

#endif
