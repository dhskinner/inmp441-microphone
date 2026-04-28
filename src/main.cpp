#include "settings.h"
#include "rgbled.h"
#include "html.h"
#include <arduinoFFT.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <WiFi.h>
#include <cmath>

namespace
{

  constexpr unsigned long WS_PUSH_INTERVAL_MS = 80;
  constexpr size_t AUDIO_FRAME_HEADER_BYTES = 16;

  AsyncWebServer server(80);
  AsyncWebSocket ws("/ws");
  WiFiServer audioServer(AUDIO_TCP_PORT);
  WiFiClient audioClient;

  double vReal[FFT_SAMPLES];
  double vImag[FFT_SAMPLES];
  ArduinoFFT<double> FFT(vReal, vImag, FFT_SAMPLES, SAMPLE_RATE);

  int32_t i2sSamples[FFT_SAMPLES];
  int16_t pcmSamples[FFT_SAMPLES];
  uint8_t spectrum[FFT_BINS];
  uint32_t audioFrameIndex = 0;

  void writeLe16(uint8_t *dst, const uint16_t value)
  {
    dst[0] = static_cast<uint8_t>(value & 0xFF);
    dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
  }

  void writeLe32(uint8_t *dst, const uint32_t value)
  {
    dst[0] = static_cast<uint8_t>(value & 0xFF);
    dst[1] = static_cast<uint8_t>((value >> 8) & 0xFF);
    dst[2] = static_cast<uint8_t>((value >> 16) & 0xFF);
    dst[3] = static_cast<uint8_t>((value >> 24) & 0xFF);
  }

  bool hasAudioClient()
  {
    return static_cast<bool>(audioClient) && audioClient.connected();
  }

  void initAudioStreamServer()
  {
    audioServer.begin();
    audioServer.setNoDelay(true);
    Serial.printf("Audio TCP stream ready on port %u\n", static_cast<unsigned int>(AUDIO_TCP_PORT));
  }

  void serviceAudioClient()
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

  void pushAudioFrame()
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

  const char *wifiModeToString(wifi_mode_t mode)
  {
    switch (mode)
    {
    case WIFI_MODE_NULL:
      return "OFF";
    case WIFI_MODE_STA:
      return "STA";
    case WIFI_MODE_AP:
      return "AP";
    case WIFI_MODE_APSTA:
      return "AP+STA";
    default:
      return "UNKNOWN";
    }
  }

  const char *wifiStatusToString(wl_status_t status)
  {
    switch (status)
    {
    case WL_IDLE_STATUS:
      return "IDLE";
    case WL_NO_SSID_AVAIL:
      return "NO_SSID";
    case WL_SCAN_COMPLETED:
      return "SCAN_DONE";
    case WL_CONNECTED:
      return "CONNECTED";
    case WL_CONNECT_FAILED:
      return "CONNECT_FAILED";
    case WL_CONNECTION_LOST:
      return "CONNECTION_LOST";
    case WL_DISCONNECTED:
      return "DISCONNECTED";
    default:
      return "UNKNOWN";
    }
  }

  IPAddress currentIpAddress()
  {
    const wifi_mode_t mode = WiFi.getMode();
    if (WiFi.status() == WL_CONNECTED)
    {
      return WiFi.localIP();
    }
    if (mode == WIFI_MODE_AP || mode == WIFI_MODE_APSTA)
    {
      return WiFi.softAPIP();
    }
    return IPAddress(0, 0, 0, 0);
  }

  void logHeartbeat()
  {
    static unsigned long lastLogMs = 0;
    const unsigned long now = millis();
    if (now - lastLogMs < 5000)
    {
      return;
    }
    lastLogMs = now;

    const wifi_mode_t mode = WiFi.getMode();
    const wl_status_t status = WiFi.status();
    const uint32_t uptimeSec = now / 1000;
    const size_t wsClients = ws.count();
    const bool audioConnected = hasAudioClient();
    const int apStations = WiFi.softAPgetStationNum();
    const IPAddress ip = currentIpAddress();

    Serial.printf(
        "[HEARTBEAT] uptime=%lus mode=%s status=%s ws_clients=%u audio_client=%s ap_stations=%d ip=%u.%u.%u.%u\n",
        static_cast<unsigned long>(uptimeSec),
        wifiModeToString(mode),
        wifiStatusToString(status),
        static_cast<unsigned int>(wsClients),
        audioConnected ? "yes" : "no",
        apStations,
        ip[0],
        ip[1],
        ip[2],
        ip[3]);
  }

  void printBootBanner()
  {
    Serial.println();
    Serial.println("========================================");
    Serial.println(" ESP32-S3 INMP441 Spectrogram Firmware ");
    Serial.printf(" Build date: %s %s\n", __DATE__, __TIME__);
    Serial.printf(" Sample rate: %d Hz | FFT: %d | Bins: %d\n", SAMPLE_RATE, FFT_SAMPLES, FFT_BINS);
    Serial.println("========================================");
  }

  void onWsEvent(AsyncWebSocket *serverRef,
                 AsyncWebSocketClient *client,
                 AwsEventType type,
                 void *arg,
                 uint8_t *data,
                 size_t len)
  {
    (void)serverRef;
    (void)arg;
    (void)data;
    (void)len;

    if (type == WS_EVT_CONNECT)
    {
      Serial.printf("WS client connected: %u\n", client->id());
    }
    else if (type == WS_EVT_DISCONNECT)
    {
      Serial.printf("WS client disconnected: %u\n", client->id());
    }
  }

  void connectWiFi()
  {
    WiFi.setSleep(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Connecting to Wi-Fi SSID '%s'", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
      for (int i = 0; i < 40; i++)
      {
        RgbLed::update(RgbLed::Mode::Disconnected);
        delay(10);
      }
      Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
      Serial.print("Wi-Fi connected. IP: ");
      Serial.println(WiFi.localIP());
      return;
    }

    Serial.println("Wi-Fi STA failed. Starting fallback AP...");
    WiFi.mode(WIFI_AP);
    const bool apOk = WiFi.softAP(AP_SSID, AP_PASSWORD);
    if (!apOk)
    {
      Serial.println("AP start failed. Rebooting in 3 seconds...");
      delay(3000);
      ESP.restart();
    }

    Serial.print("AP ready. Connect to SSID: ");
    Serial.println(AP_SSID);
    Serial.print("AP IP: ");
    Serial.println(WiFi.softAPIP());
  }

  void initI2S()
  {
    i2s_config_t i2sConfig = {
        .mode = static_cast<i2s_mode_t>(I2S_MODE_MASTER | I2S_MODE_RX),
        .sample_rate = SAMPLE_RATE,
        .bits_per_sample = I2S_BITS_PER_SAMPLE_32BIT,
        .channel_format = I2S_CHANNEL_FORMAT,
        .communication_format = I2S_COMM_FORMAT_STAND_I2S,
        .intr_alloc_flags = ESP_INTR_FLAG_LEVEL1,
        .dma_buf_count = 8,
        .dma_buf_len = 256,
        .use_apll = false,
        .tx_desc_auto_clear = false,
        .fixed_mclk = 0,
    };

    const esp_err_t installResult = i2s_driver_install(I2S_PORT, &i2sConfig, 0, nullptr);
    if (installResult != ESP_OK)
    {
      Serial.printf("I2S driver install failed: %d\n", installResult);
      while (true)
      {
        delay(1000);
      }
    }

    const esp_err_t pinResult = i2s_set_pin(I2S_PORT, &pin_config);
    if (pinResult != ESP_OK)
    {
      Serial.printf("I2S set pin failed: %d\n", pinResult);
      while (true)
      {
        delay(1000);
      }
    }

    i2s_zero_dma_buffer(I2S_PORT);
  }

  bool readAudioBlock()
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

  void computeSpectrum()
  {
    FFT.windowing(FFTWindow::Hamming, FFTDirection::Forward);
    FFT.compute(FFTDirection::Forward);
    FFT.complexToMagnitude();

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
      spectrum[i] = static_cast<uint8_t>(constrain(level, 0, 255));
    }
  }

  void pushSpectrum()
  {
    static unsigned long lastPushMs = 0;
    if (ws.count() == 0)
    {
      return;
    }

    const unsigned long now = millis();
    if (now - lastPushMs < WS_PUSH_INTERVAL_MS)
    {
      return;
    }

    if (!ws.availableForWriteAll())
    {
      return;
    }

    lastPushMs = now;

    ws.binaryAll(reinterpret_cast<const char *>(spectrum), FFT_BINS);
  }

  void initWebServer()
  {
    ws.onEvent(onWsEvent);
    server.addHandler(&ws);

    server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/html", WebUi::PAGE); });

    server.on("/health", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "application/json", "{\"ok\":true}"); });

    server.on("/ping", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "pong"); });

    server.on("/audio", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                char body[180];
                snprintf(body,
                         sizeof(body),
                         "{\"port\":%u,\"sample_rate\":%u,\"frame_samples\":%u,\"channels\":%u,\"format\":\"s16le\",\"header_magic\":\"AUD0\"}",
                         static_cast<unsigned int>(AUDIO_TCP_PORT),
                         static_cast<unsigned int>(SAMPLE_RATE),
                         static_cast<unsigned int>(FFT_SAMPLES),
                         static_cast<unsigned int>(AUDIO_STREAM_CHANNELS));
                request->send(200, "application/json", body); });

    server.begin();
    Serial.println("HTTP server started");
  }

} // namespace

void setup()
{
  Serial.begin(115200);
  const unsigned long serialWaitStart = millis();
  while (!Serial && millis() - serialWaitStart < 2500)
  {
    delay(10);
  }
  printBootBanner();

  RgbLed::init();
  RgbLed::update(RgbLed::Mode::Disconnected);

  connectWiFi();
  initI2S();
  initAudioStreamServer();
  initWebServer();

  Serial.println("System ready");
}

void loop()
{
  logHeartbeat();
  RgbLed::update(RgbLed::selectMode(ws.count() > 0));
  ws.cleanupClients();
  serviceAudioClient();

  if (!readAudioBlock())
  {
    return;
  }

  pushAudioFrame();

  computeSpectrum();
  pushSpectrum();
}
