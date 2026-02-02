#include "FlasherTask.h"
#include "SDStorage.h"
#include "ConfigFile.h"
#include "esp-loader/esp_loader.h"
#include "esp-loader/esp_targets.h"
#include "esp-loader/serial_io.h"

// Note: Ensure esp-loader files are compatible with this include path or adjust.

FlasherTask Flasher;

static TaskHandle_t xFlasherTaskHandle = NULL;
static std::vector<FlashFile> fileQueue;
static String targetChip = "esp32";
static volatile int flashProgress = 0;
static volatile bool flashingActive = false;
static String flashStatus = "Ready";

// --- ESP Loader IO Callbacks ---
// Must be extern "C" to link with esp_loader.c

static int32_t s_time_end;

extern "C" {

uint32_t loader_port_remaining_time(void) {
    int32_t remaining = (s_time_end - millis());
    return (remaining > 0) ? remaining : 0;
}

void loader_port_start_timer(uint32_t ms) {
    s_time_end = millis() + ms;
}

esp_loader_error_t loader_port_serial_write(const uint8_t *data, uint16_t size, uint32_t timeout) {
    size_t written = Serial2.write(data, size);
    if (written != size) return ESP_LOADER_ERROR_FAIL;
    return ESP_LOADER_SUCCESS;
}

esp_loader_error_t loader_port_serial_read(uint8_t *data, uint16_t size, uint32_t timeout) {
    int read = 0;
    int32_t time_end = millis() + timeout;
    while (read < size) {
        if (millis() > time_end) return ESP_LOADER_ERROR_TIMEOUT;
        if (Serial2.available()) {
            data[read++] = Serial2.read();
        } else {
            delay(1);
        }
    }
    return ESP_LOADER_SUCCESS;
}

void loader_port_delay_ms(uint32_t ms) {
    delay(ms);
}

void loader_port_enter_bootloader(void) {
    digitalWrite(TARGET_RST_PIN, LOW);
    digitalWrite(TARGET_BOOT_PIN, LOW);
    delay(50);
    digitalWrite(TARGET_RST_PIN, HIGH); // Release Reset
    delay(50);
    digitalWrite(TARGET_BOOT_PIN, HIGH); // Release Boot
}

void loader_port_reset_target(void) {
    digitalWrite(TARGET_RST_PIN, LOW);
    delay(50);
    digitalWrite(TARGET_RST_PIN, HIGH);
}

esp_loader_error_t loader_port_change_baudrate(uint32_t baudrate) {
    Serial2.updateBaudRate(baudrate);
    return ESP_LOADER_SUCCESS;
}

void loader_port_debug_print(const char *str) {
    Serial.print(str);
}

} // extern "C"


void FlasherTask::begin() {
    // Setup Target Serial
    Serial2.begin(FLASHER_BAUD_RATE, SERIAL_8N1, TARGET_RX_PIN, TARGET_TX_PIN);
    
    // Setup Pins
    pinMode(TARGET_RST_PIN, OUTPUT);
    digitalWrite(TARGET_RST_PIN, HIGH);
    pinMode(TARGET_BOOT_PIN, OUTPUT);
    digitalWrite(TARGET_BOOT_PIN, HIGH);

    xTaskCreatePinnedToCore(flasherTask, "FlasherTask", 8192, NULL, 1, &xFlasherTaskHandle, 1);
}

bool FlasherTask::flashFirmware(String targetName, std::vector<FlashFile> files) {
    if (flashingActive) return false;
    fileQueue = files;
    targetChip = targetName;
    flashingActive = true;
    xTaskNotifyGive(xFlasherTaskHandle); // Wake up task
    return true;
}

bool FlasherTask::isFlashing() {
    return flashingActive;
}

int FlasherTask::getProgress() {
    return flashProgress;
}

String FlasherTask::getStatus() {
    return flashStatus;
}

void FlasherTask::setStatus(String msg) {
    flashStatus = msg;
}

void FlasherTask::flasherTask(void *pvParameters) {
    // Setup esp-loader config
    esp_loader_connect_args_t connect_config = ESP_LOADER_CONNECT_DEFAULT();

    while (true) {
        // Wait for notification
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        
        if (fileQueue.empty()) {
            flashStatus = "Error: No files";
            flashingActive = false;
            continue;
        }

        flashStatus = "Starting...";
        flashProgress = 0;
        Serial.println("Flasher Task Started.");
        
        // Reset Target into Bootloader
        loader_port_enter_bootloader();

        // Connect
        flashStatus = "Connecting...";
        esp_loader_error_t err = esp_loader_connect(&connect_config);
        if (err != ESP_LOADER_SUCCESS) {
            flashStatus = "Connect Error: " + String(err);
            Serial.printf("Connect Error: %d\n", err);
            flashingActive = false;
            continue;
        }
        Serial.println("Target Connected");

        // Get Target Info
        target_chip_t target = esp_loader_get_target(); 
        Serial.printf("Detected Target: %d\n", target);

        // Set Higher Baudrate
        flashStatus = "Setting Baudrate...";
        if (esp_loader_change_baudrate(FLASHER_HIGHER_BAUD) == ESP_LOADER_SUCCESS) {
             Serial2.updateBaudRate(FLASHER_HIGHER_BAUD);
             Serial.println("Baudrate Updated");
        } else {
             Serial.println("Failed to change baudrate, continuing at default");
        }

        // --- Multi-File Flash Loop ---
        int fileCount = 0;
        int totalFiles = fileQueue.size();
        bool globalSuccess = true;

        for (const auto& f : fileQueue) {
            fileCount++;
            String statusMsg = "Flashing " + String(fileCount) + "/" + String(totalFiles) + ": " + f.name;
            flashStatus = statusMsg;
            Serial.println(statusMsg);

            File binFile = SDStorage.openFile(("/" + f.name).c_str());
            if (!binFile) {
                flashStatus = "Error: " + f.name + " missing";
                Serial.println(flashStatus);
                globalSuccess = false;
                break;
            }

            uint32_t binSize = binFile.size();
            uint32_t flashAddress = f.address;
            
            err = esp_loader_flash_start(flashAddress, binSize, 4096);
            if (err != ESP_LOADER_SUCCESS) {
                flashStatus = "Erase Error: " + String(err);
                binFile.close();
                globalSuccess = false;
                break;
            }

            uint8_t buffer[4096]; // Use larger buffer usually safe with esp-loader
            uint32_t written = 0;

             while (binFile.available()) {
                size_t len = binFile.read(buffer, sizeof(buffer));
                err = esp_loader_flash_write(buffer, len);
                if (err != ESP_LOADER_SUCCESS) {
                    flashStatus = "Write Error: " + String(err);
                    break;
                }
                written += len;
                // Progress is relative to current file for simplicity, 
                // or we could calculate total job progress. Stick to per-file for now.
                flashProgress = (written * 100) / binSize; 
            }
            binFile.close();

            if (err != ESP_LOADER_SUCCESS) {
                globalSuccess = false;
                break;
            }
        }

        // Verification or Finish
        if (globalSuccess) {
            flashStatus = "Success";
            Serial.println("\nAll Files Flashed Successfully!");
        } else {
            Serial.println("\nFlash Job Failed!");
        }
        
        // Restore default baud
        Serial2.updateBaudRate(FLASHER_BAUD_RATE);
        esp_loader_reset_target(); 
        
        flashingActive = false;
    }
}
