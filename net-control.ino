
String ipAddr;
String dnsAddr;
const unsigned maxWifiWaitSeconds = 60;


// note: delays mainly to keep tft text shortly readable
int setupWifi() {
  DEBUG_PRINTLN();
  DEBUG_PRINTLN("Connecting to wifi");

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);

  tft.setCursor(20, 60);
  tft.println("Connecting to WiFi");

  unsigned retry_counter = 0;
  WiFi.begin(wifi_ssid, wifi_pass);

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    DEBUG_PRINT(".");
    tft.print(".");
    retry_counter++;
    if (retry_counter > maxWifiWaitSeconds) {
      DEBUG_PRINTLN(" TIMEOUT!");
      tft.fillScreen(TFT_BLACK);
      tft.setCursor(20, 60);
      tft.println("Wifi TIMEOUT");
      return 1;
    }
  }
  ipAddr  = WiFi.localIP().toString();
  dnsAddr = WiFi.dnsIP().toString();

  DEBUG_PRINTLN("");
  DEBUG_PRINTLN("WiFi connected");
  DEBUG_PRINTLN("IP address: ");
  DEBUG_PRINTLN(ipAddr);
  DEBUG_PRINTLN("DNS address: ");
  DEBUG_PRINTLN(dnsAddr);

  tft.fillScreen(TFT_BLACK);
  tft.setCursor(20, 60);
  tft.println("Wifi CONNECTED");
  tft.print("IP Addr: "); tft.println(ipAddr);
  delay(3000);
  tft.fillScreen(TFT_BLACK);
  return 0;
}


void mqttConnect() {
  DEBUG_PRINT("Attempting MQTT connection...");
  // Attempt to connect
  if (client.connect(MQTTDEVICEID, mqtt_user_int, mqtt_pass_int,
		     "home/treadmill/status", 1, 1, "OFFLINE")) {
    DEBUG_PRINTLN("connected");
    // Once connected, publish an announcement...
    client.publish("home/treadmill/state",  "CONNECTED", true);
    client.publish("home/treadmill/version", VERSION,    true);
  } else {
    DEBUG_PRINT("failed, rc=");
    DEBUG_PRINTLN(client.state());
  }
}


// replaces placeholders
String processor(const String& var){
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

  return String();
}



void onRootRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html", false, processor);
}


void initAsyncWebserver()
{
  server.on("/", HTTP_GET, onRootRequest);
  server.serveStatic("/", SPIFFS, "/");

  // Start server
  server.begin();
}




// ----------------------------------------------------------------------------
// WebSocket initialization
// ----------------------------------------------------------------------------



// https://arduinojson.org/v6/assistant/

void notifyClients() {
  StaticJsonDocument<256> doc;

  doc["speed"]          = kmph;
  doc["incline"]        = incline;
  doc["speed_interval"] = speed_interval;
  doc["sensor_mode"]    = speedInclineMode;
  doc["distance"]       = total_distance / 1000;
  doc["elevation"]      = elevation_gain;
  doc["hour"]   = readHour();
  doc["minute"] = readMinute();
  doc["second"] = readSecond();

  char buffer[192];
  size_t len = serializeJson(doc, buffer);
  //DEBUG_PRINT("serialize json len: ");
  //DEBUG_PRINTLN(len);
  ws.textAll(buffer, len);
}


void handleWebSocketMessage(void *arg, uint8_t *data, size_t len) {
  AwsFrameInfo *info = (AwsFrameInfo*)arg;

  if (info->final && info->index == 0 &&
      info->len == len && info->opcode == WS_TEXT) {

    StaticJsonDocument<32> doc;
    DeserializationError err = deserializeJson(doc, data, len);

    if (err) {
      DEBUG_PRINT(F("deserializeJson() failed with code "));
      DEBUG_PRINTLN(err.f_str());
      return;
    }

    const char* command = doc["command"]; // e.g. "speed_interval"
    const char* value   = doc["value"];   // e.g. "0.5", "down"

    if (strcmp(command, "sensor_mode") == 0) {
      if (strcmp(value, "speed") == 0)
	speedInclineMode ^= SPEED; // b'01 toggle bit

      if (strcmp(value, "incline") == 0)
	speedInclineMode ^= INCLINE; // b'10

      showSpeedInclineMode(speedInclineMode);
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
    notifyClients();
  }
}

void onEvent(AsyncWebSocket       *server,
             AsyncWebSocketClient *client,
             AwsEventType          type, // connect/disconnect/data/pong
             void                 *arg,
             uint8_t              *data,
             size_t                len) {
  switch (type) {
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

void initWebSocket() {
  ws.onEvent(onEvent);
  server.addHandler(&ws);
}

