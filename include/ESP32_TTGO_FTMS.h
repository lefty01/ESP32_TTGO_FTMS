#ifndef _ESP32_TTGO_FTMS_H_
#define _ESP32_TTGO_FTMS_H_

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

enum SensorModeFlags {
		      MANUAL  = 0,
		      SPEED   = 1,
		      INCLINE = 2,
		      _NUM_MODES_ = 4
#warning why 3modes and a define for the fourth?			  
};

extern float  kmph;
extern float kmph_sense;
extern float  incline;
extern double total_distance;
extern double elevation_gain;
extern uint8_t speedInclineMode;
extern volatile float speed_interval;
extern uint16_t    inst_incline;
extern uint16_t    inst_grade;
extern double grade_deg;
extern uint16_t    inst_elevation_gain;
extern double angle;

void logText(const char *text);
void logText(String text);

void updateDisplay(bool clear);
void updateHeader();
void initAsyncWebserver();
void initWebSocket();

void show_FPS(int fps);
void showSpeedInclineMode(uint8_t mode);
void updateBTConnectionStatus(bool connected);
void show_WIFI(const unsigned long reconnect_counter, const String &ip);
void notifyClients();
void doReset(void);

double angleSensorTreadmillConversion(double inAngle);
void setSpeedInterval(float interval);
void speedDown();
void speedUp();
void inclineUp();
void inclineDown();

#endif
