#ifndef CORE0_H
#define CORE0_H

#include <Arduino.h>
#include "pins.h"
#include "Data.h"
#include <GyverMAX7219.h>
#include <EncButton.h>
#include <VolAnalyzer.h>
#include <EEManager.h>

// Объекты
extern MAX7219<5, 1, MTRX_CS, MTRX_DAT, MTRX_CLK> mtrx;
extern EncButton eb;
extern VolAnalyzer sound;
extern EEManager memory;

// Функции
void core0_matrix_task(void* param);

#endif