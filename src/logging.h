#include <WiFi.h>
#include "settings.h"

class Logging
{
public:
    inline static void printHeartbeat()
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
        const IPAddress ip = currentIpAddress();

        Serial.printf(
            "[HEARTBEAT] uptime=%lus mode=%s status=%s ip=%u.%u.%u.%u\n",
            static_cast<unsigned long>(uptimeSec),
            wifiModeToString(mode),
            wifiStatusToString(status),
            ip[0],
            ip[1],
            ip[2],
            ip[3]);
    }

    inline static void printBanner()
    {
        Serial.println();
        Serial.println("========================================");
        Serial.println(" ESP32-S3 INMP441 Spectrogram Firmware");
        Serial.printf(" Build date: %s %s\n", __DATE__, __TIME__);
        Serial.printf(" Sample rate: %d Hz | FFT: %d | Bins: %d\n", SAMPLE_RATE, FFT_SAMPLES, FFT_BINS);
        Serial.println("========================================");
    }

    inline static void printReady()
    {
        Serial.println();
        Serial.println("========================================");
        Serial.println(" System ready. Connect to Wi-Fi or AP to");
        Serial.println(" stream audio and view the spectrogram. ");
        Serial.println("========================================");
    }

    inline static IPAddress currentIpAddress()
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

    inline static const char *wifiModeToString(wifi_mode_t mode)
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

    inline static const char *wifiStatusToString(wl_status_t status)
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
};