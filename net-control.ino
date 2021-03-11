
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



// Replaces placeholder with DHT values
String processor(const String& var){
  if(var == "HOUR"){
    return readHour();
  }
  else if(var == "MINUTE"){
    return readMinute();
  }
  else if(var == "SECOND"){
    return readSecond();
  }
  return String();
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
    .button_start        { background-color: #4CAF50; } /* Green */
    .button_stop         { background-color: #cc0000; } /* Red   */
    .button_reset        { background-color: #008CBA; } /* Blue  */

    .button_speed_up     { background-color: #cc0000; } /* Red   */
    .button_incline_up   { background-color: #cc0000; } /* Red   */
    .button_speed_down   { background-color: #4CAF50; } /* Green */
    .button_incline_down { background-color: #4CAF50; } /* Green */
    .button_int1         { background-color: #00C8DC; } /* Light-Blue  */
    .button_int2         { background-color: #008CBA; } /* Blue  */
    .button_int3         { background-color: #008CBA; } /* Blue  */

  </style>
</head>
<body>
  <div>
    <div class="speed-dist">
      <p>
        <i class="fas fa-running" style="color:#059e8a;"></i>
        <span id="speed">n/a</span>
        <sup class="units">kph</sup>
      </p>
      <p>
        <i class="fas fa-shoe-prints" style="color:#2d0000;"></i>
        &nbsp;&nbsp;
        <span id="distance">n/a</span>
        <sup class="units">KM</sup>
      </p>
    </div>
    <div class="incline-elevation">
      <p>
        <i class="fas fa-tachometer-alt" style="color:#00add6;"></i>
        <span id="incline">%INCLINE%</span>
        <sup class="units">%</sup>
      </p>
      <p>
        <i class="fas fa-mountain" style="color:#00add6;"></i>
        <span id="elevation">%ELEVATION%</span>
        <sup class="units">M</sup>
      </p>
    </div>
  </div>

  <p>
    SPEED:
    <button id="speed_up" class="button button_speed_up" onclick="buttonclick(this);">
      &nbsp;UP&nbsp;
    </button>
    <button id="speed_down" class="button button_speed_down" onclick="buttonclick(this);">
      DOWN
    </button>
  </p>
  <p>
    INCLINE:
    <button id="incline_up" class="button button_incline_up" onclick="buttonclick(this);">
      &nbsp;UP&nbsp;
    </button>
    <button id="incline_down" class="button button_incline_down" onclick="buttonclick(this);">
      DOWN
    </button>
  </p>
  <p>
    <button id="interval_01" class="button button_int1" onclick="buttonclick(this);">0.1</button>
    <button id="interval_05" class="button button_int2" onclick="buttonclick(this);">0.5</button>
    <button id="interval_10" class="button button_int3" onclick="buttonclick(this);">1.0</button>
  </p>
  <p>
   <i class="fas fa-stopwatch" style="color:#059e8a;"></i>
    <!-- <span class="dht-labels">Time</span> -->
    <span id="hour">%HOUR%</span>
    :
    <span id="minute">%MINUTE%</span>
    :
    <span id="second">%SECOND%</span>
    <br>
    <a href="/start"><button class="button button_start">START</button></a>
    <a href="/stop" ><button class="button button_stop" >STOP</button></a>
    <a href="/reset"><button class="button button_reset">RESET</button></a>
  </p>
  <hr>
  <footer>
    <address>
      <a href="https://github.com/lefty01/ESP32_TTGO_FTMS">ESP32 Treadmill Sensor</a>
    </address>
  </footer>


</body>

<script>

function buttonclick(e) {
  console.log(e.id);

  if (e.id === "speed_up") {
    fetch("/speed/up", {
      method: 'PUT'
    });
  }
  if (e.id === "speed_down") {
    fetch("/speed/down", {
      method: 'PUT'
    });
  }
  if (e.id === "incline_up") {
    fetch("/incline/up", {
      method: 'PUT'
    });
  }
  if (e.id === "incline_down") {
    fetch("/incline/down", {
      method: 'PUT'
    });
  }

  if (e.id === "interval_01") {
    document.getElementById("interval_01").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    fetch("/speed/intervall/1", {
      method: 'PUT'
    });
  }
  if (e.id === "interval_05") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#00C8DC";
    document.getElementById("interval_10").style.backgroundColor = "#008CBA";
    fetch("/speed/intervall/2", {
      method: 'PUT'
    });
  }
  if (e.id === "interval_10") {
    document.getElementById("interval_01").style.backgroundColor = "#008CBA";
    document.getElementById("interval_05").style.backgroundColor = "#008CBA";
    document.getElementById("interval_10").style.backgroundColor = "#00C8DC";
    fetch("/speed/intervall/3", {
      method: 'PUT'
    });
  }
}

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("speed").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/speed", true);
  xhttp.send();
}, 1000);

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("distance").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/distance", true);
  xhttp.send();
}, 1000);

setInterval(function ( ) {
  var xhttp = new XMLHttpRequest();
  xhttp.onreadystatechange = function() {
    if (this.readyState == 4 && this.status == 200) {
      document.getElementById("incline").innerHTML = this.responseText;
    }
  };
  xhttp.open("GET", "/incline", true);
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
}, 1000);

</script>
</html>)rawliteral";


void InitAsync_Webserver()
{

  // Route for root / web page  This function is the main page,
  // as well as the Zoffsetdown button call function

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send_P(200, "text/html", index_html);
  });

  server.on("/speed", HTTP_GET, [](AsyncWebServerRequest *request) {
    char speed[8];
    snprintf(speed, 8, "%.2f", kmph);
    request->send_P(200, "text/plain", speed);
  });

  server.on("/distance", HTTP_GET, [](AsyncWebServerRequest *request) {
    char dist[8];
    snprintf(dist, 8, "%.2f", total_distance/1000);
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

  // PUT requests to set speed and incline (not on the machine)
  server.on("/speed/up", HTTP_PUT, [](AsyncWebServerRequest *request) {
    speed_up();
    request->send_P(200, "text/html", index_html);
  });
  server.on("/speed/down", HTTP_PUT, [](AsyncWebServerRequest *request) {
    speed_down();
    request->send_P(200, "text/html", index_html);
  });
  server.on("/incline/up", HTTP_PUT, [](AsyncWebServerRequest *request) {
    incline_up();
    request->send_P(200, "text/html", index_html);
  });
  server.on("/incline/down", HTTP_PUT, [](AsyncWebServerRequest *request) {
    incline_down();
    request->send_P(200, "text/html", index_html);
  });

  server.on("^\\/speed\\/intervall\\/([0-9]+)$",
		     HTTP_PUT, [](AsyncWebServerRequest *request) {
    String interval = request->pathArg(0);
    set_speed_interval(interval.toInt());
    request->send_P(200, "text/html", index_html);
  });


  // timer: start, stop, reset
  server.on("/reset", HTTP_GET, [] (AsyncWebServerRequest *request) {
    do_reset();
    request->redirect("/");
  });

  // Start server
  server.begin();
}

