#include <Arduino.h>
#include "pins.h"

SPIClass spiSD(VSPI);

TaskHandle_t core0_handle = NULL;
TaskHandle_t core1_handle = NULL;

void setup() {
    Serial.begin(115200);
    Serial.println("BendeRadio Dual Core START!");
    
    xTaskCreatePinnedToCore(core0_matrix_task, "Core0_Matrix", 10000, NULL, 1, &core0_handle, 0);
    xTaskCreatePinnedToCore(core1_audio_task, "Core1_Audio",  15000, NULL, 2, &core1_handle, 1);
    
    Serial.println("Handles OK. Heap: " + String(ESP.getFreeHeap()));
}

void loop() { vTaskDelete(NULL); }
