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

#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // notify
#include <math.h>
#include <SPI.h>
#include <TFT_eSPI.h>
#include <Rotary.h>
#include <Button2.h>
#include <TimeLib.h>  //https://playground.arduino.cc/Code/Time/

#define VERSION "0.0.1"
#define MQTTDEVICEID "ESP32_FTMS1"

// note: this version currently only allows manual setting of speed and incline
//       via the push-button rotary encoder (testing purpose)

// GAP  stands for Generic Access Profile
// GATT stands for Generic Attribute Profile defines the format of the data exposed
// by a BLE device. It also defines the procedures needed to access the data exposed by a device.
// Characteristics: a Characteristic is always part of a Service and it represents
// a piece of information/data that a Server wants to expose to a client. For example,
// the Battery Level Characteristic represents the remaining power level of a battery
// in a device which can be read by a Client.

// Where a characteristic can be notified or indicated, a Client Characteristic Configuration descriptor shall
// be included in that characteristic as required by the Core Specification

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

#define ADC_EN          14  //ADC_EN is the ADC detection enable port
//  -> does this interfere with input/pullup???
#define ADC_PIN         34


// start / stop button for timer ...
#define BUTTON_1        33
#define BUTTON_2        25
#define BUTTON_3        26
#define BUTTON_4        27

#define ENC_BUTTON_PUSH 15
#define ENC_BUTTON_A    37
#define ENC_BUTTON_B    17

#define SPEED_SENSOR1   12
#define SPEED_SENSOR2   13

void updateDisplay(bool clear);
void InitAsync_Webserver();

volatile unsigned long t1;
volatile unsigned long t2;
volatile boolean t1_valid = false;
volatile boolean t2_valid = false;
volatile boolean set_speed = true;

#define EVERY_SECOND 1000
unsigned long sw_timer_clock;

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

// ttgo tft: 33, 25, 26, 27
// the number of the pushbutton pins
//const int buttonPin[] = {BUTTON_1, BUTTON_2, BUTTON_3, BUTTON_4};
//const int ledPin =  13;      // the number of the LED pin
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);
Button2 btn3(BUTTON_3);
Button2 btn4(BUTTON_4);
Button2 encBtnP(ENC_BUTTON_PUSH);
Rotary rotary = Rotary(ENC_BUTTON_A, ENC_BUTTON_B);


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
float mps;  // meter per second
double angle;
double grade_deg;

float incline;

bool bleClientConnected = false;
bool bleClientConnectedPrev = false;

TFT_eSPI tft = TFT_eSPI();

bool isWifiAvailable = false;
AsyncWebServer server(80);

String readHour() {
    return String(hour());
}
String readMinute() {
    return String(minute());
}
String readSecond() {
    return String(second());
}

// note: Fitness Machine Feature is a mandatory characteristic (property_read)
#define FTMSService BLEUUID((uint16_t)0x1826)
//#define TreadmillDataCharacterisic          BLEUUID((uint16_t)0x2ACD) // opt       - notify
//#define FitnessMachineFeatureCharacteristic BLEUUID((uint16_t)0x2ACC) // mandatory - read
//#define FitnessMachineStatusCharacterisic   BLEUUID((uint16_t)0x2ADA) // mandatory if control point supported - notify
//#define SupportedSpeedRangeCharacteristic   BLEUUID((uint16_t)0x2AD4) // 
//#define SupportedInclineRangeCharacteristic BLEUUID((uint16_t)0x2AD5) // 
//#define ControlPointCharacterisic           BLEUUID((uint16_t)0x2AD9) // opt       - write/indicate


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


void InitBLE() {
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

void do_reset()
{
  setTime(0,0,0,0,0,0);
  total_distance = 0;
  elevation_gain = 0;
  kmph = 0.5;
  incline = 0;
}


void buttonInit()
{
  // encoder push-button handler
  encBtnP.setTapHandler([](Button2 & b) {
    unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN(time);
    if (time > 3000) { // > 3sec enters config menu
      DEBUG_PRINTLN("very long click ... do nothing");
    }
    else if (time > 600) {
      DEBUG_PRINTLN("Encoder Button PUSH long click...");
      // longer click while in select menu exit and restore prev mode

    }
    else {
      set_speed = !set_speed; // toggle
      // clear upper status line
      tft.fillRect(2, 2, 200, 18, TFT_BLACK);
      tft.setTextColor(TFT_WHITE);
      tft.setTextFont(2);
      tft.setCursor(100, 4);
      if (set_speed) tft.print("SPEED");
      else           tft.print("INCLINE");

      DEBUG_PRINTLN("Button PUSH short click...");
      DEBUG_PRINTLN(set_speed);
    } // short push-button

  });

  btn1.setPressedHandler([](Button2 & b) {
    DEBUG_PRINTLN("Button 1 SHORT click...");
    total_distance = 0;
    elevation_gain = 0;
  });

  btn2.setPressedHandler([](Button2 & b) {
    DEBUG_PRINTLN("Button 2 SHORT click...");
    //b2();
  });

  btn3.setPressedHandler([](Button2 & b) {
    DEBUG_PRINTLN("Button 3 SHORT click...");
    //b3();
  });
  btn4.setPressedHandler([](Button2 & b) {
    DEBUG_PRINTLN("Button 4 SHORT click...");
    //b4();
  });

}
void buttonLoop()
{
    btn1.loop();
    btn2.loop();
    btn3.loop();
    btn4.loop();
    encBtnP.loop();
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

int8_t getIncline() {
  int8_t percent;

  percent = 7;

  return percent;
}

void speed_up()
{
  kmph += speed_interval;
  if (kmph > max_speed) kmph = max_speed;
  DEBUG_PRINT("speed_up, new speed: ");
  DEBUG_PRINTLN(kmph);
}
void speed_down()
{
  kmph -= speed_interval;
  if (kmph < min_speed) kmph = min_speed;
  DEBUG_PRINT("speed_down, new speed: ");
  DEBUG_PRINTLN(kmph);
}
void incline_up()
{
  incline += incline_interval;
  if (incline > max_incline) incline = max_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_up, new incline: ");
  DEBUG_PRINTLN(incline);
}
void incline_down()
{
  incline -= incline_interval;
  if (incline <= min_incline) incline = min_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_down, new incline: ");
  DEBUG_PRINTLN(incline);
}

void set_speed_interval(int i)
{
  DEBUG_PRINT("set_speed_interval: ");
  DEBUG_PRINTLN(i);
  switch(i) {
  case 1: speed_interval = speed_interval_01; break;
  case 2: speed_interval = speed_interval_05; break;
  case 3: speed_interval = speed_interval_10; break;
  default:
    DEBUG_PRINTLN("INVALID SPEED INTERVAL");
    break;
  }
}


void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("setup started");

  buttonInit();

  tft.begin();
  tft.setRotation(1); // 3
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");

  pinMode(ENC_BUTTON_A, INPUT_PULLUP); // does not work right, need ext. pull-up (here 10k)
  pinMode(ENC_BUTTON_B, INPUT_PULLUP);

  attachInterrupt(SPEED_SENSOR1, speedSensor1_ISR, FALLING);
  attachInterrupt(SPEED_SENSOR2, speedSensor2_ISR, FALLING);

  InitBLE();
  
  // initial min treadmill speed
  kmph = 0.5;
  incline = 0;
  grade_deg = 0;
  angle = 0;
  elevation = 0;
  elevation_gain = 0;

  isWifiAvailable = setupWifi() ? false : true;

  if (isWifiAvailable)
    InitAsync_Webserver();

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Done");
  tft.print("Version:");
  tft.println(VERSION);
  DEBUG_PRINTLN("setup done");

  delay(3000);
  updateDisplay(true);
}

void loop() {
  buttonLoop();
  unsigned char result = rotary.process();

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


  if (result == DIR_CW) {
    if (set_speed) {
      speed_up();
    }
    else {
      incline_up();
    }
  }
  else if (result == DIR_CCW) {
    if (set_speed) {
      speed_down();
    }
    else {
      incline_down();
    }
  }
  // if (t2_valid) {
  //   unsigned long t = t2 - t1;
  //   int c = 359712;
  //   kmph = (1/t) * c;
  //   DEBUG_PRINTLN(t);
  //   noInterrupts();
  //   t1_valid = t2_valid = false;
  //   interrupts();
  // }

  // testing ... every second
  if ((millis() - sw_timer_clock) > EVERY_SECOND) {
    sw_timer_clock = millis();

    //int8_t incline = getIncline();
    //uint8_t speed = getSpeed();
    // total_distance = ... v = d/t -> d = v*t -> use v[m/s]
    mps = kmph / 3.6;
    total_distance += mps;
    elevation_gain += (sin(angle) * mps);

    //    elevation += ...
    // sin(a) = h/dist -> with angle as a: h = dist * sin(angle)
    DEBUG_PRINTLN("elevation:");
    DEBUG_PRINT("mps = d:    ");DEBUG_PRINTLN(mps);
    DEBUG_PRINT("angle:      ");DEBUG_PRINTLN(angle);
    DEBUG_PRINT("h (m):      ");DEBUG_PRINTLN(sin(angle) * mps);
    
    DEBUG_PRINT("h gain (m): ");DEBUG_PRINTLN(elevation_gain);
    


    //    DEBUG_PRINT("loop: "); DEBUG_PRINTLN(millis());
    DEBUG_PRINT("kmph:    "); DEBUG_PRINTLN(kmph);
    DEBUG_PRINT("incline: "); DEBUG_PRINTLN(incline);
    DEBUG_PRINT("angle:   "); DEBUG_PRINTLN(grade_deg);
    DEBUG_PRINT("dist km: "); DEBUG_PRINTLN(total_distance/1000);
    DEBUG_PRINT("ele  m:  "); DEBUG_PRINTLN(elevation_gain);

    updateDisplay(false);


    // treadmill data fields:
    // 0,1    0: flags,		         16
    // 2,3    1: Instantaneous Speed,    16 Kilometer per hour with a resolution of 0.01
    // 4,5    2: avg speed	         16 Kilometer per hour with a resolution of 0.01
    // 6,7,8  3: Total Distance	         24 Meters
    
    //  9,10  4: Incline                 16 sign, Percent with a resolution of 0.1
    // 11,12  5: Ramp Angle Setting      16 sign, Degree  with a resolution of 0.1
    // 13,14  6: Positive Elevation Gain 16 Meters with a resolution of 0.1
    // 15,16  7: Negative Elevation Gain 16 Meters with a resolution of 0.1

    // 17     8: Instantaneous Pace       8 Kilometer per minute with a resolution of 0.1
    // 18     9: Average Pace	          8 Kilometer per minute with a resolution of 0.1
    // 19,20 10: Total Energy		 16
    // 21,22 11: Energy Per Hour	 16
    // 23    12: Energy Per Minute	  8
    // 24    13: Heart Rate		  8
    // 25    14: Metabolic Equivalent	  8
    // 26,27 15: Elapsed Time		 16 seconds
    // 28,29 16: Remaining Time	         16 seconds
    // 30,31 17: Force on Belt	         16 signed newton force
    // 32,33 18: Power Output		 16 signed watts
    // total bits = 272 = 34 bytes

    uint8_t treadmillData[34] = {};
    uint16_t flags = 0x0018;  // b'000000011000
    //                             119876543210
    //                             20
    // get speed and incline ble ready
    inst_speed   = kmph * 100;  // kilometer per hour with a resolution of 0.01
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
