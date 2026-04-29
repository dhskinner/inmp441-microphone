#pragma once
#include <Arduino.h>

#define I2S_WS 38
#define I2S_SD 40
#define I2S_SCK 39

#define RGB_LED_DATA_PIN 18
#define RGB_LED_LDO2_ENABLE_PIN 17
#define RGB_LED_BRIGHTNESS 64

#define I2S_PORT I2S_NUM_0
#define SAMPLE_RATE 16000
#define FFT_SAMPLES 512
#define FFT_BINS 64

using Spectrum = uint8_t[FFT_BINS];

// Speech-focused spectrogram band
#define SPECTRUM_MIN_HZ 60
#define SPECTRUM_MAX_HZ 15000

// Matlab server port
#define AUDIO_TCP_PORT 3333
#define AUDIO_STREAM_CHANNELS 1

// Web server
#define WS_PUSH_INTERVAL_MS 80
#define AUDIO_FRAME_HEADER_BYTES 16

// If your INMP441 is strapped to right channel and you see silence, change to RightChannel.
enum class AudioMode : uint8_t
{
    LeftChannel,
    RightChannel,
};
#define I2S_CHANNEL_FORMAT AudioMode::LeftChannel

#define WIFI_SSID "Walnut-Street"
#define WIFI_PASSWORD "password01"

#define AP_SSID "WhaleBoy"
#define AP_PASSWORD "password01"
