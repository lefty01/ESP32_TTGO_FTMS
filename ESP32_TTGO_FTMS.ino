/**
 * 
 * 
 * The MIT License (MIT)
 * Copyright © 2021 <Andreas Loeffler>
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of this software
 * and associated documentation files (the “Software”), to deal in the Software without
 * restriction, including without limitation the rights to use, copy, modify, merge, publish,
 * distribute, sublicense, and/or sell copies of the Software, and to permit persons to whom the
 * Software is furnished to do so, subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all copies or
 * substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR IMPLIED, INCLUDING
 * BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM,
 * DAMAGES OR OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING
 * FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */

// initially started with the sketch from:
// https://hackaday.io/project/175237-add-bluetooth-to-treadmill

#include <Arduino.h>
//#include <ArduinoOTA.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <ArduinoJson.h>
#include <SPIFFS.h>
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // notify
#include <math.h>
#include <SPI.h>

#define TTGO_T_DISPLAY
#include <TFT_eSPI.h>
#include <Rotary.h>
#include <Button2.h>
#include <TimeLib.h>  // https://playground.arduino.cc/Code/Time/
#include <Wire.h>
#include <MPU6050_light.h> // accelerometer and gyroscope -> measure incline
#include <VL53L0X.h>       // time-of-flight sensor -> get incline % from distance to ground

#define VERSION "0.0.6F"
#define MQTTDEVICEID "ESP32_FTMS1"

// GAP  stands for Generic Access Profile
// GATT stands for Generic Attribute Profile defines the format of the data exposed
// by a BLE device. It also defines the procedures needed to access the data exposed by a device.
// Characteristics: a Characteristic is always part of a Service and it represents
// a piece of information/data that a Server wants to expose to a client. For example,
// the Battery Level Characteristic represents the remaining power level of a battery
// in a device which can be read by a Client.

// Where a characteristic can be notified or indicated, a Client Characteristic Configuration
// descriptor shall be included in that characteristic as required by the Core Specification

// use Fitness Machine Service UUID: 0x1826 (server)
// with Treadmill Data Characteristic 0x2acd

// gy-521-mpu-6050: https://de.aliexpress.com/item/32340949017.html
// http://cool-web.de/esp8266-esp32/gy-521-mpu-6050-gyroskop-beschleunigung-sensor-i2c-esp32-odroid-go.htm

// Zwift Forum Posts
// https://forums.zwift.com/t/show-us-your-zwift-setup/59647/19 

// embedded webserver status &  (manual) control
// https://hackaday.io/project/175237-add-bluetooth-to-treadmill



#define DEBUG 1
//#define DEBUG_MQTT 1
#include "debug_print.h"
#include "wifi_mqtt_creds.h"

// todo: do we get this from tft.width() and tft.height()?
#define TFT_WIDTH  240
#define TFT_HEIGHT 128
// -> defined in Setup25_TTGO_T_Display.h (via User_Setup.h)



#define ADC_EN          14  //ADC_EN is the ADC detection enable port
//  -> does this interfere with input/pullup???
#define ADC_PIN         34


// start / stop button for timer ...
// #define BUTTON_1        33 // do (data) reset
// #define BUTTON_2        25
// #define BUTTON_3        26
//#define BUTTON_4        27 // toggle sensor(auto) and manual mode
#define BUTTON_1        35
#define BUTTON_2        0


//Button2 btn1(BUTTON_1);
//Button2 btn2(BUTTON_2);


// #define ENC_BUTTON_PUSH 15
// #define ENC_BUTTON_A    37
// #define ENC_BUTTON_B    17

#define SPEED_SENSOR1   12
#define SPEED_SENSOR2   13
#define SPEED_SENSE_PIN 37


void updateDisplay(bool clear);
void initAsyncWebserver();

volatile unsigned long t1;
volatile unsigned long t2;
volatile boolean t1_valid = false;
volatile boolean t2_valid = false;
volatile boolean set_speed = true; // speed or incline via rotary encoder
//volatile boolean manual_speed_incline = true;//false
//volatile boolean manual_speed   = false;
//volatile boolean manual_incline = false;



//fixme: once and for good ... camle vs. underscore

// fixme: stick with one integer type ... i.e. use uint32_t instead od unsigned long
enum SensorFlags { MANUAL = 0, SPEED = 1, INCLINE = 2, _NUM_MODES_ = 4};
uint8_t speedInclineMode = MANUAL;
boolean hasMPU6050 = false;
boolean hasVL53L0X = false;



#define EVERY_SECOND 1000
#define WIFI_CHECK   30 * EVERY_SECOND
unsigned long sw_timer_clock = 0;
unsigned long wifi_reconnect_timer = 0;
unsigned long wifi_reconnect_counter = 0;

// my treadmill stats:
// Taurus 9.5:
// Geschwindigkeit: 0.5 - 22 km/h (increments 0.1 km/h)
// Steigung:        0   - 15 %    (increments 1 %)
const float max_speed   = 22.0;
const float min_speed   =  0.5;
const float max_incline = 15.0;
const float min_incline =  0;
const float speed_interval_10 = 1.0;
const float speed_interval_05 = 0.5;
const float speed_interval_01 = 0.1;
const float incline_interval  = 1.0;
volatile float speed_interval = speed_interval_01;
volatile unsigned long usAvg[8];

// ttgo tft: 33, 25, 26, 27
// the number of the pushbutton pins
//const int buttonPin[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
//const int ledPin =  13;      // the number of the LED pin
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
// Button2 btn3(BUTTON_3);
// Button2 btn4(BUTTON_4);
// Button2 encBtnP(ENC_BUTTON_PUSH);
// Rotary rotary = Rotary(ENC_BUTTON_A, ENC_BUTTON_B);


uint16_t    inst_speed;
uint16_t    inst_incline;
uint16_t    inst_grade;
uint8_t     inst_cadence = 1;                 /* Instantaneous Cadence. */
uint16_t    inst_stride_length = 1;           /* Instantaneous Stride Length. */
uint16_t    inst_elevation_gain = 0;
double      total_distance = 0; // m
double      elevation_gain = 0; // m
double      elevation;

float kmph; // kilometer per hour
float kmph_sense;
float mps;  // meter per second
double angle;
double grade_deg;

float incline;

bool bleClientConnected = false;
bool bleClientConnectedPrev = false;

TFT_eSPI tft = TFT_eSPI();
VL53L0X sensor;
const unsigned long MAX_DISTANCE = 1000;  // Maximum distance in mm


bool isWifiAvailable = false;
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

MPU6050 mpu(Wire);


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
					       BLECharacteristic::PROPERTY_NOTIFY);

BLECharacteristic FitnessMachineFeatureCharacteristic(BLEUUID((uint16_t)0x2ACC),
						      BLECharacteristic::PROPERTY_READ);

BLEDescriptor TreadmillDescriptor(BLEUUID((uint16_t)0x2901));

// seems kind of a standard callback function
class MyServerCallbacks : public BLEServerCallbacks {
  void onConnect(BLEServer* pServer) {
    bleClientConnected = true;
  };

  void onDisconnect(BLEServer* pServer) {
    bleClientConnected = false;
  }
};


void initBLE() {
  BLEDevice::init(MQTTDEVICEID); // set server name (here: MQTTDEVICEID)
  // create BLE Server, set callback for connect/disconnect
  pServer = BLEDevice::createServer();
  pServer->setCallbacks(new MyServerCallbacks());

  // create the fitness machine service within the server
  BLEService *pService = pServer->createService(FTMSService);

  pService->addCharacteristic(&TreadmillDataCharacteristics);
  TreadmillDescriptor.setValue("Treadmill Descriptor Value ABC");
  TreadmillDataCharacteristics.addDescriptor(&TreadmillDescriptor);
  TreadmillDataCharacteristics.addDescriptor(new BLE2902());

  pService->addCharacteristic(&FitnessMachineFeatureCharacteristic);
  FitnessMachineFeatureCharacteristic.addDescriptor(new BLE2902());
  //pFeature->addDescriptor(new BLE2902());

  // start service
  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponse(true);  
  pAdvertising->addServiceUUID(FTMSService);
  //pAdvertising->setMinPreferred(0x06);  // set value to 0x00 to not advertise this parameter
  
  pAdvertising->start();

}


void initSPIFFS() {
  if (!SPIFFS.begin()) {
    DEBUG_PRINTLN("Cannot mount SPIFFS volume...");
    while (1) {
      bool on = millis() % 200 < 50;//onboard_led.on = millis() % 200 < 50;
      // onboard_led.update();
      if (on)
	tft.fillScreen(TFT_RED);
      else
	tft.fillScreen(TFT_BLACK);
    }
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initSPIFFS Done!");
  delay(1000);
}


void doReset()
{
  setTime(0,0,0,0,0,0);
  total_distance = 0;
  elevation_gain = 0;
  kmph = 0.5;
  incline = 0;

  // calibrate

}


void buttonInit()
{
  // button 1 (GPIO 0) control auto/manual mode and reset timers
  btn1.setTapHandler([](Button2 & b) {
    unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN("Button 1 TapHandler");
    if (time > 3000) { // > 3sec enters config menu
      DEBUG_PRINTLN("RESET timer/counter!");
      doReset();
    }
    else if (time > 600) {
      DEBUG_PRINTLN("Button 1 long click...");
      // reset to manual mode
      speedInclineMode = 0;
      DEBUG_PRINT("speedInclineMode=");
      DEBUG_PRINTLN(speedInclineMode);
      showSpeedInclineMode(speedInclineMode);
    }
    else { // button1 short click toggle speed/incline mode
      DEBUG_PRINTLN("Button 1 short click...");
      speedInclineMode += 1;
      speedInclineMode %= _NUM_MODES_;
      DEBUG_PRINT("speedInclineMode=");
      DEBUG_PRINTLN(speedInclineMode);
      showSpeedInclineMode(speedInclineMode);
    }

  });

  // for testing purpose ... no pratical way to change spee/incline
  // short  click = up
  // longer click = down
  btn2.setTapHandler([](Button2& b) {
    unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN("Button 2 TapHandler");
    if (time > 3000) { // > 3sec enters config menu
      //DEBUG_PRINTLN("very long (>3s) click ... do nothing");
    }
    else if (time > 500) {
      DEBUG_PRINTLN("long (>500ms) click...");
      if ((speedInclineMode & SPEED) == 0)
	speedDown();
      if ((speedInclineMode & INCLINE) == 0)
	inclineDown();
    }
    else {
      DEBUG_PRINTLN("short click...");
      if ((speedInclineMode & SPEED) == 0)
	speedUp();

      if ((speedInclineMode & INCLINE) == 0)
	inclineUp();
    }
  });


  // btn3.setPressedHandler([](Button2 & b) {
  //   DEBUG_PRINT("Toggle Speed Sensor Auto/Manual: ");
  //   manual_speed = !manual_speed;
  //   if (manual_speed) {
  //     tft.fillCircle(210, 11, 9, TFT_RED);
  //   }
  //   else {
  //     tft.fillCircle(210, 11, 9, TFT_GREEN);
  //   }
  //   DEBUG_PRINTLN(manual_speed);
  // });
}

void buttonLoop()
{
    btn1.loop();
    btn2.loop();
    // btn3.loop();
    // btn4.loop();
    // encBtnP.loop();
}

void  speedSensor1_ISR() {
  if (! t1_valid) {
    t1 = micros();
    t1_valid = true;
  }
}

void  speedSensor2_ISR() {
  if (t1_valid && !t2_valid) {
    t2 = micros();
    t2_valid = true;
  }
}

void speedUp()
{
  if (speedInclineMode & SPEED)
    return;

  kmph += speed_interval;
  if (kmph > max_speed) kmph = max_speed;
  DEBUG_PRINT("speed_up, new speed: ");
  DEBUG_PRINTLN(kmph);
}

void speedDown()
{
  if (speedInclineMode & SPEED)
    return;

  kmph -= speed_interval;
  if (kmph < min_speed) kmph = min_speed;
  DEBUG_PRINT("speed_down, new speed: ");
  DEBUG_PRINTLN(kmph);
}

void inclineUp()
{
  if (speedInclineMode & INCLINE)
    return;

  incline += incline_interval; // incline in %
  if (incline > max_incline) incline = max_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_up, new incline: ");
  DEBUG_PRINTLN(incline);
}

void inclineDown()
{
  if (speedInclineMode & INCLINE)
    return;

  incline -= incline_interval;
  if (incline <= min_incline) incline = min_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_down, new incline: ");
  DEBUG_PRINTLN(incline);
}

float getIncline() {
  // if (hasVL53L0X) {}
  // if (hasMPU6050) {}

  // mpu.getAngle[XYZ]
  float y = mpu.getAngleY();
  DEBUG_PRINT("sensor angle (Y): ");
  DEBUG_PRINTLN(y);

  incline = tan(y / RAD_2_DEG) * 100;
  if (incline <= min_incline) incline = min_incline;
  if (incline > max_incline)  incline = max_incline;

  DEBUG_PRINTLN(incline);
  // probably need some more smoothing here ...
  // ...
  /*
  if (incline < .. && incline > ...) return 0;
  if (incline < .. && incline > ...) return 1;
  if (incline < .. && incline > ...) return 2;
  ...
  if (incline < .. && incline > ...) return 14;
  if (incline < .. && incline > ...) return 15;
  */
  
  return incline;
}

void setSpeed(float speed)
{
  kmph = speed;
  if (speed > max_speed) kmph = max_speed;
  if (speed < min_speed) kmph = min_speed;

  DEBUG_PRINT("setSpeed: ");
  DEBUG_PRINTLN(kmph);
}

void setSpeedInterval(float interval)
{
  DEBUG_PRINT("set_speed_interval: ");
  DEBUG_PRINTLN(interval);

  if ((interval < 0.1) || (interval > 2.0)) {
    DEBUG_PRINTLN("INVALID SPEED INTERVAL");
  }
  // switch(i) {
  // case 1: speed_interval = speed_interval_01; break;
  // case 2: speed_interval = speed_interval_05; break;
  // case 3: speed_interval = speed_interval_10; break;
  // default:
  //   DEBUG_PRINTLN("INVALID SPEED INTERVAL");
  //   break;
  // }
  speed_interval = interval;
}

String readSpeed()
{
  //char speed[8];
  //snprintf(speed, 8, "%.2f", kmph);
  return String(kmph);
}

String readDist()
{
  // char dist[8];
  // snprintf(dist, 8, "%.2f", total_distance/1000);
  return String(total_distance / 1000);
}

String readIncline()
{
  return String(incline);
}

String readElevation()
{
  return String(elevation_gain);
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


void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("setup started");

  // initial min treadmill speed
  kmph = 0.5;
  incline = 0;
  grade_deg = 0;
  angle = 0;
  elevation = 0;
  elevation_gain = 0;

  buttonInit();

  tft.begin();
  tft.setRotation(1); // 3
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");

  // pinMode(ENC_BUTTON_A, INPUT_PULLUP); // does not work right, need ext. pull-up (here 10k)
  // pinMode(ENC_BUTTON_B, INPUT_PULLUP);

  attachInterrupt(SPEED_SENSOR1, speedSensor1_ISR, FALLING);
  attachInterrupt(SPEED_SENSOR2, speedSensor2_ISR, FALLING);

  Wire.begin();

  initBLE();
  
  initSPIFFS();

  isWifiAvailable = setupWifi() ? false : true;

  if (isWifiAvailable) {
    DEBUG_PRINTLN("Init Webserver");
    initAsyncWebserver();
    initWebSocket();
  }
  //else show offline msg, halt or reboot?!

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_RED);
  tft.setCursor(20, 40);
  
  byte status = mpu.begin();
  DEBUG_PRINT("MPU6050 status: ");
  DEBUG_PRINTLN(status);
  if (status != 0) {
    tft.println("MPU6050 setup failed!");
    tft.println(status);
    delay(3000);
  }
  else {
    DEBUG_PRINTLN(F("Calculating offsets, do not move MPU6050"));
    tft.println("Calculating offsets, do not move MPU6050 (3sec)");
    mpu.calcOffsets(); // gyro and accel.
    delay(3000);
    speedInclineMode = 3;
    hasMPU6050 = true;
  }

  sensor.setTimeout(500);
  if (!sensor.init()) {
    DEBUG_PRINTLN("Failed to detect and initialize VL53L0X sensor!");
    tft.println("VL53L0X setup failed!");
    delay(3000);
  }
  else {
    DEBUG_PRINTLN("VL53L0X sensor detected and initialized!");
    tft.println("VL53L0X initialized!");
    hasVL53L0X = true;
    delay(3000);
  }

  // tft.fillScreen(TFT_BLACK);
  // tft.print("speedInclineMode: ");
  // tft.println(speedInclineMode);
  // delay(3000);

  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Done");
  tft.print("Version:");
  tft.println(VERSION);
  DEBUG_PRINTLN("setup done");

  delay(3000);
  updateDisplay(true);
  // indicate bt connection status ... offline
  tft.fillCircle(229, 11, 9, TFT_BLACK);
  tft.drawCircle(229, 11, 9, TFT_SKYBLUE);

  // indicate manual/auto mode (green=auto/sensor, red=manual)
  //tft.fillCircle(210, 11, 9, TFT_RED);
  showSpeedInclineMode(speedInclineMode);

  setTime(0,0,0,0,0,0);
}

void loop() {
  buttonLoop();
  mpu.update();
  //uint8_t result = rotary.process();

  // re-connect to wifi
  if ((WiFi.status() != WL_CONNECTED) && ((millis() - wifi_reconnect_timer) > WIFI_CHECK)) {
    wifi_reconnect_timer = millis();
    isWifiAvailable = false;
    DEBUG_PRINTLN("Reconnecting to WiFi...");
    WiFi.disconnect();
    WiFi.reconnect();
  }
  if (!isWifiAvailable && (WiFi.status() == WL_CONNECTED)) {
    // connection was lost and now got reconnected ...
    isWifiAvailable = true;
    wifi_reconnect_counter++;
  }


  // if changed to connected ...
  if (bleClientConnected && !bleClientConnectedPrev) {
    bleClientConnectedPrev = true;
    DEBUG_PRINTLN("BT Client connected!");
    tft.fillCircle(229, 11, 9, TFT_SKYBLUE);
  }
  else if (!bleClientConnected && bleClientConnectedPrev) {
    bleClientConnectedPrev = false;
    DEBUG_PRINTLN("BT Client disconnected!");
    tft.fillCircle(229, 11, 9, TFT_BLACK);
    tft.drawCircle(229, 11, 9, TFT_SKYBLUE);
  }

  // // manual speed/inclune settings via rotary ecoder switch
  // if (result == DIR_CW) {
  //   if (set_speed) {
  //     speedUp();
  //   }
  //   else {
  //     inclineUp();
  //   }
  // }
  // else if (result == DIR_CCW) {
  //   if (set_speed) {
  //     speedDown();
  //   }
  //   else {
  //     inclineDown();
  //   }
  // }

  // check ir-speed sensor if not manual mode
  if (t2_valid) {
    unsigned long t = t2 - t1;
    unsigned long c = 359712; // d=10cm
    kmph_sense = (float)(1.0/t) * c;
    noInterrupts();
    t1_valid = t2_valid = false;
    interrupts();
    DEBUG_PRINTLN(t);
    DEBUG_PRINTLN(kmph_sense);
  }

  // testing ... every second
  if ((millis() - sw_timer_clock) > EVERY_SECOND) {
    sw_timer_clock = millis();

    // show reconnect counter in tft
    // if (wifi_reconnect_counter > wifi_reconnect_counter_prev) ... only update on change
    tft.fillRect(2, 2, 60, 18, TFT_BLACK);
    tft.setTextColor(TFT_WHITE);
    tft.setTextFont(2);
    tft.setCursor(3, 4);
    tft.print(wifi_reconnect_counter);

    if (speedInclineMode & INCLINE) {
      incline = getIncline();
    }

    // total_distance = ... v = d/t -> d = v*t -> use v[m/s]
    if (speedInclineMode & SPEED) kmph = kmph_sense;
    mps = kmph / 3.6;
    total_distance += mps;
    elevation_gain += (sin(angle) * mps);

    DEBUG_PRINT("mps = d:    ");DEBUG_PRINTLN(mps);
    DEBUG_PRINT("angle:      ");DEBUG_PRINTLN(angle);
    DEBUG_PRINT("h (m):      ");DEBUG_PRINTLN(sin(angle) * mps);
    
    DEBUG_PRINT("h gain (m): ");DEBUG_PRINTLN(elevation_gain);
    
    DEBUG_PRINT("kmph:    "); DEBUG_PRINTLN(kmph);
    DEBUG_PRINT("incline: "); DEBUG_PRINTLN(incline);
    DEBUG_PRINT("angle:   "); DEBUG_PRINTLN(grade_deg);
    DEBUG_PRINT("dist km: "); DEBUG_PRINTLN(total_distance/1000);
    DEBUG_PRINT("ele  m:  "); DEBUG_PRINTLN(elevation_gain);

    updateDisplay(false);
    notifyClients();

    uint8_t treadmillData[34] = {};
    uint16_t flags = 0x0018;  // b'000000011000
    //                             119876543210
    //                             20
    // get speed and incline ble ready
    inst_speed   = kmph * 100;    // kilometer per hour with a resolution of 0.01
    inst_incline = incline * 10;  // percent with a resolution of 0.1
    inst_grade   = grade_deg * 10;
    inst_elevation_gain = elevation_gain * 10;

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
}
