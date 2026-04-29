#include "audio.h"

AudioServer::AudioServer() : audioServer(AUDIO_TCP_PORT), fft(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE)
{
}

void AudioServer::writeLe16(uint8_t *dst, const uint16_t value)
{
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
}

void AudioServer::writeLe32(uint8_t *dst, const uint32_t value)
{
  dst[0] = static_cast<uint8_t>(value & 0xFF);
  dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
  dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
}

bool AudioServer::hasAudioClient()
{
  return static_cast<bool>(audioClient) && audioClient.connected();
}

void AudioServer::init()
{
  initI2S();

  audioServer.begin();
  audioServer.setNoDelay(true);
  Serial.printf("Audio TCP stream ready on port %u\n", static_cast<unsigned int>(AUDIO_TCP_PORT));
}

bool AudioServer::process()
{
  serviceAudioClient();

  if (readAudioBlock())
  {
    computeSpectrum();
    pushAudioFrame();
    return true;
  }

  return false;
}

void AudioServer::serviceAudioClient()
{
  if (hasAudioClient())
  {
    return;
  }

  if (audioClient)
  {
    audioClient.stop();
  }

  WiFiClient incoming = audioServer.accept();
  if (!incoming)
  {
    return;
  }

  incoming.setNoDelay(true);
  audioClient = incoming;
  audioFrameIndex = 0;
  Serial.printf("Audio client connected from %u.%u.%u.%u:%u\n",
                audioClient.remoteIP()[0],
                audioClient.remoteIP()[1],
                audioClient.remoteIP()[2],
                audioClient.remoteIP()[3],
                static_cast<unsigned int>(audioClient.remotePort()));
}

void AudioServer::pushAudioFrame()
{
  if (!hasAudioClient())
  {
    return;
  }

  uint8_t header[AUDIO_FRAME_HEADER_BYTES];
  header[0] = 'A';
  header[1] = 'U';
  header[2] = 'D';
  header[3] = '0';
  writeLe32(&header[4], audioFrameIndex);
  writeLe16(&header[8], static_cast<uint16_t>(SAMPLE_RATE));
  writeLe16(&header[10], static_cast<uint16_t>(FFT_SAMPLES));
  writeLe16(&header[12], static_cast<uint16_t>(AUDIO_STREAM_CHANNELS));
  writeLe16(&header[14], 1); // format: signed int16 PCM little-endian

  const size_t payloadBytes = sizeof(pcmSamples);
  const size_t headerWritten = audioClient.write(header, sizeof(header));
  const size_t payloadWritten = audioClient.write(
      reinterpret_cast<const uint8_t *>(pcmSamples),
      payloadBytes);

  if (headerWritten != sizeof(header) || payloadWritten != payloadBytes)
  {
    Serial.println("Audio client write failed; disconnecting");
    audioClient.stop();
    return;
  }

  audioFrameIndex++;
}

void AudioServer::initI2S()
{
  i2s_config_t i2s_config = {
      .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
      .sample_rate = SAMPLE_RATE,
      .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
      .channel_format = (static_cast<uint8_t>(I2S_CHANNEL_FORMAT) == static_cast<uint8_t>(AudioMode::LeftChannel)) ? I2S_CHANNEL_FMT_ONLY_LEFT : I2S_CHANNEL_FMT_ONLY_RIGHT,
      .communication_format = I2S_COMM_FORMAT_STAND_I2S,
      .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
      .dma_buf_count = 8,
      .dma_buf_len = 256,
      .use_apll = false,
      .tx_desc_auto_clear = false,
      .fixed_mclk = 0,
  };

  const esp_err_t installResult = i2s_driver_install(I2S_PORT, &i2s_config, 0, nullptr);
  if (installResult != ESP_OK)
  {
    Serial.printf("I2S driver install failed: %d\n", installResult);
    while (true)
    {
      delay(1000);
    }
  }

  i2s_pin_config_t i2s_pin_config = {
      .bck_io_num = I2S_SCK,
      .ws_io_num = I2S_WS,
      .data_out_num = I2S_PIN_NO_CHANGE,
      .data_in_num = I2S_SD,
  };

  const esp_err_t pinResult = i2s_set_pin(I2S_PORT, &i2s_pin_config);
  if (pinResult != ESP_OK)
  {
    Serial.printf("I2S set pin failed: %d\n", pinResult);
    while (true)
    {
      delay(1000);
    }
  }

  i2s_zero_dma_buffer(I2S_PORT);
  Serial.println(F("I2S initialized successfully"));
}

bool AudioServer::readAudioBlock()
{
  size_t bytesRead = 0;
  const esp_err_t err = i2s_read(
      I2S_PORT,
      static_cast<void *>(i2sSamples),
      sizeof(i2sSamples),
      &bytesRead,
      portMAX_DELAY);

  if (err != ESP_OK || bytesRead != sizeof(i2sSamples))
  {
    return false;
  }

  double mean = 0.0;
  for (size_t i = 0; i < FFT_SAMPLES; i++)
  {
    const int32_t s24 = i2sSamples[i] >> 8;
    int32_t s16 = s24 >> 8;
    if (s16 > 32767)
    {
      s16 = 32767;
    }
    else if (s16 < -32768)
    {
      s16 = -32768;
    }
    pcmSamples[i] = static_cast<int16_t>(s16);

    const double sample = static_cast<double>(s24) / 8388608.0;
    vReal[i] = sample;
    vImag[i] = 0.0;
    mean += sample;
  }

  mean /= static_cast<double>(FFT_SAMPLES);
  for (size_t i = 0; i < FFT_SAMPLES; i++)
  {
    vReal[i] -= mean;
  }

  return true;
}

void AudioServer::computeSpectrum()
{
  fft.windowing(FFTWindow::Hamming, FFTDirection::Forward);
  fft.compute(FFTDirection::Forward);
  fft.complexToMagnitude();

  const double binWidthHz = static_cast<double>(SAMPLE_RATE) / static_cast<double>(FFT_SAMPLES);
  const size_t nyquistBin = (FFT_SAMPLES / 2) - 1;

  double minHz = static_cast<double>(SPECTRUM_MIN_HZ);
  double maxHz = static_cast<double>(SPECTRUM_MAX_HZ);

  if (minHz < binWidthHz)
  {
    minHz = binWidthHz;
  }

  const double nyquistHz = static_cast<double>(SAMPLE_RATE) * 0.5;
  if (maxHz > nyquistHz)
  {
    maxHz = nyquistHz;
  }

  if (maxHz <= minHz)
  {
    maxHz = minHz + binWidthHz;
  }

  const double logMin = log10(minHz);
  const double logMax = log10(maxHz);
  const double logSpan = logMax - logMin;

  for (size_t i = 0; i < FFT_BINS; i++)
  {
    const double t0 = static_cast<double>(i) / static_cast<double>(FFT_BINS);
    const double t1 = static_cast<double>(i + 1) / static_cast<double>(FFT_BINS);

    const double f0 = pow(10.0, logMin + t0 * logSpan);
    const double f1 = pow(10.0, logMin + t1 * logSpan);

    size_t binStart = static_cast<size_t>(floor(f0 / binWidthHz));
    size_t binEnd = static_cast<size_t>(ceil(f1 / binWidthHz));

    if (binStart < 1)
    {
      binStart = 1;
    }
    if (binStart > nyquistBin)
    {
      binStart = nyquistBin;
    }
    if (binEnd < binStart)
    {
      binEnd = binStart;
    }
    if (binEnd > nyquistBin)
    {
      binEnd = nyquistBin;
    }

    double magAccum = 0.0;
    for (size_t bin = binStart; bin <= binEnd; bin++)
    {
      magAccum += vReal[bin];
    }

    const double mag = magAccum / static_cast<double>(binEnd - binStart + 1);
    const double db = 20.0 * log10(mag + 1e-12);
    const double normalized = (db + 90.0) / 70.0;
    const int level = static_cast<int>(normalized * 255.0);
    spectrumBins[i] = static_cast<uint8_t>(constrain(level, 0, 255));
  }
}
