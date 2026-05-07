#include "pins.h"
#include "sdplayer.h"
#include "BluetoothA2DPSink.h"
#include <SD.h>
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

const char* random_sounds[] = {
    "/back_dock_nearby.wav", "/binout_error10.wav", "/bin_in.wav", "/bin_out.wav",
    "/bl_recovery_updatefailed.wav", "/charging.wav", "/clean_bin.wav",
    "/error11.wav", "/error12.wav", "/error13.wav", "/error7.wav", "/error_internal.wav",
    "/findme.wav", "/goto.wav", "/goto_complete.wav", "/home.wav", "/no_power.wav",
    "/pause.wav", "/power_off.wav", "/power_off_rejected.wav", "/power_resume_clean.wav",
    "/relocate_failed.wav", "/remote.wav", "/remote_complete.wav", "/resume_zone.wav",
    "/return_no.wav", "/return_yes.wav", "/spot.wav", "/start.wav", "/stop.wav",
    "/stop_zone.wav", "/sysupd_complete.wav", "/sysupd_failed.wav", "/sysupd_notready.wav",
    "/sysupd_start.wav"
};

BluetoothA2DPSink a2dp_sink;

bool sd_play_with_bt_pause(const char* fname, uint8_t vol) {
    is_sd_playing = true;
    angry_tmr.start(2750);

    // ПОЛНОЕ ОТКЛЮЧЕНИЕ A2DP ОТ I2S
    if (data.state) {
        a2dp_sink.pause();
        delay(50);

        // 1-й способ — отключить выход
        a2dp_sink.set_output_active(false);
        delay(50);

        // 2-й способ — остановить A2DP
        // (если твой проект предусматривает повторный start)
        a2dp_sink.stop();
        delay(50);

        // Тут Bluetooth уже НЕ пишет в I2S
    }

    // SD-проигрывание использует I2S целиком, уже без конкуренции
    bool res = sdplayer.playAndWait(fname, vol);

    // ВОССТАНОВЛЕНИЕ A2DP ПОСЛЕ СД
    if (data.state) {
        // Включаем A2DP debería быть перед start/play
        a2dp_sink.set_output_active(true);
        delay(50);

        // Перезапускаем A2DP sink
        a2dp_sink.start("Bender Radio");
        delay(50);

        // Установка громкости (если не менялась в core1_audio_task)
        a2dp_sink.set_volume(data.vol * 6);
        a2dp_sink.play();
    }

    is_sd_playing = false;
    return res;
}

void play_random_sound() {
    if (!data.state) return;
    uint8_t count = sizeof(random_sounds) / sizeof(random_sounds[0]);
    uint8_t idx = random(0, count);
    sd_play_with_bt_pause(random_sounds[idx], data.vol);
}

void change_state() {
    mtrx.clear();

    if (data.state) {
        // ВКЛЮЧЕНИЕ
        upd_bright();
        draw_eye(0); draw_eye(1);
        draw_eyeb(0, 2, 2, 4);
        draw_eyeb(1, 2, 2, 4);
        mtrx.update();
        sd_play_with_bt_pause("/power_on.wav", data.vol);
        a2dp_sink.set_volume(data.vol * 6);
        a2dp_sink.start("Bender Radio");
    } else {
        // ВЫКЛЮЧЕНИЕ → ПОЛНОЕ ОТКЛЮЧЕНИЕ Bluetooth

        mtrx.setBright(1);
        draw_eye(0);
        draw_eye(1);
        mtrx.rect(ANALYZ_WIDTH, 0, ANALYZ_WIDTH + 16 - 1, 3, GFX_CLEAR);
        draw_eyeb(0, 3, 5);
        draw_eyeb(1, 3, 5);
        mtrx.update();

        // 1. ОСТАНОВИТЬ ВОСПРОИЗВЕДЕНИЕ И I2S
        a2dp_sink.stop();
        delay(50);
        a2dp_sink.set_output_active(false);
        delay(50);

        // 2. ПОЛНОЕ ОТКЛЮЧЕНИЕ/РАЗРЫВ БЛЮТУЗ‑СОЕДИНЕНИЯ
        // У pschatzmann/ESP32-A2DP обычно:
        // a2dp_sink.disconnect();   // если есть в твоей версии
        // или
        // a2dp_sink.end(false);     // end без отпуска памяти, но остановка стека

        // Если у тебя есть метод disconnect:
        if (a2dp_sink.is_connected()) {
            a2dp_sink.stop();
            delay(50);
            a2dp_sink.disconnect();
            delay(50);
        }

        // 3. SD-проигрывание уже без Bluetooth вообще
        sd_play_with_bt_pause("/power_off.wav", data.vol);

        // ❗ 4. НЕ ВКЛЮЧАЕМ ОБРАТНО!
        // a2dp_sink.start("Bender Radio");   - НЕ вызываем!
        // Он включится только в ветке `if (data.state)`
    }
}

bool initSD() {
    spiSD.begin(SD_SCK, SD_MISO, SD_MOSI, SD_CS);
    return SD.begin(SD_CS, spiSD, 4000000, "/sd", 5);
}

void core1_audio_task(void* param) {
    Serial.println("Core1: START");

    if (!initSD() || !sdplayer.begin()) {
        Serial.println("Core1: SD FAILED!");
        while (1) delay(1000);
    }
    Serial.println("Core1: Ready");

    // Конфигурация I2S для A2DP
    i2s_pin_config_t pin_config = {
        .bck_io_num   = I2S_BCLK,
        .ws_io_num    = I2S_LRC,
        .data_out_num = I2S_DOUT,
        .data_in_num  = I2S_PIN_NO_CHANGE
    };
    a2dp_sink.set_pin_config(pin_config);

    static uint8_t last_vol           = 255;
    static bool    last_random_flag   = false;
    static bool    first_run          = 1;
    static bool    last_state         = 0;

    for(;;) {

        // ВОЛУМ — только если НЕ играет SD-звук
        if (!is_sd_playing && data.vol != last_vol) {
            Serial.printf("VOL: %d\n", data.vol);
            a2dp_sink.set_volume(data.vol * 6);
            last_vol = data.vol;
        }

        // РАНДОМ-СД ЗВУК (через sd_play_with_bt_pause)
        if (data.play_random_flag && !last_random_flag && data.state) {
            Serial.println("Core1: PLAY RANDOM!");
            play_random_sound();          // внутри sd_play_with_bt_pause
            data.play_random_flag = false;
        }
        last_random_flag = data.play_random_flag;


        // СМЕНА СОСТОЯНИЯ (power_on.wav / power_off.wav)
        if (first_run || data.state != last_state) {
            Serial.printf("Core1: change_state() data.state = %d\n", data.state);
            change_state();               // там тоже sd_play_with_bt_pause
            last_state = data.state;
            first_run = 0;
        }


        // WDT
        TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;
        TIMERG0.wdt_feed = 1;
        TIMERG0.wdt_wprotect = 0;

        vTaskDelay(10 / portTICK_PERIOD_MS);
    }
}