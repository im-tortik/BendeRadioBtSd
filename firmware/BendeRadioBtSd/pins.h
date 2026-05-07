#ifndef PINS_H
#define PINS_H

#include <Arduino.h>
#include <stdint.h>
#include "Data.h"
#include <GyverMAX7219.h>
#include <EncButton.h>
#include <VolAnalyzer.h>
#include <EEManager.h>
#include "tmr.h"

extern SPIClass spiSD;
extern Tmr square_tmr;
extern Tmr angry_tmr;
extern bool is_sd_playing;

// === ПИНЫ МАТРИЦЫ ===
#define MTRX_CS  22
#define MTRX_DAT 23
#define MTRX_CLK 21

#define ENC_S1   19
#define ENC_S2   18
#define ENC_BTN  5

#define ANALYZ_PIN 34
#define ANALYZ_WIDTH (3 * 8)

// === ПИНЫ SD + I2S ===
#define SD_CS   4
#define SD_SCK  16
#define SD_MISO 15
#define SD_MOSI 2

#define I2S_BCLK 27
#define I2S_LRC  26
#define I2S_DOUT 25

// Объекты Core 0
extern MAX7219<5, 1, MTRX_CS, MTRX_DAT, MTRX_CLK> mtrx;
extern EncButton eb;
extern VolAnalyzer sound;
extern EEManager memory;

// Функции
void core0_matrix_task(void* param);
void core1_audio_task(void* param);

void change_state();  // ← Для Core1!
void upd_bright();
void draw_eye(uint8_t i);
void draw_eyeb(uint8_t i, int x, int y, int w = 2);
void play_random_sound();
extern const char* random_sounds[];

#endif