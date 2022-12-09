/**
 *
 *
 * The MIT License (MIT)
 * Copyright © 2022 <Andreas Loeffler> <Zingo Andersen>
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

#include <Arduino.h>
#include <Button2.h>
//#ifndef NO_MPU6050
#include <Wire.h>
#include <MPU6050_light.h> // accelerometer and gyroscope -> measure incline
//#endif
//#ifndef NO_VL53L0X
#include <VL53L0X.h>       // time-of-flight sensor -> get incline % from distance to ground
//#endif


#include "ESP32_TTGO_FTMS.h"
#include "net-control.h"
#include "hardware.h"
#include "debug_print.h"
#include "config.h"



esp_reset_reason_t reset_reason;
unsigned long sw_timer_clock = 0;



unsigned long wifi_reconnect_counter = 0;

//#define SPEED_IR_SENSOR1   15
//#define SPEED_IR_SENSOR2   13

bool timer_tick;
static constexpr int AW9523_INTERRUPT_PIN  = 25; // GPIO Extender interrupt
static constexpr int SPEED_REED_SWITCH_PIN = 26; // REED-Contact

float rpm = 0;
volatile unsigned long t1;
volatile unsigned long t2;
volatile bool t1_valid = false;
volatile bool t2_valid = false;

volatile unsigned long startTime = 0;     // start of revolution in microseconds
volatile unsigned long longpauseTime = 0; // revolution time with no reed-switch interrupt
volatile long accumulatorInterval = 0;    // time sum between display during intervals
volatile unsigned int revCount = 0;       // number of revolutions since last display update
volatile long accumulator4 = 0;           // sum of last 4 rpm times over 4 seconds
volatile long workoutDistance = 0; // FIXME: vs. total_dist ... select either reed/ir/manual

//csstatic TwoWire I2C_0 = TwoWire(0);
TwoWire I2C_0 = TwoWire(0);

// PINS
// NOTE TTGO T-DISPAY GPIO 36,37,38,39 can only be input pins https://github.com/Xinyuan-LilyGO/TTGO-T-Display/issues/10

// ttgo tft: 33, 25, 26, 27
// the number of the pushbutton pins

#if TARGET_TTGO_T_DISPLAY
// TTGO T-Display buttons
#define BUTTON_1 35
#define BUTTON_2  0
#elif TARGET_WT32_SC01
// This is a touch screen so there is no buttons
// IO_21 and IO_22 are routed out and use used for SPI to the screen
// Pins with names that start with J seems to be connected so screen, take care to avoid.
#ifndef NUM_TOUCH_BUTTONS
#define NUM_TOUCH_BUTTONS 6
#endif
//LGFX_Button touchButtons[NUM_TOUCH_BUTTONS];
LGFX_Button btnSpeedToggle    = LGFX_Button();
LGFX_Button btnInclineToggle  = LGFX_Button();
LGFX_Button btnSpeedUp        = LGFX_Button();
LGFX_Button btnSpeedDown      = LGFX_Button();
LGFX_Button btnInclineUp      = LGFX_Button();
LGFX_Button btnInclineDown    = LGFX_Button();
// 480 x 320, use full-top-half to show speed, incline, total_dist, and acc. elevation gain
// maybe draw some nice gauges for speed/incline and odometers for dist and elevation
// in addition might want to have an elevation profile being displayed
// 260, 5, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "MODE");
#define btnToggle_W           100
#define btnToggle_H            50

#define btnSpeedToggle_X       250
#define btnSpeedToggle_Y         5
#define btnInclineToggle_X     360
#define btnInclineToggle_Y       5
#define btnSpeedUp_X           250
#define btnSpeedUp_Y           100
#define btnSpeedDown_X         250
#define btnSpeedDown_Y         170
#define btnInclineUp_X         360
#define btnInclineUp_Y         100
#define btnInclineDown_X       360
#define btnInclineDown_Y       170
#elif TARGET_TTGO_T4
// no-touch screen with three buttons
#define BUTTON_1 38  // LEFT
#define BUTTON_2 37  // CENTRE
#define BUTTON_3 39  // RIGHT
#elif TARGET_WT32_WROOMD
// no display or buttons, web only
#else
#error Unknown button setup
#endif

#ifdef BUTTON_1
Button2 btn1(BUTTON_1);
#endif
#ifdef BUTTON_2
Button2 btn2(BUTTON_2);
#endif
#ifdef BUTTON_3
Button2 btn3(BUTTON_3);
#endif

#if TARGET_TTGO_T_DISPLAY
static const int SDA_0 = 21;
static const int SCL_0 = 22;
static const uint32_t I2C_FREQ = 400000;
#elif TARGET_WT32_SC01
// This is a touch screen so there is no buttons
// IO_21 and IO_22 are routed out and use used for SPI to the screen
// Pins with names that start with J seems to be connected so screen, take care to avoid.
static const int SDA_0 = 18;
static const int SCL_0 = 19;
static const uint32_t I2C_FREQ = 400000;
#elif TARGET_TTGO_T4
static const int SDA_0 = 21;
static const int SCL_0 = 22;
static const uint32_t I2C_FREQ = 400000;
#elif TARGET_WT32_WROOMD
// no display or buttons, web only
static const int SDA_0 = 18;
static const int SCL_0 = 19;
static const uint32_t I2C_FREQ = 400000;
#else
#error Unknown HW Platform
#endif

//#ifndef NO_VL53L0X
VL53L0X sensor;
const unsigned long MAX_DISTANCE = 1000;  // Maximum distance in mm
//#endif

//#ifndef NO_MPU6050
static MPU6050 mpu(I2C_0);
//#endif

//cs static GPIOExtenderAW9523 GPIOExtender(I2C_0);
GPIOExtenderAW9523 GPIOExtender(I2C_0);

void initLovyanGFXTouchAreas(void)
 {
  btnSpeedToggle   .initButtonUL(&tft, btnSpeedToggle_X,    btnSpeedToggle_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "SPEED");
  btnInclineToggle .initButtonUL(&tft, btnInclineToggle_X,  btnInclineToggle_Y, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "INCL.");
  btnSpeedUp       .initButtonUL(&tft, btnSpeedUp_X,        btnSpeedUp_Y,       100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnSpeedDown     .initButtonUL(&tft, btnSpeedDown_X,      btnSpeedDown_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
  btnInclineUp     .initButtonUL(&tft, btnInclineUp_X,      btnInclineUp_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnInclineDown   .initButtonUL(&tft, btnInclineDown_X,    btnInclineDown_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
}

void btn1TapHandler(Button2 & b)
{
  unsigned int time = b.wasPressedFor();
  DEBUG_PRINTLN("Button 1 TapHandler");

  if (time > 3000) 
  { // > 3sec enters config menu
    DEBUG_PRINTLN("RESET timer/counter!");
    doReset();
  }
  else if (time > 600) 
  {
    DEBUG_PRINTLN("Button 1 long click...");
    // reset to manual mode
    speedInclineMode = MANUAL;
    DEBUG_PRINT("speedInclineMode=");
      DEBUG_PRINTLN(speedInclineMode);
#ifndef NO_DISPLAY
    gfxUpdateHeader();
#endif      
  }
  else 
  { // button1 short click toggle speed/incline mode
    DEBUG_PRINTLN("Button 1 short click...");
    speedInclineMode += 1;
    speedInclineMode &= SPEEDINCLINE_BITFIELD; //wrap around MANUAL,SPEED,INCLINE,SPEED & INCLINE
    DEBUG_PRINT("speedInclineMode=");
    DEBUG_PRINTLN(speedInclineMode);
#ifndef NO_DISPLAY      
    gfxUpdateHeader();
#endif      
  }
}

void btn2TapHandler(Button2 & b)
{
  unsigned int time = b.wasPressedFor();
    DEBUG_PRINTLN("Button 2 TapHandler");
  
  if (time > 3000) 
  { // > 3sec enters config menu
    //DEBUG_PRINTLN("very long (>3s) click ... do nothing");
  }
  else if (time > 500) 
  {
    DEBUG_PRINTLN("long (>500ms) click...");
    if ((speedInclineMode & SPEED) == 0)
	  {
      speedDown();
    }  
    if ((speedInclineMode & INCLINE) == 0)
	  {  
      inclineDown();
    }
    else 
    {
      DEBUG_PRINTLN("short click...");
      if ((speedInclineMode & SPEED) == 0)
      {
	      speedUp();
      }  
      if ((speedInclineMode & INCLINE) == 0)
	    {
        inclineUp();
      }  
    }
  }
}

void btn3TapHandler(Button2 & b)
{
  unsigned int time = b.wasPressedFor();
  DEBUG_PRINTLN("Button 3 TapHandler");
  if (time > 3000) {
    // DEBUG_PRINTLN("very long (>3s) click ... do nothing");
  }
  else 
  {
    DEBUG_PRINTLN("short click...");
    if ((speedInclineMode & SPEED) == 0)
    {
      speedDown();
    }  
    if ((speedInclineMode & INCLINE) == 0)
    {
      inclineDown();
    }  
  }
}

void loopHandleButton()
{
#ifdef BUTTON_1
  btn1.loop();
#endif
#ifdef BUTTON_2
  btn2.loop();
#endif
#ifdef BUTTON_3
    btn3.loop();
#endif
}

void initButton()
{
  logText("initButton...");  
#ifdef BUTTON_1
  // button 1 (GPIO 0) control auto/manual mode and reset timers
  btn1.setTapHandler(btn1TapHandler);
#endif

  // if only two buttons, then button 1 switches mode and button 2 short is up, button 2 long is down
  // if three buttons use button 2 short as up and button 3 short as down
  // fixme: keep button2 long handler for now does not hurt
  // initially for testing purpose ... but if we can access the controller while running it can serve as backup
  // if sensors fail or suddnely send incorrect readings (yeah that happend to me) ... but since the controller
  // might not be accessable we have the web interface that can run on the smartphone
#ifdef BUTTON_2
  // short  click = up
  // longer click = down
  btn2.setTapHandler(btn2TapHandler);
 #endif

#ifdef BUTTON_3
  // short  click = down
  btn3.setTapHandler(btn3TapHandler);
#endif

#ifdef TARGET_WT32_SC01
  // for (unsigned n = 0; n < NUM_TOUCH_BUTTONS; ++n) {
  //     touchButtons[n] = LGFX_Button();
  // }
  // fixme: ...
  logText(",inittouch...");  
  btnSpeedToggle   .initButtonUL(&tft, btnSpeedToggle_X,    btnSpeedToggle_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "SPEED");
  btnInclineToggle .initButtonUL(&tft, btnInclineToggle_X,  btnInclineToggle_Y, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "INCL.");
  btnSpeedUp       .initButtonUL(&tft, btnSpeedUp_X,        btnSpeedUp_Y,       100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnSpeedDown     .initButtonUL(&tft, btnSpeedDown_X,      btnSpeedDown_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
  btnInclineUp     .initButtonUL(&tft, btnInclineUp_X,      btnInclineUp_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnInclineDown   .initButtonUL(&tft, btnInclineDown_X,    btnInclineDown_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
  //modeButton.initButtonUL(&tft, 260, 5, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "MODE");
  //modeButton.drawButton();
#endif
  logText("done\n");  
}

void delayWithDisplayUpdate(unsigned long delayMilli)
{
  unsigned long timeStartMilli = millis();
  unsigned long currentMilli = timeStartMilli;
  while((currentMilli - timeStartMilli) < delayMilli)
  {
#warning todo cs: add gxloop handler again.
//    gfxUpdateLoopHandler();
    unsigned long timeLeftMilli = delayMilli - (currentMilli - timeStartMilli) ; 
    if (timeLeftMilli >= 5 )
    {
      delay(5);
    }
    else
    {
      delay(timeLeftMilli);
    }
    currentMilli = millis();
  }
}

void initSensors(void)
{
  if (configTreadmill.hasMPU6050)
  {
    logText("init MPU6050....");

    byte status = mpu.begin();
    DEBUG_PRINT("status: ");
    DEBUG_PRINT(status);

    if (status != 0) 
    {
      logText(" failed\n");
    }
    else
    {
      logText("Calc offsets, do not move MPU6050 (3sec)");

      mpu.calcOffsets(); // gyro and accel.
      delayWithDisplayUpdate(2000);
      speedInclineMode |= INCLINE;

      logText(" done\n");
    }
  }

  if (configTreadmill.hasVL53L0X) 
  {
    logText("init VL53L0X\n");
    sensor.setTimeout(500);

    if (!sensor.init()) 
    {
      DEBUG_PRINTLN("Failed to detect and initialize VL53L0X sensor!");
      logText("VL53L0X setup failed!\n");    
    }
    else 
    {
      DEBUG_PRINTLN("VL53L0X sensor detected and initialized!");
      logText("VL53L0X initialized!\n");
    }
  }
}

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
    //DEBUG_PRINT(test_elapsed); //DEBUG_PRINTLN(" Counted");

    startTime = usNow;  // reset the clock
    //long elapsed = test_elapsed;
    longpauseTime = test_elapsed;

    revCount++;
    workoutDistance += configTreadmill.belt_distance;
    accumulatorInterval += test_elapsed;
  }
}

// two IR-Sensors
void IRAM_ATTR speedSensor1_ISR(void)
{
  if (! t1_valid) {
    t1 = micros();
    t1_valid = true;
  }
}

void IRAM_ATTR speedSensor2_ISR(void)
{
  if (t1_valid && !t2_valid) {
    t2 = micros();
    t2_valid = true;
  }
}

// getIncline()
// This will read the used "incline" sensor, run this periodically
// The following global variables will be updated
//    angle - the angle of the running are
//    incline - the incline value (% of angle between 0 and 45 degree)
//              incline is usally the value shown by your treadmill.
float getIncline(void) 
{
  double sensorAngle = 0.0;
  float temp_incline;
  char yStr[5];
  char inclineStr[6];

  if (configTreadmill.hasVL53L0X) 
  {
    // calc incline/angle from distance
    // depends on sensor placement ... TODO: configure via webinterface
  }

  if (configTreadmill.hasMPU6050) 
  {
    // MPU6050 returns a incline/grade in degrees(!)
    // TODO: configure sensor orientation  via webinterface
    // FIXME: maybe get some rolling-average of Y-angle to smooth things a bit (same for speed)
    // mpu.getAngle[XYZ]
    sensorAngle = mpu.getAngleY();
    angle = angleSensorTreadmillConversion(sensorAngle);

    if (angle < 0) angle = 0;  // TODO We might allow running downhill

    snprintf(yStr, 5, "%.2f", angle);

    temp_incline = tan(angle * DEG_TO_RAD) * 100;
    snprintf(inclineStr, 6, "%.1f", temp_incline);

    //client.publish(getTopic(MQTT_TOPIC_Y_ANGLE), yStr);
#warning check out if this must be here ******************
    // CS WHY IS THIS HERE
    //client.publish(getTopic(MQTT_TOPIC_INCLINE), inclineStr);
    // ******************************************
  }

  if (temp_incline <= configTreadmill.min_incline) temp_incline = configTreadmill.min_incline;
  if (temp_incline > configTreadmill.max_incline)  temp_incline = configTreadmill.max_incline;

#warning cs todo remove this
  DEBUG_PRINTF("sensor angle (%.2f): used angle: %.2f: -> incline: %f%%\n",sensorAngle, angle, temp_incline);

  // probably need some more smoothing here ...
  // ...
  return temp_incline;
}

void loopHandleHardware(void)
{
+ // NOTE/WARNING:
  // MPU6050 (mpu) and GPIOExtender use twowire i2c driver and LovyanGFX has it's own
+ // This makes the twowire collide in some way and you get a 1s delay after each touch
+ // on the screen. To solve this collition of the i2c HW LovyanGFX code shoule be placed
  // outside of the I2C_0.begin/end below that surrounds MPU6050 (mpu) and GPIOExtender
  I2C_0.begin(SDA_0 , SCL_0 , I2C_FREQ); 

  GPIOExtender.loopHandler();

  if ((millis() - sw_timer_clock) > EVERY_SECOND) 
  {
    sw_timer_clock = millis();
    timer_tick = true;
  } 

  if (configTreadmill.hasMPU6050)
  {
    mpu.update();
  }  

  // IrSensor to check speed
  // TODO: Only run if configured?
  unsigned long t;
#warning remove this constant to some config/file  
  const unsigned long c = 359712; // d=10cm
  t = t2 - t1;
  // check ir-speed sensor if not manual mode
  if (t2_valid) { // hasIrSense = true
    configTreadmill.hasIrSense = true;
    kmphIRsense = (float)(1.0 / t) * c;
    noInterrupts();
    t1_valid = t2_valid = false;
    interrupts();
    DEBUG_PRINTF("IrSense: t=%li kmph_sense=%f\n",t,kmphIRsense);
  }
  I2C_0.end();  
} 

// A simple event handler, currenly just a call stack, an event queue would be smarter
// but this solves the problem as a start, not sure it we really have the usecase where
// we need a queue, lets add it in that case

void handleEvent(EventType event)
{
  // Add more event handlers here in prio order, checking if handled before running
  // each handle will return true is event is handled so we can stop the event check
  // in that case.
  // Currently there is no queue so if Events are translated to new events and call
  // handleEvent() to post them take care to not get into loops.
  if (GPIOExtender.pressEvent(event)) return;

  DEBUG_PRINTF("handleEvent() Cant handle Event:0x%x\n",static_cast<uint32_t>(event));
  return;
}
float calculateRPM(void)
{
  float temp_rpm = 0;

  if (configTreadmill.hasReed)
  {
    if (revCount > 0) 
    { // confirm there was at least one spin in the last second
      noInterrupts();
      temp_rpm = 60000000 / (accumulatorInterval / revCount);
      revCount = 0;
      accumulatorInterval = 0;
      interrupts();
    } 
  }
  else 
  {
    temp_rpm = 0;
    //rpmaccumulatorInterval = 0;
    //accumulator4 = 0;  // average rpm of last 4 samples
  }
  return temp_rpm;
}

static void initGPIOExtender(void) {
  logText("initGPIOExtender...\n");  
  while (!GPIOExtender.begin())
  {
    DEBUG_PRINTLN("GPIOExtender not found");
    return;
  }

#ifdef AW9523_IRQ_MODE
  pinMode(AW9523_INTERRUPT_PIN, INPUT_PULLUP); 
  //pinMode(AW9523_INTERRUPT_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(AW9523_INTERRUPT_PIN), GPIOExtenderInterrupt, CHANGE);
  GPIOExtender.getPins(); // Get pins one to clear intrrupts
#endif
   logText("done\n");  
}

#ifdef AW9523_IRQ_MODE
static void IRAM_ATTR GPIOExtenderInterrupt(void) {
  GPIOExtender.gotInterrupt();
}
#endif

void GPIOExtenderAW9523::loopHandler(void)
{
    if (enabled)
    {

#ifdef AW9523_IRQ_MODE
        if (GPIOExtender.checkInterrupt())
#endif
        {
            static uint16_t prevKeys = 0;
            uint16_t pins = getPins();
            uint16_t keys = ((~pins) & 0x3f );

            if ( keys != prevKeys)
            {
            // New key status
            //DEBUG_PRINTF("GPIOExtender KEY IN:0x%x (0x%x)\n",keys,pins);

            if((keys & AW9523_KEY_UP) && AW9523_KEY_UP)
            {
                DEBUG_PRINTLN("AW9523_KEY_UP");
                handleEvent(EventType::KEY_UP);
                //handleEvent(EventType::TREADMILL_INC_UP);
            }
            if((keys & AW9523_KEY_LEFT) && AW9523_KEY_LEFT)
            {
                DEBUG_PRINTLN("AW9523_KEY_LEFT");
                handleEvent(EventType::KEY_LEFT);
                //handleEvent(EventType::TREADMILL_SPEED_DOWN);
            }
            if((keys & AW9523_KEY_DOWV) && AW9523_KEY_DOWV)
            {
                DEBUG_PRINTLN("AW9523_KEY_DOWV");
                handleEvent(EventType::KEY_DOWN);
                //handleEvent(EventType::TREADMILL_INC_DOWN);
            }
            if((keys & AW9523_KEY_RIGHT) && AW9523_KEY_RIGHT)
            {
                DEBUG_PRINTLN("AW9523_KEY_RIGHT");
                handleEvent(EventType::KEY_RIGHT);
                //handleEvent(EventType::TREADMILL_SPEED_UP);
            }
            if((keys & AW9523_KEY_OK) && AW9523_KEY_OK)
            {
                DEBUG_PRINTLN("AW9523_KEY_OK");
                handleEvent(EventType::KEY_OK);
                //handleEvent(EventType::TREADMILL_START);
            }
            if((keys & AW9523_KEY_BACK) && AW9523_KEY_BACK)
            {
                DEBUG_PRINTLN("AW9523_KEY_BACK");
                handleEvent(EventType::KEY_BACK);
                //handleEvent(EventType::TREADMILL_STOP);
            }

            if((keys & 0xffc0)  != 0)  DEBUG_PRINTF("Non key pins: 0x%x\n",keys);
            prevKeys = keys;
            }
        }
    }
}

void GPIOExtenderAW9523::logPins(void)
{
    if (enabled) {
        uint8_t port0 = read(INPUT_PORT0);
        uint8_t port1 = read(INPUT_PORT1);
        DEBUG_PRINTF("GPIOExtenderAW9523 Read port:0x%x%x\n",port1,port0);
    }
}

// Currently only support one button at time
bool GPIOExtenderAW9523::pressEvent(EventType eventButton)
{
    DEBUG_PRINTF("GPIOExtenderAW9523: pressEvent(0x%x)\n",static_cast<uint32_t>(eventButton));
    if (enabled)
    {
        // convert event to HW line
        uint16_t line=-1;
        switch(eventButton)
        {
            case EventType::TREADMILL_START:      line = AW9523_TREADMILL_START; break;
            case EventType::TREADMILL_SPEED_DOWN: line = AW9523_TREADMILL_SPEED_DOWN; break;
            case EventType::TREADMILL_INC_DOWN:   line = AW9523_TREADMILL_INC_DOWN; break;
            case EventType::TREADMILL_STOP:       line = AW9523_TREADMILL_STOP; break;
            case EventType::TREADMILL_SPEED_UP:   line = AW9523_TREADMILL_SPEED_UP; break;
            case EventType::TREADMILL_INC_UP:     line = AW9523_TREADMILL_INC_UP; break;
            default:
                // Cant handle Event
                DEBUG_PRINTF("GPIOExtenderAW9523 ERROR: Unknown event 0x%x\n",static_cast<uint32_t>(eventButton));
                return false;
        }
        if (line ==-1)
        {
            DEBUG_PRINTF("GPIOExtenderAW9523 ERROR: event not converted to line 0x%x\n",static_cast<uint32_t>(eventButton));
            return false;
        }
        DEBUG_PRINTF("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x\n",static_cast<uint32_t>(eventButton),line);

        if (line & 0xff)
        {
            uint8_t port0Bit = line & 0xff;
            DEBUG_PRINTF("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x port0=0x%x ~x%x\n",static_cast<uint32_t>(eventButton),line,port0Bit,~port0Bit);

            // pinMode(pin, OUTPUT);
            if (!write(CONFIG_PORT0,~port0Bit)) return false; //0-output 1-input
            // digitalWrite(pin, LOW);
            if (!write(OUTPUT_PORT0,~port0Bit)) return false; //0-low 1-high

            delay(TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);

            // digitalWrite(pin, HIGH);
            if (!write(OUTPUT_PORT0,0xff)) return false; //0-low 1-high
            // pinMode(pin, INPUT);
            if (!write(CONFIG_PORT0,0xff)) return false; //0-output 1-input
        }
        else
        {
            uint8_t port1Bit = (line & 0xff00) >> 8;
            DEBUG_PRINTF("GPIOExtenderAW9523: pressEvent(0x%x) -> 0x%x port1=0x%x ~x%x\n",static_cast<uint32_t>(eventButton),line,port1Bit,~port1Bit);
            // pinMode(pin, OUTPUT);
            if (!write(CONFIG_PORT1,~port1Bit)) return false; //0-output 1-input
            // digitalWrite(pin, LOW);
            if (!write(OUTPUT_PORT1,~port1Bit)) return false; //0-low 1-high

            delay(TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS);

            // digitalWrite(pin, HIGH);
            if (!write(OUTPUT_PORT1,0xff)) return false; //0-low 1-high
            // pinMode(pin, INPUT);
            if (!write(CONFIG_PORT1,0xff)) return false; //0-output 1-input

        }
        return true;
    }
    return false;
}

uint8_t GPIOExtenderAW9523::read(uint8_t reg)
{
    wire->beginTransmission(AW9523_ADDR);
    wire->write(reg);
    wire->endTransmission(true);
    wire->requestFrom(AW9523_ADDR, static_cast<uint8_t>(1));
    return wire->read();
}

bool GPIOExtenderAW9523::write(uint8_t reg, uint8_t data)
{
    wire->beginTransmission(AW9523_ADDR);
    wire->write(reg);
    wire->write(data);
    uint8_t ret = wire->endTransmission();  // 0 if OK
    if (ret != 0)
    {
        DEBUG_PRINTF("GPIOExtenderAW9523 ERROR: write reg: 0x%x value:0x%x -> Error: 0x%x\n",reg, data, ret);
        return false;
    }
    return true;
}

bool GPIOExtenderAW9523::begin()
{
    enabled = false;
    isInterrupted = false;
    uint8_t id = read(ID);
    DEBUG_PRINTF("GPIOExtenderAW9523: Read ID: 0x%x = 0x%x\n",ID,id);

    if (id != ID_AW9523)
    {
      return false;
    }

    if (!write(OUTPUT_PORT0,0xff)) return false; //0-low 1-high
    if (!write(OUTPUT_PORT1,0xff)) return false; //0-low 1-high

    // All input
    if (!write(CONFIG_PORT0,0xff)) return false; //0-output 1-input
    if (!write(CONFIG_PORT1,0xff)) return false; //0-output 1-input

    if (!write(CTL,0x08)) return false; //0000x000  0-Open Drain 1-Push-Pull

#ifdef AW9523_IRQ_MODE
    if (!write(INT_PORT0,0x00)) return false; //0-enable 1-disable
    if (!write(INT_PORT1,0x00)) return false; //0-enable 1-disable
#else
    if (!write(INT_PORT0,0xff)) return false; //0-enable 1-disable
    if (!write(INT_PORT1,0xff)) return false; //0-enable 1-disable
#endif
    if (!write(LED_MODE_SWITCH0,0xff)) return false; //0-LED mode 1-GPIO mode
    if (!write(LED_MODE_SWITCH1,0xff)) return false; //0-LED mode 1-GPIO mode
    enabled = true;
    getPins();
    return true;
}

bool GPIOExtenderAW9523::isAvailable()
{
    return enabled;
}

uint16_t GPIOExtenderAW9523::getPins(void)
{
    if (enabled)
    {
      isInterrupted = false;
      uint8_t port0 = read(INPUT_PORT0);
      uint8_t port1 = read(INPUT_PORT1);

#ifdef AW9523_IRQ_MODE
      uint8_t int0 = read(INT_PORT0);
      uint8_t int1 = read(INT_PORT0);
      DEBUG_PRINTF("GPIOExtenderAW9523 Read port:0x%x%x  interrupt:0x%x%x \n",port1,port0,int0,int1);
#endif
      return ((port1 << 8) | port0);
    }
    return 0;
}  

bool GPIOExtenderAW9523::checkInterrupt(void)
{
    return (enabled && isInterrupted);
}

#ifdef AW9523_IRQ_MODE
void IRAM_ATTR GPIOExtenderAW9523::gotInterrupt(void)
{
    DEBUG_PRINTLN("AW9523 Interrupt");

    if (enabled)
    {
      // let set this and poll this bit later to avoid I2C reads in the interrupt routine
      isInterrupted = true;
      // TODO Is it ok to read I2C here? If so we can rework this for simpler code
      //      to maybe avoid the poll in loop()
    }
}
#endif

void initHardware(void)
{
  logText("init Hardware...");  
  reset_reason = esp_reset_reason();
#if defined(SPEED_IR_SENSOR1) && defined(SPEED_IR_SENSOR2)
  // pinMode(SPEED_IR_SENSOR1, INPUT_PULLUP); //TODO used?
  // pinMode(SPEED_IR_SENSOR1, INPUT_PULLUP); //TODO used?
  attachInterrupt(SPEED_IR_SENSOR1, speedSensor1_ISR, FALLING); //TODO used?
  attachInterrupt(SPEED_IR_SENSOR2, speedSensor2_ISR, FALLING); //TODO used?
#endif

  // note: I have 10k pull-up on SPEED_REED_SWITCH_PIN
  pinMode(SPEED_REED_SWITCH_PIN, INPUT);
  attachInterrupt(digitalPinToInterrupt(SPEED_REED_SWITCH_PIN), reedSwitch_ISR, FALLING);

  I2C_0.begin(SDA_0, SCL_0, I2C_FREQ);
  
  initGPIOExtender();
  logText("done\n");
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
