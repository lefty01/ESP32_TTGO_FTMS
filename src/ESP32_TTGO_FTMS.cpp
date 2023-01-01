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
// TODO:  web config menu (+https, +setup initial user password)

#include <math.h>
#include <SPI.h>
//#include <FS.h>
#include <LittleFS.h>
#include <Preferences.h>
#include <unity.h>

#include "common.h"
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

float  kmph = 0;
float  kmphIRsense = 0;
float  incline = 0;
double totalDistance = 0;
double elevationGain = 0;
//uint8_t     inst_cadence = 1;                 /* Instantaneous Cadence. */
//uint16_t    inst_stride_length = 1;           /* Instantaneous Stride Length. */
double      elevation;
float mps;  // meter per second
double angle = 0;
double gradeDeg = 0;
Preferences prefs;

bool setupDone = false;

uint8_t speedInclineMode = SPEED;
static unsigned long touch_timer = 0;

void logText(const char *text)
{
  // Serial consol
  DEBUG_PRINTF(text);
#warning todo cs:putback gfxlogtext
#ifndef NO_DISPLAY 
  gfxLogText(text);
#endif
}

void logText(uint32_t text)
{
  logText(std::to_string(text));
}


void logText(String text)
{
  logText(text.c_str());
}

void logText(std::string text)
{
  logText(text.c_str());
}

void initLittleFS() 
{
  logText("initLittleFS...");

  // begin, format if failed
  if (!LittleFS.begin(true)) { // true = formatOnFail
    logText("Failed, Cannot mount LittkeFS volume\n");
    return;
  }
  logText("done\n");
}

void doReset()
{
  resetStopWatch();
  totalDistance = 0;
  elevationGain = 0;
  kmph = 0.5;
  incline = 0;
}

// UI controlled (web or button on esp32) speedUp
void speedUp()
{
  if (speedInclineMode & SPEED) {
    handleEvent(EventType::TREADMILL_SPEED_UP);
    return;
  }

  kmph += configTreadmill.speed_interval_step;
  if (kmph > configTreadmill.max_speed) kmph = configTreadmill.max_speed;
  logText("speed_up,   new speed: ");
  logText(kmph);
  logText("\n");
}

// UI controlled (web or button on esp32) speedDown
void speedDown()
{
  if (speedInclineMode & SPEED) {
    handleEvent(EventType::TREADMILL_SPEED_DOWN);
    return;
  }

  kmph -= configTreadmill.speed_interval_step;
  if (kmph < configTreadmill.min_speed) kmph = configTreadmill.min_speed;
  logText("speed_down, new speed: ");
  logText(kmph);
  logText("\n");
}

// UI controlled (web or button on esp32) inclineUp
void inclineUp()
{
  if (speedInclineMode & INCLINE) {
    handleEvent(EventType::TREADMILL_INC_UP);
    return;
  }

  incline += configTreadmill.incline_interval_step; // incline in %
  if (incline > configTreadmill.max_incline) incline = configTreadmill.max_incline;
  angle = atan2(incline, 100);
  gradeDeg = angle * 57.296;
  logText("incline_up, new incline: ");
  logText(incline);
  logText("\n");
}

// UI controlled (web or button on esp32) inclineDown
void inclineDown()
{
  if (speedInclineMode & INCLINE) {
    handleEvent(EventType::TREADMILL_INC_DOWN);
    return;
  }

  incline -= configTreadmill.incline_interval_step;
  if (incline <= configTreadmill.min_incline) incline = configTreadmill.min_incline;
  angle = atan2(incline, 100);
  gradeDeg = angle * 57.296;
  logText("incline_down, new incline: ");
  logText(incline);
  logText("\n");
}

// angleSensorTreadmillConversion()
// If a treadmill has some special placement of the angle sensor
// here is where that value is converted from sensor read to proper angle of running area.

double angleSensorTreadmillConversion(double inAngle) {
  double convertedAngle = inAngle;

#warning TODO make this a configurable depending on where on the threadmill you place the MPU6050   
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

  logText("setSpeed: ");
  logText(kmph);
  logText("\n");
}

void setSpeedInterval(float interval)
{
  logText("set_speed_interval: ");
  logText(interval);

  if ((interval < 0.1) || (interval > 2.0)) {
    logText(" INVALID SPEED INTERVAL");
  }
  logText("\n");

  configTreadmill.speed_interval_step = interval;
}

static void showInfo()
{
  String intoText = String("----------------------------------------------\n");
  logText(intoText.c_str());
  intoText = String("ESP32 FTMS - ") + VERSION + String("\n");
  logText(intoText.c_str());
  logText(configTreadmill.treadmill_name.c_str());
  intoText = String("\nSpeed[") + configTreadmill.min_speed + String(", ") + configTreadmill.max_speed +
             String(" Incline[") + configTreadmill.min_incline + String(", ") + configTreadmill.max_incline + String("]\n");
  logText(intoText.c_str());
  intoText = String("Dist/REED: ") + configTreadmill.belt_distance + String("mm\n");
  logText(intoText.c_str());
  intoText = String("HW: REED: ") + configTreadmill.hasReed +
             String(" MPU6050: ") + configTreadmill.hasMPU6050 +
             String(" IrSense: ") + configTreadmill.hasIrSense +
             String(" GPIOExtender(AW9523): ") + GPIOExtender.isAvailable() + String("\n");
  logText(intoText.c_str());
}

void setup() 
{
  // initial min treadmill speed
  kmph = 0.5;
  incline = 0;
  gradeDeg = 0;
  angle = 0;
  elevation = 0;
  elevationGain = 0;

  DEBUG_BEGIN(115200);
  initDisplay();

  logText("Setup started:\n");
  initWifi();
  initLittleFS();
  initButton();
  initBLE();
  initConfig();
  initHardware();

  if (isWifiAvailable) {
    initAsyncWebserver();
    initWebSocket();
    initMqtt();
  }

  showInfo();
  delay(4000); //allow showInfo() to display for a short while
  resetStopWatch();

  logText("setup done\n");
  setupDone = true;
#ifndef NO_DISPLAY
  gfxUpdateDisplay(true);
#endif
}

void loop() 
{  
  loopHandleHardware();
  loopHandleButton();
  loopHandleGfx();
  loopHandleWIFI();
  loopHandleBLE();

 // timertick... every second
  if (timer_tick == true) {
    timer_tick = false;

    if (speedInclineMode & INCLINE) {
      char inclineStr[6];

      incline = getIncline(); // sets global 'angle' and 'incline' variable
      snprintf(inclineStr, 6, "%.1f", incline);
    //client.publish(getTopic(MQTT_TOPIC_INCLINE), inclineStr);
    }

    // totalDistance = ... v = d/t -> d = v*t -> use v[m/s]
    if (speedInclineMode & SPEED) {  // get speed from sensor (no-manual mode)
      // FIXME: ... probably can get rid of this if/else if ISR for the ir-sensor
      // and calc rpm from reed switch provide same unit
      if (configTreadmill.hasIrSense) {
        kmph = kmphIRsense;
        mps = kmph / 3.6; // meter per second (EVERY_SECOND)
        totalDistance += mps;
      }
      else {
        float rpm = calculateRPM();
        mps = configTreadmill.belt_distance * (rpm) / (60 * 1000); // meter per sec
        kmph = mps * 3.6;                          // km per hour
        totalDistance = workoutDistance / 1000;   // conv mm to meter
      }
    }
    else {
      mps = kmph / 3.6;
      totalDistance += mps;
    }
    //elevationGain += (double)(sin(angle) * mps);
    elevationGain += incline / 100 * mps;

 #if 0
    logText("mps = d: ");       logText(mps);
    logText("   angle: ");      logText(angle);
    logText("   h (m): ");      logText(sin(angle) * mps);
    logText("   h gain (m): "); logText(elevationGain);
    logText("   kmph: ");       logText(kmph);
    logText("   incline: ");    logText(incline);
    logText("   angle: ");      logText(gradeDeg);
    logText("   dist km: ");    logText(totalDistance/1000);
    logText("\n");
#endif
    publishTopicsMqtt();
#ifndef NO_DISPLAY
    gfxUpdateDisplay(false);
#endif
    notifyClientsWebSockets();
    updateBLEdata(); //Send FTMS mased in calulated globals kmph, incline, gradeDeg, elevationGain 
  }
}
