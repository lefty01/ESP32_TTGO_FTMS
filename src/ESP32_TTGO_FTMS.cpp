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

#define NIMBLE

// initially started with the sketch from:
// https://hackaday.io/project/175237-add-bluetooth-to-treadmill

// FIXME: incline manual/auto toggle -> not same for incline and speed!!
// FIXME: if manual incline round up/down to integer
// FIXME: elevation gain
// FIXME: once and for good ... camle vs. underscore case
// TODO:  web config menu (+https, +setup initial user password)

#include "common.h"

#include <SPIFFS.h>
#include <Preferences.h>
#include <unity.h>

#ifndef NIMBLE
// original 
#include <BLEDevice.h>
#include <BLEUtils.h>
#include <BLEServer.h>
#include <BLE2902.h> // notify
#else
#include <NimBLEDevice.h>
#endif

#include <math.h>
#include <SPI.h>


#include <Button2.h>
#include <TimeLib.h>  // https://playground.arduino.cc/Code/Time/
#ifndef NO_MPU6050
#include <Wire.h>
#include <MPU6050_light.h> // accelerometer and gyroscope -> measure incline
#endif
#ifndef NO_VL53L0X
#include <VL53L0X.h>       // time-of-flight sensor -> get incline % from distance to ground
#endif



// Select and uncomment one of the Treadmills below
//#define TREADMILL_MODEL "TAURUS_9_5"
//#define TREADMILL_MODEL "NORDICTRACK_12SI"
// or via platformio.ini:
// -DTREADMILL_MODEL="TAURUS_9_5"


const char* VERSION = "0.0.18";




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

// Zwift Forum Posts
// https://forums.zwift.com/t/show-us-your-zwift-setup/59647/19 

// embedded webserver status &  (manual) control
// https://hackaday.io/project/175237-add-bluetooth-to-treadmill

// Parts Used:
// laufband geschwindigkeit sensor
// https://de.aliexpress.com/item/4000023371194.html?spm=a2g0s.9042311.0.0.556d4c4d8wMaUG
// IR Infrarot
// https://de.aliexpress.com/item/1005001285654366.html?spm=a2g0s.9042311.0.0.27424c4dPrwkYp
// GY-521 MPU-6050 MPU6050
// https://de.aliexpress.com/item/32340949017.html?spm=a2g0s.9042311.0.0.27424c4dPrwkYp
// GY-530 VL53L0X (ToF)
// https://de.aliexpress.com/item/32738458924.html?spm=a2g0s.9042311.0.0.556d4c4d8wMaUG


#define ADC_EN          14  //ADC_EN is the ADC detection enable port
//  -> does this interfere with input/pullup???
#define ADC_PIN         34

#ifdef TTGO_T_DISPLAY
// TTGO T-Display buttons
#define BUTTON_1        35
#define BUTTON_2        0
#else
#error Unknow button setup
#endif

// PINS 
// NOTE TTGO T-DISPAY GPIO 36,37,38,39 can only be input pins https://github.com/Xinyuan-LilyGO/TTGO-T-Display/issues/10

#define SPEED_IR_SENSOR1   15
#define SPEED_IR_SENSOR2   13
#define SPEED_REED_SWITCH_PIN 26 // REED-Contact

// These are used to control/override the Treadmil, e.g. pins are connected to
// the different button so that software can "press" them
// TODO: They could also be used to read pins 
#define TREADMILL_BUTTON_INC_DOWN_PIN 32
#define TREADMILL_BUTTON_INC_UP_PIN   33
#define TREADMILL_BUTTON_SPEED_DOWN_PIN 25
#define TREADMILL_BUTTON_SPEED_UP_PIN   27
#define TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS   250



// DEBUG0_PIN could point to an led you connect or read by a osciliscope or logical analyzer
// Uncomment this line to use it
//#define DEBUG0_PIN       17

#ifdef DEBUG0_PIN
volatile bool debug0State = LOW;
#endif



volatile unsigned long t1;
volatile unsigned long t2;
volatile boolean t1_valid = false;
volatile boolean t2_valid = false;


uint8_t speedInclineMode = SPEED;
boolean hasMPU6050 = false;
boolean hasVL53L0X = false;
boolean hasIrSense = false;
boolean hasReed    = false;

#define EVERY_SECOND 1000
#define WIFI_CHECK   30 * EVERY_SECOND
unsigned long sw_timer_clock = 0;
unsigned long wifi_reconnect_timer = 0;
unsigned long wifi_reconnect_counter = 0;


String MQTTDEVICEID = "ESP32_FTMS_";

uint8_t mac_addr[6];
Preferences prefs;
esp_reset_reason_t rr;

// treadmill stats
#ifndef TREADMILL_MODEL
  #error "***** ATTENTION NO TREADMILL MODEL DEFINED ******"
#elif TREADMILL_MODEL == TAURUS_9_5
// Taurus 9.5:
// Speed:    0.5 - 22 km/h (increments 0.1 km/h)
// Incline:  0..15 Levels  (increments 1 Level) -> 0-11 %
const float max_speed   = 22.0;
const float min_speed   =  0.5;
const float max_incline = 11.0;
const float min_incline =  0.0;
const float speed_interval_min = 0.1;
const float incline_interval_min  = 1.0;
const long  belt_distance = 250; // mm ... actually circumfence of motor wheel!

#elif TREADMILL_MODEL == NORDICTRACK_12SI
// Northtrack 12.2 Si:
// Geschwindigkeit: 0.5 - 20 km/h (increments 0.1 km/h)
// Steigung:        0   - 12 %    (increments .5 %)
// On Northtrack 12.2 Si there is a connector between "computer" and lower buttons (Speed+/-, Start/Stop, Incline +/-) with all cables from
// motor controll board MC2100ELS-18w together with all 6 buttons (Speed+/-, Start/Stop, Incline +/-)
// This seem like a nice place to interface the unit. This might be true from many more treadmills from differnt brands from Icon Healt & Fitness Inc
//
// Speed:
// Connect Tach. e.g. the Green cable 5v from MC2100ELS-18w to pin SPEED_REED_SWITCH_PIN in TTGO T-Display with a levelshifter 5v->3v in between seem to work quite ok
// This should probably work on most many (all?) treadmills using the motor controll board MC2100ELS-18w from Icon Healt & Fitness Inc
//
// Incline:
// If no MPU6050 is used (e.g. if you place the esp32 in the computer unit and don't want to place a long cable down to treadmill band)
// Incline steps from the compuer can be read like this. 
// Incline up is on the Orange cable 5v about a 2s puls is visible on ech step. Many steps cause a longer pulse.
// Incline down is on the Yellow cable 5v about a 2s puls is visible on ech step. Many steps cause a longer pulse
// Incline seem to cause Violet cable to puls 3-4 times during each step, maye this can be used to keep beter cound that dividing with 2s on the above?
//
// Control
// As for controling the treadmill connecting the cables to GROUND for abour 200ms on the four Speed+, Speed-, Incline+ and Incline- cables from the buttons
// in the connector seem to do the trick. I don't expect Start/Stop to be controlled but maybe we want to read them, lets see.
// Currently the scematighs is not worked out maybe they can just be connected via a levelshifter, or some sort of relay need to be used?
// If you during some start phase send a set of Incline- until you are sure the treadmill is at it's lowers position 
// it's problabe possible to keek track if current incline after this.

const float max_speed   = 20.0;
const float min_speed   =  0.5;
const float max_incline = 12.0;
const float min_incline =  0.0;
const float speed_interval_min = 0.1;
const float incline_interval_min  = 0.5;
const long  belt_distance = 153.3; // mm ... actually distance traveled of each tach from MCU1200els motor control board 
                                   // e.g. circumfence of front roler (calibrated with a distant wheel)
                                   // Accroding to manuel this should be 19 something, but I do not get this, could 19 be based on some imperial unit?

#else
  #error "Unexpected value for TREADMILL_MODEL defined!"
#endif



const float speed_interval_10 = 1.0;
const float speed_interval_05 = 0.5;
const float speed_interval_01 = speed_interval_min;
const float incline_interval  = incline_interval_min;

volatile float speed_interval = speed_interval_01;
volatile unsigned long usAvg[8];
volatile unsigned long startTime = 0;     // start of revolution in microseconds
volatile unsigned long longpauseTime = 0; // revolution time with no reed-switch interrupt
volatile long accumulatorInterval = 0;    // time sum between display during intervals
//volatile unsigned int Totalrevcount = 0;  // number of revolutions since last display update
volatile unsigned int revCount = 0;       // number of revolutions since last display update
volatile long accumulator4 = 0;           // sum of last 4 rpm times over 4 seconds
volatile long workoutDistance = 0; // FIXME: vs. total_dist ... select either reed/ir/manual


// ttgo tft: 33, 25, 26, 27
// the number of the pushbutton pins
//const int ledPin =  13;      // the number of the LED pin
Button2 btn1(BUTTON_1);
Button2 btn2(BUTTON_2);


uint16_t    inst_speed;
uint16_t    inst_incline;
uint16_t    inst_grade;
uint8_t     inst_cadence = 1;                 /* Instantaneous Cadence. */
uint16_t    inst_stride_length = 1;           /* Instantaneous Stride Length. */
uint16_t    inst_elevation_gain = 0;
double      total_distance = 0; // m
double      elevation_gain = 0; // m
double      elevation;

float rpm = 0;

float kmph; // kilometer per hour
float kmph_sense;
float mps;  // meter per second
double angle;
double grade_deg;

float incline;

bool bleClientConnected = false;
bool bleClientConnectedPrev = false;

#ifdef USE_TFT_ESPI
TFT_eSPI tft = TFT_eSPI();
#else
static LGFX tft;
#endif

#ifndef NO_VL53L0X
VL53L0X sensor;
const unsigned long MAX_DISTANCE = 1000;  // Maximum distance in mm
#endif

bool isWifiAvailable = false;
bool isMqttAvailable = false;
// todo: https and require initial user pass
AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

#ifndef NO_MPU6050
MPU6050 mpu(Wire);
#endif

#ifdef MQTT_USE_SSL
WiFiClientSecure espClient;
#else
WiFiClient espClient;
#endif
PubSubClient client(espClient);  // mqtt client

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
#ifndef NIMBLE
					       BLECharacteristic::PROPERTY_NOTIFY
#else
					       NIMBLE_PROPERTY::NOTIFY
#endif
					       );

BLECharacteristic FitnessMachineFeatureCharacteristic(BLEUUID((uint16_t)0x2ACC),
#ifndef NIMBLE
						      BLECharacteristic::PROPERTY_READ
#else
						      NIMBLE_PROPERTY::READ
#endif
						      );

BLEDescriptor TreadmillDescriptor(BLEUUID((uint16_t)0x2901)
#ifdef NIMBLE
				  , NIMBLE_PROPERTY::READ,1000
#endif
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


void initBLE() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initBLE");

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
#ifndef NIMBLE
  TreadmillDataCharacteristics.addDescriptor(new BLE2902());
  FitnessMachineFeatureCharacteristic.addDescriptor(new BLE2902());
  //pFeature->addDescriptor(new BLE2902());
#else
  /***************************************************
   NOTE: DO NOT create a 2902 descriptor. 
   it will be created automatically if notifications
   or indications are enabled on a characteristic.
  ****************************************************/
  //TreadmillDataCharacteristics.addDescriptor(new BLE2902());
  //FitnessMachineFeatureCharacteristic.addDescriptor(new BLE2902());

#endif
  // start service
  pService->start();
  pAdvertising = pServer->getAdvertising();
  pAdvertising->setScanResponse(true);  
  pAdvertising->addServiceUUID(FTMSService);
  //pAdvertising->setMinPreferred(0x06);  // set value to 0x00 to not advertise this parameter
  
  pAdvertising->start();
  delay(2000); // added to keep tft msg a bit longer ...
}


void initSPIFFS() {
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initSPIFFS");

  if (!SPIFFS.begin(true)) { // true = formatOnFail
    DEBUG_PRINTLN("Cannot mount SPIFFS volume...");

    tft.setTextColor(TFT_RED);
    tft.println("FAILED!!!");
    delay(5000);
    while (1) {
      bool on = millis() % 200 < 50;  // onboard_led.on = millis() % 200 < 50;
      // onboard_led.update();
      if (on)
        tft.fillScreen(TFT_RED);
      else
	tft.fillScreen(TFT_BLACK);
    }
  }
  else {
    DEBUG_PRINTLN("SPIFFS Setup Done");
  }
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initSPIFFS Done!");
  delay(2000);
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

void press_external_button_low(uint32_t pin, uint32_t time_ms) {
  digitalWrite(pin, LOW);
  pinMode(pin, OUTPUT);
  digitalWrite(pin, LOW);
  delay(time_ms);
  //digitalWrite(pin, HIGH);
  //delay(time_ms/2);
  //digitalWrite(pin, LOW);
  pinMode(pin, INPUT);
}


void buttonInit()
{
  // button 1 (GPIO 0) control auto/manual mode and reset timers
  btn1.setTapHandler([](Button2 & b) {
    unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN("Button 1 TapHandler");
    //press_external_button_low(TREADMILL_BUTTON_INC_DOWN_PIN,TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);
    if (time > 3000) { // > 3sec enters config menu
      DEBUG_PRINTLN("RESET timer/counter!");
      doReset();
    }
    else if (time > 600) {
      DEBUG_PRINTLN("Button 1 long click...");
      // reset to manual mode
      speedInclineMode = MANUAL;
      DEBUG_PRINT("speedInclineMode=");
      DEBUG_PRINTLN(speedInclineMode);
      showSpeedInclineMode(speedInclineMode);
      updateBTConnectionStatus(bleClientConnected);
      show_WIFI(wifi_reconnect_counter, getWifiIpAddr());
    }
    else { // button1 short click toggle speed/incline mode
      DEBUG_PRINTLN("Button 1 short click...");
      speedInclineMode += 1;
      speedInclineMode %= _NUM_MODES_;
      DEBUG_PRINT("speedInclineMode=");
      DEBUG_PRINTLN(speedInclineMode);
      showSpeedInclineMode(speedInclineMode);
      updateBTConnectionStatus(bleClientConnected);
      show_WIFI(wifi_reconnect_counter, getWifiIpAddr());
    }

  });

  // for testing purpose ... no pratical way to change spee/incline
  // short  click = up
  // longer click = down
  btn2.setTapHandler([](Button2& b) {
    unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN("Button 2 TapHandler");
    //press_external_button_low(TREADMILL_BUTTON_INC_UP_PIN,TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);
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
  //     tft.fillCircle(210, 11, 8, TFT_RED);
  //   }
  //   else {
  //     tft.fillCircle(210, 11, 8, TFT_GREEN);
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

#ifdef DEBUG0_PIN
// Safe to use from Interrupt code
void IRAM_ATTR showAndToggleDebug0_I() {
  digitalWrite(DEBUG0_PIN, debug0State);
  debug0State = !debug0State;
}

// Safe to use from Interrupt code
void IRAM_ATTR showDebug0_I(bool state) {
  digitalWrite(DEBUG0_PIN, state);
  debug0State = !state;
}
#endif

void IRAM_ATTR reedSwitch_ISR()
{
  // calculate the microseconds since the last interrupt.
  // Interupt activates on falling edge.
  uint32_t usNow = micros();
  uint32_t test_elapsed = usNow - startTime;

  // handle button bounce (ignore if less than 300 microseconds since last interupt)
  if (test_elapsed < 300) {
    return;
  }

  //if (test_elapsed > 1000000) {
  if (test_elapsed > 1000000) {  // assume the treadmill isn't spinning in 1 second.
    startTime = usNow; // reset the clock;
    longpauseTime = 0;
    return;
  }
  if (test_elapsed > longpauseTime / 2) {
    // acts as a debounce, don't looking for interupts soon after the first hit.
    //Serial.println(test_elapsed); //Serial.println(" Counted");

#ifdef DEBUG0_PIN
    showAndToggleDebug0_I();
#endif

    startTime = usNow;  // reset the clock
    //long elapsed = test_elapsed;
    longpauseTime = test_elapsed;

    revCount++;
    workoutDistance += belt_distance;
    //Totalrevcount++;
    accumulatorInterval += test_elapsed;
  }
}

// two IR-Sensors
void IRAM_ATTR speedSensor1_ISR() {
  if (! t1_valid) {
    t1 = micros();
    t1_valid = true;
  }
}

void IRAM_ATTR speedSensor2_ISR() {
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
  if (hasVL53L0X) {
    // calc incline/angle from distance
    // depends on sensor placement ... TODO: configure via webinterface
  }
  else if (hasMPU6050) {
#ifndef NO_MPU6050
    // TODO: configure sensor orientation  via webinterface

    // FIXME: maybe get some rolling-average of Y-angle to smooth things a bit (same for speed)
    // mpu.getAngle[XYZ]
    //float y = mpu.getAngleY();
    
    angle = mpu.getAngleY();
    if (angle < 0) angle = 0;
    char yStr[5];

    snprintf(yStr, 5, "%.2f", angle);
    client.publish(getTopic(MQTT_TOPIC_Y_ANGLE), yStr);

    DEBUG_PRINT("sensor angle (Y): ");
    DEBUG_PRINTLN(angle);

    incline = tan(angle / RAD_2_DEG) * 100;
#else
    //incline = 0;
    //angle = 0;
#endif
  }
  if (incline <= min_incline) incline = min_incline;
  if (incline > max_incline)  incline = max_incline;

  DEBUG_PRINT("sensor angle (Y): ");
  DEBUG_PRINT(angle);
  DEBUG_PRINT(" -> ");
  DEBUG_PRINTLN(incline);

  // probably need some more smoothing here ...
  // ...
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


// void calculateRPM() {
//   // divide number of microseconds in a minute, by the average interval.
//   if (revCount > 0) { // confirm there was at least one spin in the last second
//     hasReed = true;
//     DEBUG_PRINT("revCount="); DEBUG_PRINTLN(revCount);
//     // rpmaccumulatorInterval = 60000000/(accumulatorInterval/revCount);
//     rpm = 60000000 / (accumulatorInterval / revCount);
//     //accumulatorInterval = 0;
//     //Test = Calculate average from last 4 rpms - response too slow
//     accumulator4 -= (accumulator4 >> 2);
//     accumulator4 += rpm;
//     mps = belt_distance * (rpm) / (60 * 1000);
//   }
//   else {
//     rpm = 0;
//     //rpmaccumulatorInterval = 0;
//     accumulator4 = 0;  // average rpm of last 4 samples
//   }
//   revCount = 0;
//   accumulatorInterval = 0;
//   //mps = belt_distance * (rpm) / (60 * 1000);
// }

String mqtt_topics[] {
  "home/treadmill/%MQTTDEVICEID%/setconfig",
  "home/treadmill/%MQTTDEVICEID%/state",
  "home/treadmill/%MQTTDEVICEID%/version",
  "home/treadmill/%MQTTDEVICEID%/ipaddr",
  "home/treadmill/%MQTTDEVICEID%/rst",
  "home/treadmill/%MQTTDEVICEID%/speed",
  "home/treadmill/%MQTTDEVICEID%/rpm",
  "home/treadmill/%MQTTDEVICEID%/incline",
  "home/treadmill/%MQTTDEVICEID%/y_angle"
};

void setupMqttTopic(const String &id)
{
  for (unsigned i = 0; i < MQTT_NUM_TOPICS; ++i) {
    mqtt_topics[i].replace("%MQTTDEVICEID%", id);
  }
}

const char* getTopic(topics_t topic) {
  return mqtt_topics[topic].c_str();
}

const char* getRstReason(esp_reset_reason_t r) {
  switch(r) {
  case ESP_RST_UNKNOWN:    return "ESP_RST_UNKNOWN";   //!< Reset reason can not be determined
  case ESP_RST_POWERON:    return "ESP_RST_POWERON";   //!< Reset due to power-on event
  case ESP_RST_EXT:        return "ESP_RST_EXT";       //!< Reset by external pin (not applicable for ESP32)
  case ESP_RST_SW:         return "ESP_RST_SW";        //!< Software reset via esp_restart
  case ESP_RST_PANIC:      return "ESP_RST_PANIC";     //!< Software reset due to exception/panic
  case ESP_RST_INT_WDT:    return "ESP_RST_INT_WDT";   //!< Reset (software or hardware) due to interrupt watchdog
  case ESP_RST_TASK_WDT:   return "ESP_RST_TASK_WDT";  //!< Reset due to task watchdog
  case ESP_RST_WDT:        return "ESP_RST_WDT";       //!< Reset due to other watchdogs
  case ESP_RST_DEEPSLEEP:  return "ESP_RST_DEEPSLEEP"; //!< Reset after exiting deep sleep mode
  case ESP_RST_BROWNOUT:   return "ESP_RST_BROWNOUT";  //!< Brownout reset (software or hardware)
  case ESP_RST_SDIO:       return "ESP_RST_SDIO";      //!< Reset over SDIO
  }
  return "INVALID";
}


void setup() {
  DEBUG_BEGIN(115200);
  DEBUG_PRINTLN("setup started");
  rr = esp_reset_reason();

  // fixme check return code
  esp_efuse_mac_get_default(mac_addr);

  MQTTDEVICEID += String(mac_addr[4], HEX);
  MQTTDEVICEID += String(mac_addr[5], HEX);
  setupMqttTopic(MQTTDEVICEID);

  // initial min treadmill speed
  kmph = 0.5;
  incline = 0;
  grade_deg = 0;
  angle = 0;
  elevation = 0;
  elevation_gain = 0;


#if TREADMILL_MODEL == NORDICTRACK_12SI
  speedInclineMode = SPEED | INCLINE;
  hasReed          = true;
#endif
#if TREADMILL_MODEL == TAURUS_9_5
  hasReed          = true;
#endif

  buttonInit();
  tft.init();
  tft.setRotation(1); // 3
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");
#ifdef DEBUG0_PIN
  pinMode(DEBUG0_PIN, OUTPUT);
#endif
  pinMode(TREADMILL_BUTTON_INC_DOWN_PIN, INPUT);
  pinMode(TREADMILL_BUTTON_INC_UP_PIN, INPUT);

  delay(3000);
  // pinMode(SPEED_IR_SENSOR1, INPUT_PULLUP);
  // pinMode(SPEED_IR_SENSOR1, INPUT_PULLUP);

  attachInterrupt(SPEED_IR_SENSOR1, speedSensor1_ISR, FALLING);
  attachInterrupt(SPEED_IR_SENSOR2, speedSensor2_ISR, FALLING);

  // note: I have 10k pull-up on SPEED_REED_SWITCH_PIN
  pinMode(SPEED_REED_SWITCH_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SPEED_REED_SWITCH_PIN), reedSwitch_ISR, FALLING);

  #ifndef NO_MPU6050
  Wire.begin();
  #endif

  initBLE();
  
  initSPIFFS();

  isWifiAvailable = setupWifi() ? false : true;

  if (isWifiAvailable) {
    DEBUG_PRINTLN("Init Webserver");
    initAsyncWebserver();
    initWebSocket();
  }
  //else show offline msg, halt or reboot?!

  if (isWifiAvailable) {
    isMqttAvailable = mqttConnect();
    delay(2000);
  }

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("mqtt setup Done!");
  delay(2000);
  
#ifndef NO_MPU6050
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(20, 40);
  tft.println("init MPU6050");

  byte status = mpu.begin();
  DEBUG_PRINT("MPU6050 status: ");
  DEBUG_PRINTLN(status);
  if (status != 0) {
    tft.setTextColor(TFT_RED);
    tft.println("MPU6050 setup failed!");
    tft.println(status);
    delay(2000);
  }
  else {
    DEBUG_PRINTLN(F("Calculating offsets, do not move MPU6050"));
    tft.setTextFont(2);
    tft.println("Calc offsets, do not move MPU6050 (3sec)");
    tft.setTextFont(4);
    mpu.calcOffsets(); // gyro and accel.
    delay(2000);
    speedInclineMode |= INCLINE;
    hasMPU6050 = true;
    tft.setTextColor(TFT_RED);
    tft.println("MPU6050 OK!");
  }
#else
  hasMPU6050 = false;
#endif
#ifndef NO_VL53L0X
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setCursor(20, 40);
  tft.println("init VL53L0X");

  sensor.setTimeout(500);
  if (!sensor.init()) {
    DEBUG_PRINTLN("Failed to detect and initialize VL53L0X sensor!");
    tft.setTextColor(TFT_RED);
    tft.println("VL53L0X setup failed!");
    delay(2000);
  }
  else {
    DEBUG_PRINTLN("VL53L0X sensor detected and initialized!");
    tft.setTextColor(TFT_GREEN);
    tft.println("VL53L0X initialized!");
    hasVL53L0X = true;
    delay(2000);
  }
#else
    hasVL53L0X = false;
#endif

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  //tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Done");
  tft.print("Version:");
  tft.println(VERSION);
  DEBUG_PRINTLN("setup done");

  delay(3000);
  updateDisplay(true);

  // indicate manual/auto mode (green=auto/sensor, red=manual)
  showSpeedInclineMode(speedInclineMode);

  // indicate bt connection status ... offline
  tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_BLACK);
  tft.drawCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);

  show_WIFI(wifi_reconnect_counter, getWifiIpAddr());

  setTime(0,0,0,0,0,0);
}

void loop_handle_WIFI() {
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
    show_WIFI(wifi_reconnect_counter, getWifiIpAddr());
  }
}

void loop_handle_BLE() {
  // if changed to connected ...
  if (bleClientConnected && !bleClientConnectedPrev) {
    bleClientConnectedPrev = true;
    DEBUG_PRINTLN("BT Client connected!");
    updateBTConnectionStatus(bleClientConnectedPrev);
  }
  else if (!bleClientConnected && bleClientConnectedPrev) {
    bleClientConnectedPrev = false;
    DEBUG_PRINTLN("BT Client disconnected!");
    updateBTConnectionStatus(bleClientConnectedPrev);
  }
}

//#define SHOW_FPS

void loop() {
#ifdef SHOW_FPS
  static int fps;
  ++fps;
  updateDisplay(false);
#endif

  buttonLoop();

#ifndef NO_MPU6050
  mpu.update();
#endif
  //uint8_t result = rotary.process();

  loop_handle_WIFI();
  loop_handle_BLE();

  // check ir-speed sensor if not manual mode
  if (t2_valid) { // hasIrSense = true
    hasIrSense = true;
    unsigned long t = t2 - t1;
    unsigned long c = 359712; // d=10cm
    kmph_sense = (float)(1.0 / t) * c;
    noInterrupts();
    t1_valid = t2_valid = false;
    interrupts();
    DEBUG_PRINTLN(t);
    DEBUG_PRINTLN(kmph_sense);
  }

  // testing ... every second
  if ((millis() - sw_timer_clock) > EVERY_SECOND) {
    sw_timer_clock = millis();

#ifdef SHOW_FPS
    show_FPS(fps);
    fps = 0;
#endif 

    if (speedInclineMode & INCLINE) {
      incline = getIncline(); // also sets 'angle' variable
    }

    // total_distance = ... v = d/t -> d = v*t -> use v[m/s]
    if (speedInclineMode & SPEED) {  // get speed from sensor (no-manual mode)
      // FIXME: ... probably can get rid of this if/else if ISR for the ir-sensor
      // and calc rpm from reed switch provide same unit
      if (hasIrSense) {
        kmph = kmph_sense;
        mps = kmph / 3.6;
        total_distance += mps;
      }
      else if (hasReed) {
	if (revCount > 0) { // confirm there was at least one spin in the last second
	  noInterrupts();
	  rpm = 60000000 / (accumulatorInterval / revCount);
	  mps = belt_distance * (rpm) / (60 * 1000);
	  revCount = 0;
	  accumulatorInterval = 0;
	  interrupts();

	  kmph = mps * 3.6;
	}
	else {
	  rpm = 0;
	  //rpmaccumulatorInterval = 0;
	  //accumulator4 = 0;  // average rpm of last 4 samples
	}
	//mps = belt_distance * (rpm) / (60 * 1000);
        //kmph = mps * 3.6;
        total_distance = workoutDistance / 1000;  // meter
      }
    }
    else {
      mps = kmph / 3.6;
      total_distance += mps;
    }
    elevation_gain += (double)(sin(angle) * mps);

#if 0
    DEBUG_PRINT("mps = d:    ");DEBUG_PRINTLN(mps);
    DEBUG_PRINT("angle:      ");DEBUG_PRINTLN(angle);
    DEBUG_PRINT("h (m):      ");DEBUG_PRINTLN(sin(angle) * mps);
    
    DEBUG_PRINT("h gain (m): ");DEBUG_PRINTLN(elevation_gain);
    
    DEBUG_PRINT("kmph:      "); DEBUG_PRINTLN(kmph);
    DEBUG_PRINT("incline:   "); DEBUG_PRINTLN(incline);
    DEBUG_PRINT("angle:     "); DEBUG_PRINTLN(grade_deg);
    DEBUG_PRINT("dist km:   "); DEBUG_PRINTLN(total_distance/1000);
    DEBUG_PRINT("elegain m: "); DEBUG_PRINTLN(elevation_gain);
#endif
    char inclineStr[6];
    char kmphStr[6];
    snprintf(inclineStr, 6, "%.1f", incline);
    snprintf(kmphStr,    6, "%.1f", kmph);
    client.publish(getTopic(MQTT_TOPIC_INCLINE), inclineStr);
    client.publish(getTopic(MQTT_TOPIC_SPEED),   kmphStr);

#ifndef SHOW_FPS
    updateDisplay(false);
#endif
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