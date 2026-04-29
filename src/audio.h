#include "settings.h"
#include <driver/i2s.h>
#include <arduinoFFT.h>
#include <WiFi.h>

class AudioServer
{
protected:
  WiFiServer audioServer;
  WiFiClient audioClient;
  double vReal[FFT_SAMPLES];
  double vImag[FFT_SAMPLES];
  ArduinoFFT<double> fft;
  int32_t i2sSamples[FFT_SAMPLES];
  int16_t pcmSamples[FFT_SAMPLES];
  Spectrum spectrumBins;
  uint32_t audioFrameIndex = 0;

  void writeLe16(uint8_t *dst, const uint16_t value);
  void writeLe32(uint8_t *dst, const uint32_t value);
  void initI2S();
  void serviceAudioClient();
  bool readAudioBlock();
  void pushAudioFrame();
  bool hasAudioClient();
  void computeSpectrum();

public:
  AudioServer();
  void init();
  bool process();
  inline const int16_t *pcm() const
  {
    return pcmSamples;
  }
  inline Spectrum &spectrum()
  {
    return spectrumBins;
  }
};
