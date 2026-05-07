#ifndef DATA_H
#define DATA_H

#include <stdint.h>  // ← ДОБАВИТЬ для uint8_t

struct Data {
    bool state = false;
    uint8_t vol = 10;
    uint8_t mode = 0;
    uint8_t trsh = 50;
    uint8_t bright_mouth = 8;
    uint8_t bright_eyes = 12;

    bool play_random_flag = false;
    bool is_sd_playing = false;
};

extern Data data;

#endif