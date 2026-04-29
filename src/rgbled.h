#pragma once

#include <Arduino.h>

enum class RgbMode
{
    BlinkRed,
    BlinkGreen,
    BlinkBlue,
    Multicolor,
};

class RgbLed
{
protected:
    uint8_t powerPin = 255;
    uint8_t dataPin = 255;
    uint8_t brightness = 128;
    RgbMode mode = RgbMode::BlinkRed;

    void wheelToRgb(uint16_t wheel, uint8_t &r, uint8_t &g, uint8_t &b);
    void write(uint8_t r, uint8_t g, uint8_t b);

public:
    void init(uint8_t powerpin, uint8_t datapin, uint8_t brightness);
    void update();
    void set(RgbMode rgbmode);
};
