#pragma once
#include <Arduino.h>
#include <FS.h>
#include <SD.h>
#include "pins.h"

class SDPlayer {
public:
    SDPlayer();
    bool begin();
    bool play(const char* fname, uint8_t vol);
    bool playAndWait(const char* fname, uint8_t vol);
    void stop();  // ✅ НОВОЕ!

private:
    bool initI2S();
    bool playWAV(File& f, uint8_t vol);
    void deinitI2S();  // ✅ НОВОЕ!
};

extern SDPlayer sdplayer;