#include "SDStorage.h"
#include "ConfigFile.h"

SDManager SDStorage;

bool SDManager::begin() {
    #ifdef USE_SD_CARD
        // SPI Pin configuration
        SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
        
        if (!SD.begin(SD_CS_PIN)) {
            Serial.println("Warning: SD Mount Failed");
            return false;
        }
        
        uint8_t cardType = SD.cardType();
        if (cardType == CARD_NONE) {
            Serial.println("No SD card attached");
            return false;
        }
        Serial.println("SD Card Initialized");
        return true;
    #else
        if (!SPIFFS.begin(true)) {
            Serial.println("An Error has occurred while mounting SPIFFS");
            return false;
        }
        Serial.println("SPIFFS Initialized");
        return true;
    #endif
}

void SDManager::printCardInfo() {
    #ifdef USE_SD_CARD
        uint64_t cardSize = SD.cardSize() / (1024 * 1024);
        Serial.printf("Storage Size: %lluMB\n", cardSize);
    #else
        size_t totalBytes = SPIFFS.totalBytes();
        size_t usedBytes = SPIFFS.usedBytes();
        Serial.printf("Storage: %u / %u bytes used\n", usedBytes, totalBytes);
    #endif
}

std::vector<String> SDManager::listFiles(const char * dirname) {
    std::vector<String> fileList;
    #ifdef USE_SD_CARD
        listDir(SD, dirname, 0, fileList);
    #else
        listDir(SPIFFS, dirname, 0, fileList);
    #endif
    return fileList;
}

File SDManager::openFile(const char * path) {
    #ifdef USE_SD_CARD
        return SD.open(path);
    #else
        return SPIFFS.open(path);
    #endif
}

void SDManager::listDir(fs::FS &fs, const char * dirname, uint8_t levels, std::vector<String> &fileList) {
    File root = fs.open(dirname);
    if (!root) {
        Serial.println("Failed to open directory");
        return;
    }
    if (!root.isDirectory()) {
        Serial.println("Not a directory");
        return;
    }

    File file = root.openNextFile();
    while (file) {
        if (file.isDirectory()) {
            if (levels) {
                listDir(fs, file.name(), levels - 1, fileList);
            }
        } else {
            String fileName = String(file.name());
            Serial.printf("Found file: %s (%u bytes)\n", fileName.c_str(), file.size());
            // Filter for .bin files only
            if (fileName.endsWith(".bin")) {
                 // Format: /filename.bin|Size
                 String fileEntry = fileName + "|" + String(file.size());
                 fileList.push_back(fileEntry);
            }
        }
        file = root.openNextFile();
    }
}
