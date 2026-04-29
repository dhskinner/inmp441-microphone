#pragma once
#include "settings.h"
#include "rgbled.h"
#include "webui.h"
#include <WiFi.h>
#include <ESPAsyncWebServer.h>

enum class WebMode
{
    Disconnected,
    AccessPointIdle,
    WifiConnectedIdle,
    ClientConnected
};

class WebServer
{
protected:
    AsyncWebServer server;
    AsyncWebSocket ws;
    RgbLed led;

    void onEvent(AsyncWebSocket *serverRef,
                 AsyncWebSocketClient *client,
                 AwsEventType type,
                 void *arg,
                 uint8_t *data,
                 size_t len);
    void pushSpectrum(Spectrum &spectrum);

public:
    WebServer();
    void init();
    void connect();
    void process(bool updated, Spectrum &spectrum);
};
