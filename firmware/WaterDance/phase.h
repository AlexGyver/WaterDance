#pragma once
#include <Arduino.h>

struct Shifter {
    uint8_t val = 0;
    bool flag = 0;
    bool autoMode = 0;
    int8_t step = 0;
    uint8_t shift = 0;

    uint8_t getVal() {
        return autoMode ? (val + shift) : val;
    }

    bool tick(uint8_t count) {
        if (!flag && count == getVal()) {
            flag = 1;
            shift += step;
            return 1;
        }
        if (count == (uint8_t)(getVal() + 120)) flag = 0;
        return 0;
    }
};

class Phase {
   public:
    bool tick(uint8_t count) {
        if (!state) return 0;
        hue.tick(count);
        if (trig.tick(count)) return 1;
        return 0;
    }

    bool state = 0;
    Shifter hue;
    Shifter trig;
};