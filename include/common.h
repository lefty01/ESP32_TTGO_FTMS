#ifndef _ESP32_TTGO_FTMS_H_
#define _ESP32_TTGO_FTMS_H_

#undef ARDUINO_RASPBERRY_PI_PICO_W

//#define DEBUG 1
//#define DEBUG_MQTT 1
//#include "debug_print.h"
//#include "display.h"
//#include "wifi_mqtt_creds.h"

//#include <stdint.h>
#include <Arduino.h>

//#include <WiFi.h>
#if ASYNC_TCP_SSL_ENABLED
#include <AsyncTCP_SSL.h>
#endif
//#include <ESPAsyncWebServer.h>
//#include <PubSubClient.h>
//#include <ArduinoJson.h>

//const int BORDER = 2;
//const int HEADER = 16; // percent
//const int HEADER_PX = HEADER * tft.height();
//const int X_05 = tft.width()  / 2;
//const int Y_05 = tft.height() / 2;

#define EVERY_SECOND 1000
#define WIFI_CHECK   30 * EVERY_SECOND

// Events
enum class EventType {
  KEY_UP,
  KEY_LEFT,
  KEY_DOWN,
  KEY_RIGHT,
  KEY_OK,
  KEY_BACK,
  TREADMILL_REED,
  TREADMILL_START,
  TREADMILL_SPEED_DOWN,
  TREADMILL_INC_DOWN,
  TREADMILL_STOP,
  TREADMILL_SPEED_UP,
  TREADMILL_INC_UP,
};

// TODO clean up speedInclineMode some day, Is it needed outside debugging mode?
//      Yes maybe a user just want a unconected ESP32, if so should we support it?
//      Maybe, but maybe in some nicer way?? Code is a bit messy right now and
//      it's not 100% clear what it means 
enum SensorModeFlags {
    MANUAL  = 0x00, // speed and incline can be adjusted via button or webui
    SPEED   = 0x01, // b'01 ... only incline can be adjusted (speed from sensor)
    INCLINE = 0x02, // b'10 ... only speed can be adjusted (incline from sensor)
    SPEEDINCLINE_BITFIELD = 0x03 // b'11 ... no adjust/overwrite get speed and incline from sensor
};

extern bool setupDone;

extern uint8_t speedInclineMode;

extern float kmph;
extern float kmphIRsense;
extern float incline;
extern double totalDistance;
extern double elevationGain;
//extern volatile float speed_interval_step;
extern double gradeDeg;
extern double angle;

void logText(const char *text);
void logText(String text);
void logText(std::string text);

void initAsyncWebserver();
void initWebSocket();

void notifyClientsWebSockets();
void doReset(void);

void setSpeedInterval(float interval);
void speedDown();
void speedUp();
void inclineUp();
void inclineDown();

#endif
