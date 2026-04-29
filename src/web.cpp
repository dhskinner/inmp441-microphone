#include "web.h"

WebServer::WebServer() : server(80),
                         ws("/ws")
{
}

void WebServer::connect()
{
    // start a led for proof of life and status indication
    led.init(RGB_LED_LDO2_ENABLE_PIN, RGB_LED_DATA_PIN, RGB_LED_BRIGHTNESS);
    led.set(RgbMode::BlinkRed);

    WiFi.setSleep(false);
    WiFi.mode(WIFI_STA);
    WiFi.begin(WIFI_SSID, WIFI_PASSWORD);

    Serial.printf("Connecting to Wi-Fi SSID '%s'", WIFI_SSID);
    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 10000)
    {
        for (int i = 0; i < 40; i++)
        {
            led.update();
            delay(10);
        }
        Serial.print('.');
    }
    Serial.println();

    if (WiFi.status() == WL_CONNECTED)
    {
        Serial.print("Wi-Fi connected. IP: ");
        Serial.println(WiFi.localIP());
        led.set(RgbMode::BlinkGreen);
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
    led.set(RgbMode::BlinkBlue);
}

void WebServer::init()
{
    ws.onEvent([this](AsyncWebSocket *serverRef,
                      AsyncWebSocketClient *client,
                      AwsEventType type,
                      void *arg,
                      uint8_t *data,
                      size_t len)
               { this->onEvent(serverRef, client, type, arg, data, len); });
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

    server.on("/spectrum", HTTP_GET, [](AsyncWebServerRequest *request)
              {
                char body[200];
                snprintf(body,
                         sizeof(body),
                         "{\"min_hz\":%u,\"max_hz\":%u,\"sample_rate\":%u,\"fft_bins\":%u}",
                         static_cast<unsigned int>(SPECTRUM_MIN_HZ),
                         static_cast<unsigned int>(SPECTRUM_MAX_HZ),
                         static_cast<unsigned int>(SAMPLE_RATE),
                         static_cast<unsigned int>(FFT_BINS));
                request->send(200, "application/json", body); });

    server.begin();
    Serial.println("HTTP server started");
}

void WebServer::process(bool updated, Spectrum &spectrum)
{
    // signs of life
    if ((ws.count() > 0))
        led.set(RgbMode::Multicolor);
    else if (WiFi.status() == WL_CONNECTED)
        led.set(RgbMode::BlinkGreen);
    else if (WiFi.getMode() == WIFI_MODE_AP || WiFi.getMode() == WIFI_MODE_APSTA)
        led.set(RgbMode::BlinkBlue);
    else
        led.set(RgbMode::BlinkRed);

    // clean up disconnected WS clients to prevent resource exhaustion
    ws.cleanupClients();

    // push spectrum data to WS clients if connected and updated
    if (updated)
    {
        pushSpectrum(spectrum);
    }
}

void WebServer::pushSpectrum(Spectrum &spectrum)
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

void WebServer::onEvent(AsyncWebSocket *serverRef,
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
