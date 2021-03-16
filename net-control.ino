
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


// replaces placeholders
String processor(const String& var){
  if (var == "HOUR")
    return readHour();
  else if (var == "MINUTE")
    return readMinute();
  else if (var == "SECOND")
    return readSecond();
  else if (var == "SPEED")
    return read_speed();
  else if (var == "DISTANCE")
    return read_dist();
  else if (var == "INCLINE")
    return read_incline();
  else if (var == "ELEVATION")
    return read_elevation();
  else if (var == "VERSION")
    return VERSION;

  return String();
}


void onRootRequest(AsyncWebServerRequest *request) {
  request->send(SPIFFS, "/index.html", "text/html", false, processor);
}


void InitAsync_Webserver()
{
  server.on("/", HTTP_GET, onRootRequest);
  server.serveStatic("/", SPIFFS, "/");

  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", read_speed().c_str());
  });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", read_dist().c_str());
  });

  server.on("/incline", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", read_incline().c_str());
  });

  server.on("/elevation", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/plain", read_elevation().c_str());
  });


  // PUT requests to set speed and incline (not on the machine)
  server.on("/speed/up", HTTP_PUT, [](AsyncWebServerRequest *request) {
    speed_up();
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });
  server.on("/speed/down", HTTP_PUT, [](AsyncWebServerRequest *request) {
    speed_down();
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });
  server.on("/incline/up", HTTP_PUT, [](AsyncWebServerRequest *request) {
    incline_up();
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });
  server.on("/incline/down", HTTP_PUT, [](AsyncWebServerRequest *request) {
    incline_down();
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });
  server.on("^\\/speed\\/intervall\\/([0-9]+)$",
		     HTTP_PUT, [](AsyncWebServerRequest *request) {
    String interval = request->pathArg(0);
    set_speed_interval(interval.toInt());
    //request->send_P(200, "text/html", index_html);
    request->send(SPIFFS, "/index.html", "text/html", false, processor);
  });

  server.on("/hour", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readHour().c_str());
  });
  server.on("/minute", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readMinute().c_str());
  });
  server.on("/second", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/plain", readSecond().c_str());
  });

  // timer: start, stop, reset
  server.on("/reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    doReset();
    request->redirect("/");
  });

  // Start server
  server.begin();
}

