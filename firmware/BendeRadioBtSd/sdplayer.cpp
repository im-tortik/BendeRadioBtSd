#include "sdplayer.h"
#include "driver/i2s.h"
#include "pins.h"
#include <SPI.h>
#include <SD.h>

//SPIClass spiSD(HSPI);
static bool i2s_enabled = false;
#define I2S_NUM I2S_NUM_0

SDPlayer::SDPlayer() {
    // пустой или инициализация
}

SDPlayer sdplayer;

bool SDPlayer::begin() {
    return initI2S();
}

void SDPlayer::stop() {
    deinitI2S();
}

void SDPlayer::deinitI2S() {
    if (i2s_enabled) {
        i2s_stop(I2S_NUM);
        i2s_driver_uninstall(I2S_NUM);
        i2s_enabled = false;
    }
}

bool SDPlayer::initI2S() {
    if (i2s_enabled) return true;  // уже инициализирован
    
    i2s_config_t i2s_config = {
        .mode = (i2s_mode_t)(I2S_MODE_MASTER | I2S_MODE_TX),
        .sample_rate = 44100,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_16BIT,
        .channel_format = I2S_CHANNEL_FMT_RIGHT_LEFT,
        .communication_format = I2S_COMM_FORMAT_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 6,
        .dma_buf_len = 128,
        .use_apll = false,
        .tx_desc_auto_clear = true,
        .fixed_mclk = 0
    };

    i2s_pin_config_t pin_config = {
        .bck_io_num = I2S_BCLK,
        .ws_io_num = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num = I2S_PIN_NO_CHANGE
    };

    esp_err_t err = i2s_driver_install(I2S_NUM, &i2s_config, 0, nullptr);
    if (err != ESP_OK) return false;

    err = i2s_set_pin(I2S_NUM, &pin_config);
    if (err != ESP_OK) return false;

    i2s_start(I2S_NUM);
    i2s_enabled = true;
    return true;
}

bool SDPlayer::playWAV(File& f, uint8_t vol) {
    if (f.read() != 'R' || f.read() != 'I' || f.read() != 'F' || f.read() != 'F') {
        return false;
    }
    for (int i = 0; i < 40; i++) f.read();

    if (!f) return false;

    size_t buf_size = 512;
    uint8_t* buf = new uint8_t[buf_size];
    if (!buf) return false;

    size_t rlen, bytes_written;
    uint8_t gain = vol ? (uint8_t)((vol * 255ULL) / 21ULL) : 0;

    while ((rlen = f.read(buf, buf_size)) > 0) {
        for (size_t i = 0; i < rlen; i += 2) {
            int16_t sample = (int16_t)(buf[i + 1] << 8) | buf[i];
            sample = (sample * gain) / 255;
            sample = constrain(sample, -32768, 32767);
            buf[i] = (uint8_t)(sample & 0xFF);
            buf[i + 1] = (uint8_t)(sample >> 8);
        }
        i2s_write(I2S_NUM, buf, rlen, &bytes_written, portMAX_DELAY);
    }

    delete[] buf;
    return true;
}

bool SDPlayer::play(const char* fname, uint8_t vol) {
    if (!i2s_enabled) initI2S();

    File f = SD.open(fname);
    if (!f) return false;

    bool res = playWAV(f, vol);
    f.close();
    return res;
}

bool SDPlayer::playAndWait(const char* fname, uint8_t vol) {
    bool res = play(fname, vol);
    vTaskDelay(pdMS_TO_TICKS(100));
    return res;
}