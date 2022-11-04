/**
 *
 *
 * The MIT License (MIT)
 * Copyright © 2021, 2022 <Andreas Loeffler>
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

// FIXME: incline manual/auto toggle -> not same for incline and speed!!
// FIXME: if manual incline round up/down to integer
// FIXME: elevation gain
// FIXME: once and for good ... camle vs. underscore case
// TODO:  web config menu (+https, +setup initial user password)

//#include "common.h"
#include <math.h>
#include <SPI.h>
//#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <unity.h>

#include "ESP32_TTGO_FTMS.h"
#include "debug_print.h"
#include "hardware.h"
#include "net-control.h"
#include "display.h"
#include "config.h"

// Select and uncomment one of the Treadmills below
//#define TREADMILL_MODEL TAURUS_9_5
//#define TREADMILL_MODEL NORDICTRACK_12SI
// or via platformio.ini:
// -DTREADMILL_MODEL="TAURUS_9_5"

//unsigned long sw_timer_clock = 0;

float  kmph=0;
float kmph_sense=0;
float  incline=0;
double total_distance=0;
double elevation_gain=0;
//uint8_t     inst_cadence = 1;                 /* Instantaneous Cadence. */
//uint16_t    inst_stride_length = 1;           /* Instantaneous Stride Length. */
double      elevation;
float mps;  // meter per second
double angle = 0;
double grade_deg = 0;
Preferences prefs;

uint8_t speedInclineMode = SPEED;
static unsigned long touch_timer = 0;

void logText(const char *text)
{
  // Serial consol
  DEBUG_PRINTF(text);
#warning todo cs:putback gfxlogtext
#ifndef NO_DISPLAY 
//  gfxLogText(text);
#endif
}

void logText(String text)
{
  logText(text.c_str());
}

void initLittleFS() 
{
  logText("initLittleFS...");

  // begin, format if failed
  if(!LittleFS.begin(true))  // true = formatOnFail
  {
    logText("Failed, Cannot mount LittkeFS volume\n");
//    Serial.println("LITTLEFS Mount Failed");

//        return;
  }
//  LittleFS.begin();
  
//  if (!SPIFFS.begin(true)) { // true = formatOnFail
//    logText("Failed, Cannot mount SPIFFS volume\n");
//  }  
//  else {
    logText("done\n");
//  }
}


void doReset()
{
  resetStopWatch();
  total_distance = 0;
  elevation_gain = 0;
  kmph = 0.5;
  incline = 0;
}

// UI controlled (web or button on esp32) speedUp
void speedUp()
{
  if (speedInclineMode & SPEED) {
    handle_event(EventType::TREADMILL_SPEED_UP);
    return;
  }

  kmph += configTreadmill.speed_interval_step;
  if (kmph > configTreadmill.max_speed) kmph = configTreadmill.max_speed;
  DEBUG_PRINT("speed_up, new speed: ");
  DEBUG_PRINTLN(kmph);
}

// UI controlled (web or button on esp32) speedDown
void speedDown()
{
  if (speedInclineMode & SPEED) {
    handle_event(EventType::TREADMILL_SPEED_DOWN);
    return;
  }

  kmph -= configTreadmill.speed_interval_step;
  if (kmph < configTreadmill.min_speed) kmph = configTreadmill.min_speed;
  DEBUG_PRINT("speed_down, new speed: ");
  DEBUG_PRINTLN(kmph);
}

// UI controlled (web or button on esp32) inclineUp
void inclineUp()
{
  if (speedInclineMode & INCLINE) {
    handle_event(EventType::TREADMILL_INC_UP);
    return;
  }

  incline += configTreadmill.incline_interval_step; // incline in %
  if (incline > configTreadmill.max_incline) incline = configTreadmill.max_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_up, new incline: ");
  DEBUG_PRINTLN(incline);
}

// UI controlled (web or button on esp32) inclineDown
void inclineDown()
{
  if (speedInclineMode & INCLINE) {
    handle_event(EventType::TREADMILL_INC_DOWN);
    return;
  }

  incline -= configTreadmill.incline_interval_step;
  if (incline <= configTreadmill.min_incline) incline = configTreadmill.min_incline;
  angle = atan2(incline, 100);
  grade_deg = angle * 57.296;
  DEBUG_PRINT("incline_down, new incline: ");
  DEBUG_PRINTLN(incline);
}

// angleSensorTreadmillConversion()
// If a treadmill has some special placement of the angle sensor
// here is where that value is converted from sensor read to proper angle of running area.

double angleSensorTreadmillConversion(double inAngle) {
  double convertedAngle = inAngle;

#warning todo remove this   
//#if TREADMILL_MODEL == NORDICTRACK_12SI
  // TODO: Maybe this should be a config somewhere together with sensor orientation

  /* If Sensor is placed in inside the treadmill engine
    TODO: Maybe this can be automatic, e.g. Let user selct a incline at a time
          and record values and select inbeween bounderies.
          If treadmill support stearing the incline maybe it can also be an automatic
          calibration step. e.g. move it max down, callibrate sensor, step up max and measure
          User input treadmill "Max incline" value and Running are length somehow.
              /|--- ___
           c / |        --- ___ a
            /  |x              --- ___  Running area
           /   |_                      --- ___
          / A  | |                           C ---  ___

    A = inAngle (but we want the angle C)
    sin(A)=x/c     sin(C)=x/a
    x=c*sin(A)     x=a*sin(C)
    C=asin(c*sin(A)/a)
  */
  double c = 32.0;  // lenght of motor part in cm
  double a = 150.0; // lenght of running area in cm
  convertedAngle = asin(c*sin(inAngle * DEG_TO_RAD)/a) * RAD_TO_DEG;
//#endif
  return convertedAngle;
}

void setSpeed(float speed)
{
  kmph = speed;
  if (speed > configTreadmill.max_speed) kmph = configTreadmill.max_speed;
  if (speed < configTreadmill.min_speed) kmph = configTreadmill.min_speed;

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

  configTreadmill.speed_interval_step = interval;
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

void setup() 
{
  // initial min treadmill speed
  kmph = 0.5;
  incline = 0;
  grade_deg = 0;
  angle = 0;
  elevation = 0;
  elevation_gain = 0;

  DEBUG_BEGIN(115200);

  delay(2000);  
  logText("setup started...\n");
  initWifi();
  delay(4000);  

  do_it();

  initLittleFS();
  initButton();
  initBLE();
  initConfig();
  initHardware();
  initDisplay();


  if (isWifiAvailable) 
  {
    initAsyncWebserver();
    initWebSocket();
    initMqtt();
  }

  initSensors();
  showInfo();
  updateDisplay(true);
  resetStopWatch();

  logText("setup done\n");
}

void loop() 
{
  float rpm = 0;  

#ifdef SHOW_FPS
  static int fps;
  ++fps;
#endif

  loop_handle_hardware();
  loop_handle_button();
  loop_handle_touch();
  loop_handle_WIFI();
  loop_handle_BLE();

 // timertick... every second
  if (timer_tick == true)
  {
    timer_tick = false;

#ifdef SHOW_FPS
    show_FPS(fps);
    fps = 0;
#endif

    if (speedInclineMode & INCLINE) 
    {
      char inclineStr[6];

      incline = getIncline(); // sets global 'angle' and 'incline' variable
      snprintf(inclineStr, 6, "%.1f", incline);
    //client.publish(getTopic(MQTT_TOPIC_INCLINE), inclineStr);
    }

    // total_distance = ... v = d/t -> d = v*t -> use v[m/s]
    if (speedInclineMode & SPEED) 
    {  // get speed from sensor (no-manual mode)
      // FIXME: ... probably can get rid of this if/else if ISR for the ir-sensor
      // and calc rpm from reed switch provide same unit
      if (configTreadmill.hasIrSense) {
        kmph = kmph_sense;
        mps = kmph / 3.6; // meter per second (EVERY_SECOND)
        total_distance += mps;
      }
      else
      {
        rpm = calculate_RPM();
        mps = configTreadmill.belt_distance * (rpm) / (60 * 1000); // meter per sec
        kmph = mps * 3.6;                          // km per hour
        total_distance = workoutDistance / 1000;   // conv mm to meter
      }
    }
    else
    {
      mps = kmph / 3.6;
      total_distance += mps;
    }
    //elevation_gain += (double)(sin(angle) * mps);
    elevation_gain += incline / 100 * mps;

 #if 0
    DEBUG_PRINT("mps = d: ");       DEBUG_PRINT(mps);
    DEBUG_PRINT("   angle: ");      DEBUG_PRINT(angle);
    DEBUG_PRINT("   h (m): ");      DEBUG_PRINT(sin(angle) * mps);
    DEBUG_PRINT("   h gain (m): "); DEBUG_PRINT(elevation_gain);
    DEBUG_PRINT("   kmph: ");       DEBUG_PRINT(kmph);
    DEBUG_PRINT("   incline: ");    DEBUG_PRINT(incline);
    DEBUG_PRINT("   angle: ");      DEBUG_PRINT(grade_deg);
    DEBUG_PRINT("   dist km: ");    DEBUG_PRINTLN(total_distance/1000);
#endif
    publish_topics();
    
#ifndef SHOW_FPS
    updateDisplay(false);
#endif
    notifyClients();
    updateBLEdata();
  }
}