#pragma once

#include <Arduino.h>
#include <WiFi.h>
#include "settings.h"

namespace RgbLed
{

    enum class Mode
    {
        Disconnected,
        AccessPointIdle,
        WifiConnectedIdle,
        ClientConnected
    };

    inline void wheelToRgb(uint16_t wheel, uint8_t &r, uint8_t &g, uint8_t &b)
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

    inline void write(uint8_t r, uint8_t g, uint8_t b)
    {
        const uint16_t scaledR = (static_cast<uint16_t>(r) * RGB_LED_BRIGHTNESS) / 255;
        const uint16_t scaledG = (static_cast<uint16_t>(g) * RGB_LED_BRIGHTNESS) / 255;
        const uint16_t scaledB = (static_cast<uint16_t>(b) * RGB_LED_BRIGHTNESS) / 255;
        rgbLedWrite(RGB_LED_PIN, scaledR, scaledG, scaledB);
    }

    inline void init()
    {
        pinMode(RGB_LED_LDO2_ENABLE_PIN, OUTPUT);
        digitalWrite(RGB_LED_LDO2_ENABLE_PIN, HIGH);
        delay(5);
    }

    inline Mode selectMode(bool hasWsClient)
    {
        if (hasWsClient)
        {
            return Mode::ClientConnected;
        }

        if (WiFi.status() == WL_CONNECTED)
        {
            return Mode::WifiConnectedIdle;
        }

        const wifi_mode_t mode = WiFi.getMode();
        if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)
        {
            return Mode::AccessPointIdle;
        }

        return Mode::Disconnected;
    }

    inline void update(Mode mode)
    {
        static Mode lastMode = Mode::Disconnected;
        static unsigned long lastBlinkStepMs = 0;
        static bool blinkOn = false;
        static unsigned long lastRainbowStepMs = 0;
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
        case Mode::Disconnected:
            write(255, 0, 0);
            return;

        case Mode::AccessPointIdle:
        {
            if (now - lastBlinkStepMs >= 350)
            {
                lastBlinkStepMs = now;
                blinkOn = !blinkOn;
            }
            write(blinkOn ? 0 : 0, blinkOn ? 0 : 0, blinkOn ? 255 : 0);
            return;
        }

        case Mode::WifiConnectedIdle:
        {
            if (now - lastBlinkStepMs >= 350)
            {
                lastBlinkStepMs = now;
                blinkOn = !blinkOn;
            }
            write(blinkOn ? 0 : 0, blinkOn ? 255 : 0, blinkOn ? 0 : 0);
            return;
        }

        case Mode::ClientConnected:
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

} // namespace RgbLed
