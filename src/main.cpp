#include "settings.h"
#include "logging.h"
#include "rgbled.h"
#include "audio.h"
#include "web.h"

AudioServer audio;
WebServer web;

void setup()
{
  // start a serial port for logging, but wait up to 5 seconds
  // (e.g. for USB CDC console) before proceeding with setup
  Serial.begin(115200);
  const unsigned long startMillis = millis();
  while (!Serial && millis() - startMillis < 5000)
  {
    delay(10);
  }
  Logging::printBanner();

  // connect to wifi and/or start AP
  web.connect();

  // start the audio stream server
  audio.init();

  // start the web server
  web.init();

  // success - print a happy message
  Logging::printReady();
}

void loop()
{
  // show signs of life
  Logging::printHeartbeat();

  // service the audio stream
  bool updated = audio.process();

  // service the websocket clients
  web.process(updated, audio.spectrum(), audio.pcm());
}
