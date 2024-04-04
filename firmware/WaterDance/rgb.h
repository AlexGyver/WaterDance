#pragma once
#include <Arduino.h>

class RGB {
   public:
    RGB(uint8_t r, uint8_t g, uint8_t b, uint8_t startCh = 0, uint16_t freq = 8192) : startCh(startCh) {
        ledcSetup(startCh + 0, freq, 8);
        ledcAttachPin(r, startCh + 0);

        ledcSetup(startCh + 1, freq, 8);
        ledcAttachPin(g, startCh + 1);

        ledcSetup(startCh + 2, freq, 8);
        ledcAttachPin(b, startCh + 2);
        off();
    }

    void write(uint16_t r, uint16_t g, uint16_t b) {
        ledcWrite(startCh + 0, r);
        ledcWrite(startCh + 1, g);
        ledcWrite(startCh + 2, b);
    }

    void writeHue(uint8_t color) {
        uint8_t r, g, b;
        uint8_t shift;
        if (color > 170) {
            shift = (color - 170) * 3;
            r = shift;
            g = 0;
            b = 255 - shift;
        } else if (color > 85) {
            shift = (color - 85) * 3;
            r = 0;
            g = 255 - shift;
            b = shift;
        } else {
            shift = color * 3;
            r = 255 - shift;
            g = shift;
            b = 0;
        }
        write(r, g, b);
    }

    void off() {
        write(0, 0, 0);
    }

   private:
    uint8_t startCh;
};