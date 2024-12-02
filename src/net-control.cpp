/*
Handle BT, Wifisetup, webserver and mqtt

TODO: Maybe we want to split this up in different files later? Lets see how it grows
*/
#define NIMBLE
#include <LittleFS.h>
#include <PubSubClient.h>
#include <ArduinoJson.h>
#include <TimeLib.h>  // https://playground.arduino.cc/Code/Time/

//#include <WiFiManager.h> // https://github.com/tzapu/WiFiManager
#include <ESPAsyncWebServer.h>     //Local WebServer used to serve the configuration portal
#include <ESPAsyncWiFiManager.h>   //https://github.com/tzapu/WiFiManager WiFi Configuration Magic
#include <ElegantOTA.h>

#include "common.h"
#include "display.h"
#include "net-control.h"
#include "config.h"
#include "debug_print.h"
#include "hardware.h"
#include "wifi_mqtt_creds.h"


#include <NimBLEDevice.h>
#ifdef MQTT_USE_SSL
#include <WiFiClientSecure.h>
extern WiFiClientSecure espClient;
#else
extern WiFiClient espClient;
#endif

bool bleClientConnected = false;
bool bleClientConnectedPrev = false;

#ifdef MQTT_USE_SSL
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
#ifdef ASYNC_TCP_SSL_ENABLED
AsyncWebServer server(443);
#else
AsyncWebServer server(80);
#endif
AsyncWebSocket ws("/ws");
PubSubClient client(espClient);  // mqtt client
DNSServer dns;

//extern AsyncWebServer server;
//extern AsyncWebSocket ws;

AsyncWiFiManager wifiManager(&server,&dns);

//cs bool WifiAvailable = false;
bool isMqttAvailable = false;
bool isWifiAvailable = false;
unsigned long wifiReconnectCounter = 0;

static unsigned long mqtt_reconnect_counter = 0;
unsigned long wifi_reconnect_timer = 0;

String ipAddr;
String dnsAddr;
uint8_t macAddr[6];

String MQTTDEVICEID = "ESP32_FTMS_";

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
	       MQTT_TOPIC_DIST,
	       MQTT_TOPIC_ELEGAIN,
	       MQTT_NUM_TOPICS
};

String mqtt_topics[] {
  "home/treadmill/%MQTTDEVICEID%/setconfig",
  "home/treadmill/%MQTTDEVICEID%/state",
  "home/treadmill/%MQTTDEVICEID%/version",
  "home/treadmill/%MQTTDEVICEID%/ipaddr",
  "home/treadmill/%MQTTDEVICEID%/rst",
  "home/treadmill/%MQTTDEVICEID%/speed",
  "home/treadmill/%MQTTDEVICEID%/rpm",
  "home/treadmill/%MQTTDEVICEID%/incline",
  "home/treadmill/%MQTTDEVICEID%/y_angle",
  "home/treadmill/%MQTTDEVICEID%/dist",
  "home/treadmill/%MQTTDEVICEID%/elegain"
};

#ifdef ASYNC_TCP_SSL_ENABLED
// if using board_build.embed_txtfiles instead of spiffs -> that would not require the callback for:
// server.onSslFileRequest(&sslFileRequestCallback, NULL);
// (int sslFileRequestCallback(void *arg, const char *filename, uint8_t **buf))
extern const uint8_t server_crt_start[] asm("_binary_server_crt_start");
extern const uint8_t server_key_start[] asm("_binary_server_key_start");
// server.beginSecure((const char*)server_crt_start, (const char*)server_key_start, NULL);
#endif

// note: Fitness Machine Feature is a mandatory characteristic (property_read)
#define FTMSService BLEUUID((uint16_t)0x1826)

BLEServer* pServer = NULL;
BLECharacteristic* pTreadmill    = NULL;
BLECharacteristic* pFeature      = NULL;
BLECharacteristic* pControlPoint = NULL;
BLECharacteristic* pStatus       = NULL;
BLEAdvertisementData advert;
BLEAdvertisementData scan_response;
BLEAdvertising *pAdvertising;

// {0x2ACD,"Treadmill Data"},
BLECharacteristic TreadmillDataCharacteristics(BLEUUID((uint16_t)0x2ACD),
					       NIMBLE_PROPERTY::NOTIFY
					       );

BLECharacteristic FitnessMachineFeatureCharacteristic(BLEUUID((uint16_t)0x2ACC),
						      NIMBLE_PROPERTY::READ
						      );

BLEDescriptor TreadmillDescriptor(BLEUUID((uint16_t)0x2901)
				  , NIMBLE_PROPERTY::READ,1000
				  );

// seems kind of a standard callback function
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleClientConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    bleClientConnected = false;
  }
};

void loopHandleBLE()
{
  // if changed to connected ...
  if (bleClientConnected && !bleClientConnectedPrev) {
    bleClientConnectedPrev = true;
    logText("BT Client connected!\n");
#ifndef NO_DISPLAY
    gfxUpdateHeader();
#endif
  }
  else if (!bleClientConnected && bleClientConnectedPrev) {
    bleClientConnectedPrev = false;
    logText("BT Client disconnected!\n");
#ifndef NO_DISPLAY
    gfxUpdateHeader();
#endif
  }
}

void setupDeviceID()
{
  esp_efuse_mac_get_default(macAddr);
  MQTTDEVICEID += String(macAddr[4], HEX);
  MQTTDEVICEID += String(macAddr[5], HEX);
}

void initBLE() {
  logText("init BLE...");

  BLEDevice::init(MQTTDEVICEID.c_str());  // set server name (here: MQTTDEVICEID)

  // create BLE Server, set callback for connect/disconnect
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // create the fitness machine service within the server
  BLEService *pService = pServer->createService(FTMSService);

  pService->addCharacteristic(&TreadmillDataCharacteristics);
  TreadmillDescriptor.setValue("Treadmill Descriptor Value ABC");
  TreadmillDataCharacteristics.addDescriptor(&TreadmillDescriptor);
  pService->addCharacteristic(&FitnessMachineFeatureCharacteristic);

  // https://www.bluetooth.com/specifications/gatt/viewer?attributeXmlFile=org.bluetooth.descriptor.gatt.client_characteristic_configuration.xml
  // Create a BLE Descriptor
  /***************************************************
   NOTE: DO NOT create a 2902 descriptor.
   it will be created automatically if notifications
   or indications are enabled on a characteristic.
  ****************************************************/
  // start service
  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponse(true);
  pAdvertising->addServiceUUID(FTMSService);
  //pAdvertising->setMinPreferred(0x06);  // set value to 0x00 to not advertise this parameter

  pAdvertising->start();
  logText("done\n");
}

void updateBLEdata(void)
{
  uint16_t inst_incline = 0;
  uint16_t inst_grade = 0;
  uint16_t inst_elevation_gain = 0;
  uint8_t treadmillData[34] = {};
  uint16_t flags = 0x0018;  // b'000000011000
  //                             119876543210
  //                             20

  // get speed and incline ble ready
  uint16_t inst_speed = kmph * 100;    // kilometer per hour with a resolution of 0.01
  inst_incline = incline * 10;         // percent with a resolution of 0.1
  inst_grade   = gradeDeg * 10;
  inst_elevation_gain = elevationGain * 10;

  // now data is filled starting at bit0 and appended for the
  // fields we 'enable' via the flags above ...
  //treadmillData[0,1] -> flags
  treadmillData[0] = (uint8_t)(flags & 0xFF);
  treadmillData[1] = (uint8_t)(flags >> 8);

  // speed
  treadmillData[2] = (uint8_t)(inst_speed & 0xFF);
  treadmillData[3] = (uint8_t)(inst_speed >> 8);

  // incline & degree
  treadmillData[4] = (uint8_t)(inst_incline & 0xFF);
  treadmillData[5] = (uint8_t)(inst_incline >> 8);
  treadmillData[6] = (uint8_t)(inst_grade & 0xFF);
  treadmillData[7] = (uint8_t)(inst_grade >> 8);

  // Positive Elevation Gain 16 Meters with a resolution of 0.1
  treadmillData[8] = (uint8_t)(inst_elevation_gain & 0xFF);
  treadmillData[9] = (uint8_t)(inst_elevation_gain >> 8);

   // flags do enable negative elevation as well but this will be always 0
  TreadmillDataCharacteristics.setValue(treadmillData, 34);
  TreadmillDataCharacteristics.notify();
}

String getWifiIpAddr() {
  return ipAddr;
}

void resetWifiConnection(AsyncWebServerRequest *request)
{
  wifiManager.resetSettings();
  logText("Wifi connection reset\n");
  while(1)
  {
    // hangup for reset by wdt
  }
}

void resetStopWatch(void)
{
  setTime(0,0,0,0,0,0);
}

//csint initWifi()
void initWifi()
{
  bool res;

  logText("Init Wifi...");
  // reset settings - wipe stored credentials for testing
  // these are stored by the esp library
  // wm.resetSettings();

  // Automatically connect using saved credentials,
  // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
  // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
  // then goes into a blocking loop awaiting configuration and will return success result
  //Local intialization. Once its business is done, there is no need to keep it around

  //reset saved settings
  //wifiManager.resetSettings();
  //wifiManager.setDebugOutput(true);
  //res = wifiManager.autoConnect(); // auto generated AP name from chipid
  // res = wm.autoConnect("AutoConnectAP"); // anonymous ap
  res = wifiManager.autoConnect("FTMS Threadmill AP");

  if (!res) {
    isWifiAvailable = false;
    logText("failed\n");
  }
  else {
    isWifiAvailable = true;
    //if you get here you have connected to the WiFi
    ipAddr  = WiFi.localIP().toString();
    dnsAddr = WiFi.dnsIP().toString();

    logText(" OK\n");
    logText("IP address: ");
    logText(ipAddr.c_str());
    logText("DNS address: ");
    logText(dnsAddr.c_str());
    logText("\n");
  }
}

void loopHandleWIFI()
{
//  static unsigned long mqtt_reconnect_counter = 0;
//  unsigned long wifi_reconnect_timer = 0;
//  bool isWifiAvailable = false;

  // re-connect to wifi
  if ((WiFi.status() != WL_CONNECTED) && ((millis() - wifi_reconnect_timer) > WIFI_CHECK)) {
    wifi_reconnect_timer = millis();
    isWifiAvailable = false;
    isMqttAvailable = false;
    mqtt_reconnect_counter = 0;

    logText("Disconnect and Reconnecting to WiFi...\n");
    WiFi.disconnect();
    WiFi.reconnect();
  }

  if (!isWifiAvailable && (WiFi.status() == WL_CONNECTED))
  {
    // connection was lost and now got reconnected ...
    isWifiAvailable = true;
    wifiReconnectCounter++;
    logText("Reconnecting to wifi...\n");
    logText("isWifiAvailable=");
    logText(isWifiAvailable?"Yes":"No");
    logText(" WiFi.status=");
    logText(WiFi.status()== WL_CONNECTED?"Connected":"Not Connected");
    logText("\n");

#ifndef NO_DISPLAY
    gfxUpdateHeader();
#endif
  }

  if (!isMqttAvailable && isWifiAvailable)
  {
    // TODO Add a menu item to start a new retry?
    //      Or and mqtt enable/disable config when we have on device configs
    //      Do we want to retry this more less often like every 15 min or 1h?
    // Limit this to 2 retrys to not bug down a system forever if not availible
    if (mqtt_reconnect_counter < 2)
    {
      logText("Reconnecting to mqtt...\n");
      mqtt_reconnect_counter++;
      isMqttAvailable = mqttConnect();
#ifndef NO_DISPLAY
      gfxUpdateDisplay(true); //TODO replace this with gfxUpdateHeader();
      gfxUpdateHeader();
#endif
    }
  }
}

const char* getTopic(topics_t topic)
{
  return mqtt_topics[topic].c_str();
}

void publishTopicsMqtt(void)
{
  char kmphStr[6];
  snprintf(kmphStr,    6, "%.1f", kmph);

  client.publish(getTopic(MQTT_TOPIC_SPEED),   kmphStr);
}

void setupMqttTopic(const String &id)
{
  for (unsigned i = 0; i < MQTT_NUM_TOPICS; ++i) {
    mqtt_topics[i].replace("%MQTTDEVICEID%", id);
  }
}

int initMqtt(void)
{
  logText("Init Mqtt...");

  setupMqttTopic(MQTTDEVICEID);
  isMqttAvailable = mqttConnect();
  delay(2000);  // TODO why 2s delay?

  logText("mqtt setup done!\n");

  return isMqttAvailable;
}

bool mqttConnect(void)
{
  bool result = false;

  logText("Attempting MQTT connection to server: ");
  logText(mqtt_host);

  // Attempt to connect
#ifdef MQTT_USE_SSL
  logText(" [SSL]");
  espClient.setCACert(server_crt_str);
  espClient.setCertificate(client_crt_str);
  espClient.setPrivateKey(client_key_str);
#endif
  logText("\n");

  client.setServer(mqtt_host, mqtt_port);
  if (client.connect(MQTTDEVICEID.c_str(), mqtt_user, mqtt_pass,
		     getTopic(MQTT_TOPIC_STATE), 1, 1, "OFFLINE"))
  {
    // Once connected, publish an announcement...
    logText("publish connected...");
    bool rc = client.publish(getTopic(MQTT_TOPIC_STATE),  "CONNECTED", true);
    if (rc) logText("OK\n");
    else    logText("ERROR\n");

    logText("publish version & IP...");
    rc |= client.publish(getTopic(MQTT_TOPIC_RST), getRstReason(reset_reason), true);
    rc |= client.publish(getTopic(MQTT_TOPIC_VERSION), VERSION, true);
    rc |= client.publish(getTopic(MQTT_TOPIC_IPADDR), ipAddr.c_str(), true);
    if (rc) logText("OK\n");
      else  logText("ERROR\n");

    logText("MQTT CONNECTED\n");
    result = true;
  }
  else
  {
    logText("MQTT FAILED, rc=");
    logText(std::to_string(client.state()));
    logText("\n");
    result= false;
  }
return result;
}

// replaces placeholders
String processor(const String& var)
{
  if (var == "HOUR")
    return readHour();
  else if (var == "MINUTE")
    return readMinute();
  else if (var == "SECOND")
    return readSecond();
  else if (var == "SPEED")
    return readSpeed();
  else if (var == "DISTANCE")
    return readDist();
  else if (var == "INCLINE")
    return readIncline();
  else if (var == "ELEVATION")
    return readElevation();
  else if (var == "VERSION")
    return VERSION;
  else if (var == "RESET_REASON")
    return getRstReason(reset_reason);

  return String();
}

void onNotFound(AsyncWebServerRequest* request)
{
  request->send(404, "text/plain", "Not found");
}

void onRootRequest(AsyncWebServerRequest *request)
{
  request->send(LittleFS, "/index.html", "text/html", false, processor);
}

#ifdef ASYNC_TCP_SSL_ENABLED
int sslFileRequestCallback(void *arg, const char *filename, uint8_t **buf)
{
  //Serial.printf("SSL File: %s\n", filename);
  // sanitize filename!??
  File file = SPIFFS.open(filename, "r");
  if (file) {
    //    ssize_t bytes_read;
    size_t size = file.size();
    uint8_t * nbuf = (uint8_t*) malloc(size);
    if (nbuf) {
      size = file.read(nbuf, size);
      file.close();
      *buf = nbuf;
      return size;
    }
    file.close();
  }
  *buf = 0;
  return 0;
}
#endif

void initAsyncWebserver()
{
  logText("init webserver...");
  server.on("/", HTTP_GET, onRootRequest);
  server.on("/resetwifi", HTTP_GET, resetWifiConnection);
  server.serveStatic("/", LittleFS, "/");

  ElegantOTA.begin(&server);
  // Start server
#ifdef ASYNC_TCP_SSL_ENABLED
  //server.onSslFileRequest(&sslFileRequestCallback, NULL);
  server.beginSecure((const char*)server_crt_start, (const char*)server_key_start, NULL);
#else
    server.begin();
#endif
  logText("done\n");
}

// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------

// https://arduinojson.org/v6/assistant/

void notifyClientsWebSockets()
{
  StaticJsonDocument<256> doc;

  doc["speed"]          = kmph;
  doc["incline"]        = incline;
  doc["speed_interval"] = configTreadmill.speed_interval_step;
  doc["sensor_mode"]    = speedInclineMode;
  doc["distance"]       = totalDistance / 1000;
  doc["elevation"]      = elevationGain;
  doc["hour"]   = readHour();
  doc["minute"] = readMinute();
  doc["second"] = readSecond();

  char buffer[192];
  size_t len = serializeJson(doc, buffer);
  ws.textAll(buffer, len);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len)
{
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 &&
      info->len == len && info->opcode == WS_TEXT) {

    StaticJsonDocument<32> doc;
    DeserializationError err = deserializeJson(doc, data, len);

    if (err) {
      logText("deserializeJson() failed with code: ");
      logText(err.f_str());
      logText("\n");
      return;
    }

    const char* command = doc["command"]; // e.g. "speed_interval"
    const char* value   = doc["value"];   // e.g. "0.5", "down"

    if (strcmp(command, "sensor_mode") == 0) {
      if (strcmp(value, "speed") == 0)
	speedInclineMode ^= SPEED; // b'01 toggle bit

      if (strcmp(value, "incline") == 0)
	speedInclineMode ^= INCLINE; // b'10

#ifndef NO_DISPLAY
      gfxUpdateHeader();
#endif
    }
    if (strcmp(command, "speed") == 0) {
      if (strcmp(value, "up") == 0)
	speedUp();
      if (strcmp(value, "down") == 0)
	speedDown();
    }
    if (strcmp(command, "incline") == 0) {
      if (strcmp(value, "up") == 0)
	inclineUp();
      if (strcmp(value, "down") == 0)
	inclineDown();
    }
    if (strcmp(command, "speed_interval") == 0) {
      if (strcmp(value, "0.1") == 0)
	setSpeedInterval(0.1);
      if (strcmp(value, "0.5") == 0)
	setSpeedInterval(0.5);
      if (strcmp(value, "1.0") == 0)
	setSpeedInterval(1.0);
    }
    notifyClientsWebSockets();
  }
}

void onEvent(AsyncWebSocket       *server,
	     AsyncWebSocketClient *client,
	     AwsEventType          type, // connect/disconnect/data/pong
	     void                 *arg,
	     uint8_t              *data,
	     size_t                len)
{
  switch (type)
  {
    case WS_EVT_CONNECT:
      DEBUG_PRINTF("WebSocket client #%u connected from %s\n", client->id(),
		   client->remoteIP().toString().c_str());
      break;
    case WS_EVT_DISCONNECT:
      DEBUG_PRINTF("WebSocket client #%u disconnected\n", client->id());
      break;
    case WS_EVT_DATA:
      handleWebSocketMessage(arg, data, len);
      break;
    case WS_EVT_PONG:
    case WS_EVT_ERROR:
      break;
  }
}


String readSpeed()
{
  return String(kmph);
}

String readDist()
{
  return String(totalDistance / 1000);
}

String readIncline()
{
  return String(incline);
}

String readElevation()
{
  return String(elevationGain);
}

String readHour() {
  return String(hour());
}

String readMinute() {
  int m = minute();
  String mStr(m);

  if (m < 10)
    mStr = "0" + mStr;

  return mStr;
}

String readSecond() {
  int s = second();
  String sStr(s);

  if (s < 10)
    sStr = "0" + sStr;

  return sStr;
}

void initWebSocket()
{
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}
