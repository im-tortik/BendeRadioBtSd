#include "pins.h"
#include "tmr.h"

// Инициализация объектов Core 0
MAX7219<5, 1, MTRX_CS, MTRX_DAT, MTRX_CLK> mtrx;
EncButton eb(ENC_S1, ENC_S2, ENC_BTN);
VolAnalyzer sound(ANALYZ_PIN);
EEManager memory(data);
Tmr square_tmr, eye_tmr(150), matrix_tmr(1000), angry_tmr(800);
bool is_sd_playing = false; 

// Noise + матрица (без изменений)
inline uint8_t my_inoise8(uint16_t x, uint16_t y) {
    float fx = x * 0.01f; float fy = y * 0.01f;
    float v = sin(fx) * sin(fy); v = (v + 1.0f) * 0.5f;
    return (uint8_t)(v * 255.0f);
}

void upd_bright() {
    uint8_t m = data.bright_mouth, e = data.bright_eyes;
    uint8_t br[] = {m, m, m, e, e};
    mtrx.setBright(br);
}

void print_val(char c, uint8_t v) {
    mtrx.rect(0, 0, ANALYZ_WIDTH - 1, 7, GFX_CLEAR);
    mtrx.setCursor(8*0+2,1); mtrx.print(c);
    mtrx.setCursor(8*1+2,1); mtrx.print(v/10);
    mtrx.setCursor(8*2+2,1); mtrx.print(v%10);
    mtrx.update();
}

void draw_eye(uint8_t i) {
    uint8_t x = ANALYZ_WIDTH + i*8;
    mtrx.rect(1+x,1,6+x,6,GFX_FILL);
    mtrx.lineV(0+x,2,5); mtrx.lineV(7+x,2,5);
    mtrx.lineH(0,2+x,5+x); mtrx.lineH(7,2+x,5+x);
}

void draw_eyeb(uint8_t i, int x, int y, int w) {
    x += ANALYZ_WIDTH + i * 8;
    mtrx.rect(x, y, x + w - 1, y + w - 1, GFX_CLEAR);
}

void analyz0(uint8_t vol) {
    static uint16_t offs; offs += 20 * vol / 100;
    for (uint8_t i = 0; i < ANALYZ_WIDTH; i++) {
        int16_t val = my_inoise8(i*50, offs);
        val -= 128; val = val * vol / 100; val += 128;
        val = map(val, 45, 255-45, 0, 7);
        mtrx.dot(i, val);
    }
}

void core0_matrix_task(void* param) {
    Serial.println("Core0: START");

    // Инициализируем EEPROM
    EEPROM.begin(memory.blockSize());
    memory.begin(0, 'b'); 
    
    // ✅ ИНИЦИАЛИЗАЦИЯ VolAnalyzer
    sound.setAmpliDt(300);
    sound.setTrsh(data.trsh);
    sound.setPulseMin(40);
    sound.setPulseMax(80);
    
    // ✅ ТАЙМЕРЫ
    square_tmr.timerMode(1);
    matrix_tmr.timerMode(1);
    angry_tmr.timerMode(1);
    
    // ✅ МАТРИЦА ПЕРЕД ВСЕМ!
    mtrx.begin();
    upd_bright();
    mtrx.clear();
    mtrx.update();
    Serial.println("Core0: Matrix ready");

    bool pulse = 0;
    
    for(;;) {
        square_tmr.tick();
        matrix_tmr.tick();
        angry_tmr.tick();
        memory.tick();

        // ✅ АНИМАЦИЯ ГЛАЗ (data.state проверка!)
        if (data.state && !square_tmr.state() && eye_tmr) {
            draw_eye(0); draw_eye(1);
            
            if (angry_tmr.state()) {
                draw_eyeb(0, 3, 3);
                draw_eyeb(1, 3, 3);
                mtrx.lineH(0, ANALYZ_WIDTH, ANALYZ_WIDTH + 16 - 1, GFX_CLEAR);
                mtrx.lineH(1, ANALYZ_WIDTH + 5, ANALYZ_WIDTH + 5 + 6 - 1, GFX_CLEAR);
                mtrx.lineH(2, ANALYZ_WIDTH + 6, ANALYZ_WIDTH + 6 + 4 - 1, GFX_CLEAR);
                mtrx.lineH(3, ANALYZ_WIDTH + 7, ANALYZ_WIDTH + 7 + 2 - 1, GFX_CLEAR);
            } else if (eb.pressing()) {
                draw_eyeb(0,4,3,3); draw_eyeb(1,1,3,3);
            } else {
                static uint16_t pos; pos += 15;
                uint8_t x = my_inoise8(pos,0), y = my_inoise8(pos+UINT16_MAX/4,0);
                x = constrain(x,40,255-40); y = constrain(y,40,255-40);
                x = map(x,40,255-40,2,5); y = map(y,40,255-40,2,5);
                if (pulse) {
                    pulse = 0; int8_t sx = random(-1,1), sy = random(-1,1);
                    draw_eyeb(0,x+sx,y+sy,3); draw_eyeb(1,x+sx,y+sy,3);
                } else {
                    draw_eyeb(0,x,y); draw_eyeb(1,x,y);
                }
            }
            mtrx.update();
        }

        // ✅ АНАЛИЗ ЗВУКА
        if (sound.tick() && data.state && !matrix_tmr.state()) {
            if (sound.pulse()) pulse = 1;
            mtrx.rect(0,0,ANALYZ_WIDTH-1,7,GFX_CLEAR);
            if (data.mode == 0) analyz0(sound.getVol());
            mtrx.update();
        }

        // ✅ ЭНКОДЕР (полная логика оригинала!)
        if (eb.tick()) {
            if (eb.turn()) {
                if (eb.pressing()) {
                    // 1 клик + поворот = рот
                    if (eb.getClicks() == 1) {
                        data.bright_mouth += eb.dir();
                        data.bright_mouth = constrain(data.bright_mouth,0,16);
                        upd_bright(); print_val('m',data.bright_mouth); matrix_tmr.start();
                    }
                    // 2 клика + поворот = глаза
                    else if (eb.getClicks() == 2) {
                        data.bright_eyes += eb.dir();
                        data.bright_eyes = constrain(data.bright_eyes,0,16);
                        upd_bright(); print_val('e',data.bright_eyes); matrix_tmr.start();
                    }
                } else {
                    // Поворот без кнопки = ГРОМКОСТЬ
                    if (data.state) {
                        angry_tmr.start();
                        data.vol += eb.dir();
                        data.vol = constrain(data.vol, 0, 21);
                        print_val('v', data.vol);
                        matrix_tmr.start();
                        memory.update();
                    }
                }
            }
            
            if (eb.hasClicks()) {
                switch(eb.getClicks()) {
                    case 1:
                        data.state = !data.state;
                        if (data.state) {
                            square_tmr.start(600);
                        }
                        break;
                    case 2:
                        data.play_random_flag = true;
                        break;
                    case 3:
                        data.trsh = sound.getMax()*2/3;
                        sound.setTrsh(data.trsh);
                        break;
                }
            }
            memory.update();
        }
        
        vTaskDelay(1);
    }
}