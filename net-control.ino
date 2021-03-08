
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
    // display.drawXbm(46, 30, 8, 8, retry_counter % 3 == 0 ? activeSymbole : inactiveSymbole);
    // display.drawXbm(60, 30, 8, 8, retry_counter % 3 == 1 ? activeSymbole : inactiveSymbole);
    // display.drawXbm(74, 30, 8, 8, retry_counter % 3 == 2 ? activeSymbole : inactiveSymbole);

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


const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html>
<head>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  <link rel="stylesheet" href="https://use.fontawesome.com/releases/v5.7.2/css/all.css" integrity="sha384-fnmOCqbTlWIlj8LyTjo7mOUStjsKC4pOpQbqyi7RrhN7udi9RwhKkMHpvLbHG9Sr" crossorigin="anonymous">
  <style>
    html {
     font-family: Arial;
     display: inline-block;
     margin: 0px auto;
     text-align: center;
    }
    h2 { font-size: 3.0rem; }
    p { font-size: 3.0rem; }
    .units { font-size: 1.2rem; }
    .dht-labels{
      font-size: 1.5rem;
      vertical-align:middle;
      padding-bottom: 15px;
    }

.button {
  border: none;
  color: white;
  padding: 15px 32px;
  text-align: center;
  text-decoration: none;
  display: inline-block;
  font-size: 16px;
  margin: 4px 2px;
  cursor: pointer;
}
.button1 {background-color: #4CAF50;} /* Green */
.button2 {background-color: #008CBA;} /* Blue */
.button3 {background-color: #cc0000;} /* Red */
  </style>
</head>
<body>
   <p>
    <i class="fas fa-running" style="color:#059e8a;"></i>
    <!-- <span class="dht-labels">Speed</span> -->
    <span id="speed">%SPEED%</span>
    <sup class="units">kph</sup>
  </p>
    <p>
    <i class="fas fa-shoe-prints" style="color:#2d0000;"></i>
    &nbsp;&nbsp;
    <!-- <span class="dht-labels">Distance</span> -->
    <span id="distance">%DISTANCE%</span>
    <sup class="units">M</sup>
  </p>
  <p>
   <i class="fas fa-stopwatch" style="color:#059e8a;"></i>
    <!-- <span class="dht-labels">Time</span> -->
    <span id="hour">%HOUR%</span>
    :
    <span id="minute">%MINUTE%</span>
    :
    <span id="second">%SECOND%</span>
  </p>
   <p>
    <i class="fas fa-arrows-alt-h" style="color:#059e8a;"></i>
    <!-- <span class="dht-labels">Zoffset</span> -->
    <span id="zoffset">%ZOFFSET%</span>
    <sup class="units">kph</sup>
  <br>
    <a href=/zoffsetup\><button class="button button1">&nbsp;Up&nbsp;</button></a>
    <a href=/><button class="button button2">Down</button></a>
  </p>
  <p>
    <i class="fas fa-tachometer-alt" style="color:#00add6;"></i>
    <!-- <span class="dht-labels">Rpm</span> -->
    <span id="rpm">%RPM%</span>
    <sup class="units">rpm</sup>
  </p>
    <p>
    <i class="fas fa-tachometer-alt" style="color:#00add6;"></i>
    <!-- <span class="dht-labels">Totalrevcount</span> -->
    <span id="totalrevcount">%TOTALREVCOUNT%</span>
    <sup class="units">total</sup>
  </p>
  <p>
  <a href=/reset\><button class="button button3">&nbsp;Reset&nbsp;</button></a>
  </p>
  <h4>FootPod</h4>

</body>
<script>
setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("speed").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/speed", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distance", true);
  xhttp.send();
}, 1000 ) ;

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("elevation").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/elevation", true);
  xhttp.send();
}, 1000 ) ;

</script>
</html>)rawliteral";


void InitAsync_Webserver()
{

  // Route for root / web page  This function is the main page,
  // as well as the Zoffsetdown button call function

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    //readZoffsetdown();
    request->send_P(200, "text/html", index_html);
  });

  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    char speed[8];
    snprintf(speed, 8, "%.2f", kmph);
    request->send_P(200, "text/plain", speed);
  });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
    char dist[8];
    snprintf(dist, 8, "%d", total_distance);
    request->send_P(200, "text/plain", dist);
  });

  server.on("/incline", HTTP_GET, [](AsyncWebServerRequest *request) {
    char inc[8];
    snprintf(inc, 8, "%.2f", incline);
    request->send_P(200, "text/plain", inc);
  });

  server.on("/elevation", HTTP_GET, [](AsyncWebServerRequest *request) {
    char ele[8];
    snprintf(ele, 8, "%.2f", elevation_gain);
    request->send_P(200, "text/plain", ele);
  });

  // server.on("/zoffset", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readZoffset().c_str());
  // });
  // server.on("/totalrevcount", HTTP_GET, [](AsyncWebServerRequest *request){
  //   request->send_P(200, "text/plain", readTotalrevcount().c_str());
  // });
  // // Send a GET request to <IP>/sensor/<number>/action/<action>
  // server.on("/zoffsetup", HTTP_GET, [] (AsyncWebServerRequest *request) {
  //   readZoffsetup();
  //   request->send_P(200, "text/html", index_html, processor);
  // });


  // Start server
  server.begin();
}

