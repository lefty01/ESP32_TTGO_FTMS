#ifndef _debug_print_h_
#define _debug_print_h_


#ifdef DEBUG

#ifdef DEBUG_MQTT
#include <PubSubClient.h>
#include <map>

std::map<int, std::string> mqtt_state_map = {
  {MQTT_CONNECTION_TIMEOUT,      "MQTT_CONNECTION_TIMEOUT"},
  {MQTT_CONNECTION_LOST,         "MQTT_CONNECTION_LOST"},
  {MQTT_CONNECT_FAILED,          "MQTT_CONNECT_FAILED"},
  {MQTT_DISCONNECTED,            "MQTT_DISCONNECTED"},
  {MQTT_CONNECT_BAD_PROTOCOL,    "MQTT_CONNECT_BAD_PROTOCOL"},
  {MQTT_CONNECT_BAD_CLIENT_ID,   "MQTT_CONNECT_BAD_CLIENT_ID"},
  {MQTT_CONNECT_UNAVAILABLE,     "MQTT_CONNECT_UNAVAILABLE"},
  {MQTT_CONNECT_BAD_CREDENTIALS, "MQTT_CONNECT_BAD_CREDENTIALS"},
  {MQTT_CONNECT_UNAUTHORIZED,    "MQTT_CONNECT_UNAUTHORIZED"}
};

const char* state2str(int state) {
  return std::string(mqtt_state_map[state]).c_str();
}

#ifndef MQTTDEVICEID
#error "MQTTDEVICEID not defined! define before include of this header"
#endif
const char* mqttDbg = MQTTDEVICEID "/debug";

#define DEBUG_PRINT_MQTTSTATE(x) Serial.println(state2str(x))
#define DEBUG_PRINTMQTT(x)    mqttClient.publish(mqttDbg, x)
#else
#define DEBUG_PRINT_MQTTSTATE(x)
#endif

#define DEBUG_BEGIN(x)        Serial.begin(x)
#define DEBUG_PRINT(x)        Serial.print(x)
#define DEBUG_PRINTLN(x)      Serial.println(x)
#define DEBUG_PRINTDEC(x)     Serial.print(x, DEC)
#define DEBUG_PRINTHEX(x)     Serial.print(x, HEX)
#define DEBUG_PRINTLNHEX(x)   Serial.println(x, HEX)
#define DEBUG_PRINTF(x_, ...) Serial.printf((x_), ##__VA_ARGS__)

#else

#define DEBUG_BEGIN(x)
#define DEBUG_PRINT(x)
#define DEBUG_PRINTLN(x)
#define DEBUG_PRINTDEC(x)
#define DEBUG_PRINTHEX(x)
#define DEBUG_PRINTLNHEX(x)
#define DEBUG_PRINT_MQTTSTATE(x)
#define DEBUG_PRINTF(x_, ...)
#define DEBUG_PRINTMQTT(x)
#endif

#endif
