#pragma once

#include <Arduino.h>
#include <driver/i2s.h>

#define I2S_WS 38
#define I2S_SD 40
#define I2S_SCK 39

#define RGB_LED_PIN 18
#define RGB_LED_LDO2_ENABLE_PIN 17
#define RGB_LED_BRIGHTNESS 64

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define FFT_SAMPLES 512
#define FFT_BINS 64

// Speech-focused spectrogram band. INMP441 low-end rolloff is around this region.
#define SPECTRUM_MIN_HZ 80
#define SPECTRUM_MAX_HZ 8000

#define AUDIO_TCP_PORT 3333
#define AUDIO_STREAM_CHANNELS 1

// If your INMP441 is strapped to right channel and you see silence, change to I2S_CHANNEL_FMT_ONLY_RIGHT.
#define I2S_CHANNEL_FORMAT I2S_CHANNEL_FMT_ONLY_LEFT

#define WIFI_SSID "Walnut-Street"
#define WIFI_PASSWORD "password01"

#define AP_SSID "WhaleBoy"
#define AP_PASSWORD "password01"

inline const i2s_pin_config_t pin_config = {
    .bck_io_num = I2S_SCK,
    .ws_io_num = I2S_WS,
    .data_out_num = I2S_PIN_NO_CHANGE,
    .data_in_num = I2S_SD};
