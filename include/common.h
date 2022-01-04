#ifndef _ESP32_TTGO_FTMS_H_
#define _ESP32_TTGO_FTMS_H_

#define DEBUG 1
//#define DEBUG_MQTT 1
#include "debug_print.h"
//#include "wifi_mqtt_creds.h"

#include <Arduino.h>

#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>

#ifdef MQTT_USE_SSL
#include <WiFiClientSecure.h>
extern WiFiClientSecure espClient;
#else
extern WiFiClient espClient;
#endif

extern PubSubClient client;
extern AsyncWebServer server;
extern AsyncWebSocket ws;


#ifdef USE_TFT_ESPI
  #include <TFT_eSPI.h>
#else
  #include <LovyanGFX.hpp>
  #include <LGFX_AUTODETECT.hpp>
#endif


#ifdef USE_TFT_ESPI
extern TFT_eSPI tft;
#else
extern LGFX tft;
#endif

// display is configured within platformio.ini
#ifndef TFT_WIDTH
#define TFT_WIDTH  135
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif


//const int BORDER = 2;
//const int HEADER = 16; // percent
//const int HEADER_PX = HEADER * tft.height();
//const int X_05 = tft.width()  / 2;
//const int Y_05 = tft.height() / 2;

#define CIRCLE_SPEED_X_POS   188
#define CIRCLE_INCLINE_X_POS 208
#define CIRCLE_BT_STAT_X_POS 227

#define CIRCLE_SPEED_Y_POS    11
#define CIRCLE_INCLINE_Y_POS  11
#define CIRCLE_BT_STAT_Y_POS  11
#define CIRCLE_Y_POS          11
#define CIRCLE_RADIUS          8


enum SensorModeFlags {
		      MANUAL  = 0,
		      SPEED   = 1,
		      INCLINE = 2,
		      _NUM_MODES_ = 4
};



extern float  kmph;
extern float  incline;
extern double total_distance;
extern double elevation_gain;
extern uint8_t speedInclineMode;

extern bool bleClientConnected;

extern unsigned long wifi_reconnect_counter;

extern esp_reset_reason_t rr;

extern String MQTTDEVICEID;
extern const char* VERSION;

extern volatile float speed_interval;

// mqtt topics
enum topics_t {
	       MQTT_TOPIC_SETCONFIG = 0,
	       MQTT_TOPIC_STATE,
	       MQTT_TOPIC_VERSION,
	       MQTT_TOPIC_IPADDR,
	       MQTT_TOPIC_RST,
	       MQTT_TOPIC_SPEED,
	       MQTT_TOPIC_RPM,
	       MQTT_TOPIC_INCLINE,
	       MQTT_TOPIC_Y_ANGLE,
	       MQTT_NUM_TOPICS
};

extern String mqtt_topics[];


String readHour();
String readMinute();
String readSecond();
String readDist();
String readIncline();
String readElevation();
String readSpeed();
void speedDown();
void speedUp();
void inclineUp();
void inclineDown();

void updateDisplay(bool clear);
void initAsyncWebserver();
void initWebSocket();
String getWifiIpAddr();
void show_FPS(int fps);
void showSpeedInclineMode(uint8_t mode);
void updateBTConnectionStatus(bool connected);
void show_WIFI(const unsigned long reconnect_counter, const String &ip);
String getWifiIpAddr();
void setSpeedInterval(float interval);
int setupWifi();
bool mqttConnect();
void notifyClients();
const char* getTopic(topics_t topic);
const char* getRstReason(esp_reset_reason_t r);


#endif
