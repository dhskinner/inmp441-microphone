#include "rgbled.h"

void RgbLed::init(uint8_t powerpin, uint8_t datapin, uint8_t brightness)
{
    this->powerPin = powerpin;
    this->dataPin = datapin;
    this->brightness = brightness;

    pinMode(powerPin, OUTPUT);
    digitalWrite(powerPin, HIGH);
    delay(5);
}

void RgbLed::set(RgbMode rgbmode)
{
    this->mode = rgbmode;
    this->update();
}

void RgbLed::update()
{
    static RgbMode lastMode = RgbMode::BlinkRed;
    static unsigned long lastRainbowStepMs = 0;
    static unsigned long lastBlinkStepMs = 0;
    static bool blinkOn = false;
    static uint16_t wheel = 0;

    const unsigned long now = millis();

    if (mode != lastMode)
    {
        lastMode = mode;
        lastBlinkStepMs = now;
        lastRainbowStepMs = now;
        blinkOn = true;
    }

    switch (mode)
    {
    case RgbMode::BlinkRed:
        this->write(255, 0, 0);
        return;

    case RgbMode::BlinkBlue:
    {
        if (now - lastBlinkStepMs >= 350)
        {
            lastBlinkStepMs = now;
            blinkOn = !blinkOn;
        }
        this->write(0, 0, blinkOn ? 255 : 0);
        return;
    }

    case RgbMode::BlinkGreen:
    {
        if (now - lastBlinkStepMs >= 350)
        {
            lastBlinkStepMs = now;
            blinkOn = !blinkOn;
        }
        this->write(0, blinkOn ? 255 : 0, 0);
        return;
    }

    case RgbMode::Multicolor:
    {
        if (now - lastRainbowStepMs >= 18)
        {
            lastRainbowStepMs = now;
            wheel = static_cast<uint16_t>((wheel + 4) % 1536);
        }

        uint8_t r = 0;
        uint8_t g = 0;
        uint8_t b = 0;
        wheelToRgb(wheel, r, g, b);
        write(r, g, b);
        return;
    }
    }
}

void RgbLed::wheelToRgb(uint16_t wheel, uint8_t &r, uint8_t &g, uint8_t &b)
{
    wheel %= 1536;

    if (wheel < 256)
    {
        r = 255;
        g = static_cast<uint8_t>(wheel);
        b = 0;
        return;
    }
    if (wheel < 512)
    {
        r = static_cast<uint8_t>(511 - wheel);
        g = 255;
        b = 0;
        return;
    }
    if (wheel < 768)
    {
        r = 0;
        g = 255;
        b = static_cast<uint8_t>(wheel - 512);
        return;
    }
    if (wheel < 1024)
    {
        r = 0;
        g = static_cast<uint8_t>(1023 - wheel);
        b = 255;
        return;
    }
    if (wheel < 1280)
    {
        r = static_cast<uint8_t>(wheel - 1024);
        g = 0;
        b = 255;
        return;
    }

    r = 255;
    g = 0;
    b = static_cast<uint8_t>(1535 - wheel);
}

void RgbLed::write(uint8_t r, uint8_t g, uint8_t b)
{
    const uint16_t scaledR = (static_cast<uint16_t>(r) * brightness) / 255;
    const uint16_t scaledG = (static_cast<uint16_t>(g) * brightness) / 255;
    const uint16_t scaledB = (static_cast<uint16_t>(b) * brightness) / 255;
    rgbLedWrite(dataPin, scaledR, scaledG, scaledB);
}
