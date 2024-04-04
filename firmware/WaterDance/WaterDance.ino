#include <Arduino.h>
#include <GyverHub.h>
GyverHub hub;

#define AP_SSID "Alex"
#define AP_PASS "lolpass12345"
#define DAC_CH1 25
#define DAC_CH2 26
#define PIN_R 27
#define PIN_G 32
#define PIN_B 33

#include <driver/dac.h>

#include "phase.h"
#include "rgb.h"
#include "sine.h"
#include "soc/timer_group_reg.h"
#include "soc/timer_group_struct.h"

TaskHandle_t Task0;
void core0(void *p);
struct Data {
    bool state = 0;
    uint8_t freq = 15;
    uint8_t ampliX = 20;
    uint8_t ampliY = 20;
    uint8_t phaseY = 0;
    int8_t multY = 0;
    uint16_t expo = 500;
};
Data data;
gh::Timer genTmr, rgbTmr;
RGB rgb(PIN_R, PIN_G, PIN_B);

#define PHASE_AMOUNT 5
Phase phase[PHASE_AMOUNT];

// build
void build(gh::Builder &b) {
    {
        gh::Row r(b);
        b.Switch(&data.state).label("СОСТОЯНИЕ");
        if (b.click()) genTmr.restart();
    }

    b.Slider(&data.freq).label("ЧАСТОТА").range(10, 30, 1);
    if (b.click()) genTmr.startInterval(1000000ul / data.freq / 256);
    b.Slider(&data.ampliX).label("АМПЛИТУДА X").range(0, 128, 1);

    b.Slider(&data.multY).label("МНОЖИТЕЛЬ Y").range(0, 3, 1);
    b.Slider(&data.ampliY).label("АМПЛИТУДА Y").range(0, 128, 1);
    b.Slider(&data.phaseY).label("ФАЗА Y").range(0, 255, 1);

    b.Slider(&data.expo).label("ЯРКОСТЬ").range(100, 2000, 1);

    static int p = 0;
    b.Select(&p).label("СТРУЯ").text("СТРУЯ 1;СТРУЯ 2;СТРУЯ 3;СТРУЯ 4;СТРУЯ 5");
    if (b.click()) b.refresh();

    bool resetTrig = 0;
    bool resetHue = 0;
    {
        gh::Row r(b);
        b.Switch(&phase[p].state).label("СОСТОЯНИЕ");
        b.Switch(&phase[p].hue.autoMode).label("ЦВЕТ АВТО");
        b.Switch(&phase[p].trig.autoMode).label("ФАЗА АВТО");
    }

    b.Slider(&phase[p].hue.val).label("ЦВЕТ").range(0, 255, 1);
    if (b.click()) resetHue = 1;

    b.Slider(&phase[p].trig.val).label("ФАЗА").range(0, 255, 1);
    if (b.click()) resetTrig = 1;

    b.Slider(&phase[p].hue.step).label("ШАГ ЦВЕТА").range(-10, 10, 1);
    if (b.click()) resetHue = 1;

    b.Slider(&phase[p].trig.step).label("ШАГ ФАЗЫ").range(-20, 20, 1);
    if (b.click()) resetTrig = 1;

    if (resetHue) {
        for (int i = 0; i < PHASE_AMOUNT; i++) phase[i].hue.shift = 0;
    }
    if (resetTrig) {
        for (int i = 0; i < PHASE_AMOUNT; i++) phase[i].trig.shift = 0;
    }
}

void setup() {
    xTaskCreatePinnedToCore(core0, "Task0", 10000, NULL, 1, &Task0, 0);

    Serial.begin(115200);
    WiFi.mode(WIFI_STA);
    WiFi.begin(AP_SSID, AP_PASS);
    while (WiFi.status() != WL_CONNECTED) {
        delay(500);
        Serial.print(".");
    }
    Serial.println();
    Serial.println(WiFi.localIP());

    hub.config(F("MyDevices"), F("WaterDance"));
    hub.onBuild(build);
    hub.begin();

    rgbTmr.setSource(micros);
    genTmr.setSource(micros);
    genTmr.keepPhase(true);
    genTmr.startInterval(1000000ul / data.freq / 256);
    dac_output_enable(DAC_CHANNEL_1);
    dac_output_enable(DAC_CHANNEL_2);
}

uint8_t fade8(uint8_t val, uint8_t fade) {
    return ((uint16_t)val * (fade + 1)) >> 8;
}

void loop() {
    if (rgbTmr) rgb.off();

    if (data.state && genTmr.tick()) {
        static uint8_t count = 0;
        count++;
        dac_output_voltage(DAC_CHANNEL_1, fade8(sine_table[count], data.ampliX));
        dac_output_voltage(DAC_CHANNEL_2, fade8(sine_table[(uint8_t)((count * data.multY) + data.phaseY)], data.ampliY));

        for (int i = 0; i < PHASE_AMOUNT; i++) {
            if (phase[i].tick(count)) {
                rgb.writeHue(phase[i].hue.getVal());
                rgbTmr.startTimeout(data.expo);
            }
        }
    }
}

void core0(void *p) {
    for (;;) {
        // feed dog
        TIMERG0.wdt_wprotect = TIMG_WDT_WKEY_VALUE;  // write enable
        TIMERG0.wdt_feed = 1;                        // feed dog
        TIMERG0.wdt_wprotect = 0;                    // write protect

        hub.tick();
    }
}