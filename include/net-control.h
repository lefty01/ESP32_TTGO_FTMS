#ifndef _NET_CONTROL_H_
#define _NET_CONTROL_H_

//extern PubSubClient client;

extern bool isWifiAvailable;
extern unsigned long wifiReconnectCounter;
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

int initMqtt(void);
//int initWifi(void);
void initWifi(void);
void initBLE(void);
bool mqttConnect(void);
String getWifiIpAddr(void);
void loopHandleWIFI(void);
void initBLE(void);
void loopHandleBLE(void); 
void updateBLEdata(void);
void resetStopWatch(void);
void publishTopicsMqtt(void);

#endif