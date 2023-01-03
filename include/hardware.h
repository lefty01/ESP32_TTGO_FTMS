#ifndef _HARDWARE_H_
#define _HARDWARE_H_

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

#pragma once

#include <Wire.h>

static constexpr int TREADMILL_BUTTON_PRESS_SIGNAL_TIME_MS = 250;
extern volatile long workoutDistance;
extern bool timer_tick;


class GPIOExtenderAW9523
{
public:
  GPIOExtenderAW9523(TwoWire &w)
  {
    enabled = false;
    isInterrupted = false;
    wire = &w;
  }

  bool begin();
  bool isAvailable();
  uint16_t getPins(void);
  bool checkInterrupt(void);
  void scanButtons(void);
  void logPins(void);
  bool pressEvent(EventType eventButton);
private:
  uint8_t read(uint8_t reg);
  uint8_t read(uint8_t reg, bool i2cHandled);
  bool write(uint8_t reg, uint8_t data);
  bool enabled = false;
  bool isInterrupted = false;
  TwoWire *wire;
  static constexpr uint8_t AW9523_ADDR = 0x5B;
  static constexpr uint8_t INPUT_PORT0 = 0x00; //P0 port input state
  static constexpr uint8_t INPUT_PORT1 = 0x01; //P1 port input state
  static constexpr uint8_t OUTPUT_PORT0 = 0x02; //P0 port output state
  static constexpr uint8_t OUTPUT_PORT1 = 0x03; //P1 port output state
  static constexpr uint8_t CONFIG_PORT0 = 0x04; //P0 port direction configure
  static constexpr uint8_t CONFIG_PORT1 = 0x05; //P1 port direction configure
  static constexpr uint8_t INT_PORT0 = 0x06; //P0 port interrupt enable
  static constexpr uint8_t INT_PORT1 = 0x07; //P1 port interrupt enable
  static constexpr uint8_t ID = 0x10;
  static constexpr uint8_t CTL = 0x11; //Global control register
  static constexpr uint8_t LED_MODE_SWITCH0 = 0x12; //P0 port mode configure
  static constexpr uint8_t LED_MODE_SWITCH1 = 0x13; //P1 port mode configure
  static constexpr uint8_t DIM0 = 0x20; //P1_0 LED current control
  static constexpr uint8_t DIM1 = 0x21; //P1_1 LED current control
  static constexpr uint8_t DIM2 = 0x22; //P1_2 LED current control
  static constexpr uint8_t DIM3 = 0x23; //P1_3 LED current control
  static constexpr uint8_t DIM4 = 0x24; //P0_0 LED current control
  static constexpr uint8_t DIM5 = 0x25; //P0_1 LED current control
  static constexpr uint8_t DIM6 = 0x26; //P0_2 LED current control
  static constexpr uint8_t DIM7 = 0x27; //P0_3 LED current control
  static constexpr uint8_t DIM8 = 0x28; //P0_4 LED current control
  static constexpr uint8_t DIM9 = 0x29; //P0_5 LED current control
  static constexpr uint8_t DIM10 = 0x2A; //P0_6 LED current control
  static constexpr uint8_t DIM11 = 0x2B; //P0_7 LED current control
  static constexpr uint8_t DIM12 = 0x20; //P1_4 LED current control
  static constexpr uint8_t DIM13 = 0x21; //P1_5 LED current control
  static constexpr uint8_t DIM14 = 0x22; //P1_6 LED current control
  static constexpr uint8_t DIM15 = 0x23; //P1_7 LED current control
  static constexpr uint8_t SW_RSTN = 0x7F;
  static constexpr uint8_t ID_AW9523 = 0x23;
  static constexpr uint16_t AW9523_KEY_UP    = 0x0001;
  static constexpr uint16_t AW9523_KEY_LEFT  = 0x0002;
  static constexpr uint16_t AW9523_KEY_DOWN  = 0x0004;
  static constexpr uint16_t AW9523_KEY_RIGHT = 0x0008;
  static constexpr uint16_t AW9523_KEY_OK    = 0x0010;
  static constexpr uint16_t AW9523_KEY_BACK  = 0x0020;
  static constexpr uint16_t AW9523_TREADMILL_REED       = 0x0100;

// These are used to control/override the Treadmil, e.g. pins are connected to
// the different button so that software can "press" them
// TODO: They could also be used to read pins, to see if user presses them
  static constexpr uint16_t AW9523_TREADMILL_START      = 0x0200;
  static constexpr uint16_t AW9523_TREADMILL_SPEED_DOWN = 0x0400;
  static constexpr uint16_t AW9523_TREADMILL_INC_DOWN   = 0x0800;
  static constexpr uint16_t AW9523_TREADMILL_INC_UP     = 0x1000;
  static constexpr uint16_t AW9523_TREADMILL_SPEED_UP   = 0x2000;
  static constexpr uint16_t AW9523_TREADMILL_STOP       = 0x4000;
};

void initHardware(void);
void initSensors(void);
void initButton(void);
void loopHandleHardware(void);
void loopHandleButton(void);
void handleEvent(EventType event);
float calculateRPM(void);
float getIncline(void);
extern esp_reset_reason_t reset_reason;
extern const char* getRstReason(esp_reset_reason_t r);
extern TwoWire I2C_0;
extern GPIOExtenderAW9523 GPIOExtender;


#endif