#ifndef CORE1_H
#define CORE1_H

#include <Arduino.h>
#include "pins.h"
#include "Data.h"
#include "sdplayer.h"
#include "BluetoothA2DPSink.h"

void core1_audio_task(void* param);

#endif