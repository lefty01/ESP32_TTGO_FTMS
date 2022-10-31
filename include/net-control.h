#ifndef _NET_CONTROL_H_
#define _NET_CONTROL_H_

//extern PubSubClient client;

extern bool isWifiAvailable;
extern String MQTTDEVICEID;
extern String mqtt_topics[];
extern bool bleClientConnected;

String readHour(void);
String readMinute(void);
String readSecond(void);
String readSpeed(void);
String readDist();
String readIncline();
String readElevation();

void initDisplay(void);
int initMqtt(void);
//int initWifi(void);
void initWifi(void);
void initBLE(void);
bool mqttConnect(void);
String getWifiIpAddr(void);
void loop_handle_WIFI(void);
void initBLE(void);
void loop_handle_BLE(void); 
void updateBLEdata(void);
void resetStopWatch(void);
void publish_topics(void);

#endif