#include <Arduino.h>
#include "ESP32_TTGO_FTMS.h"
#include "debug_print.h"
#include "hardware.h"
#include "display.h"
#include "config.h"

volatile bool touch_b1 = false;
volatile bool touch_b2 = false;
volatile bool touch_b3 = false;

#ifndef NO_DISPLAY
#if LGFX_USE_V1
LGFX tft;
LGFX_Sprite sprite(&tft);
#elif USE_TFT_ESPI
TFT_eSPI tft = TFT_eSPI();
#endif
#endif

void initDisplay(void)
{
  logText("Init display...");    
#ifndef NO_DISPLAY  
  tft.init(); // vs begin??
  tft.setRotation(1); // 3
#endif  
#ifdef TFT_ROTATE
  tft.setRotation(TFT_ROTATE);
#endif
#ifdef TFT_BL
  if (TFT_BL > 0) {
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
#endif
#ifndef NO_DISPLAY  
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");
#endif

#if defined(HAS_TOUCH_DISPLAY) && defined(TOUCH_CALLIBRATION_AT_STARTUP)
  if (tft.touch()) {
    tft.setTextFont(4);
    tft.setCursor(20, tft.height()/2);
    tft.println("Press corner near arrow to callibrate touch");
    tft.setCursor(0, 0);
    tft.calibrateTouch(nullptr, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
  }
#endif
#ifndef NO_DISPLAY 
  delay(3000);
#endif  
  logText("done\n");  
}

void showInfo() {
#ifndef NO_DISPLAY    
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(2);
  tft.setCursor(5, 5);
  tft.printf("ESP32 FTMS - %s - %s\nSpeed[%.2f-%.2f] Incline[%.2f-%.2f]\nDist/REED:%limm\nREED:%d MPU6050:%d VL53L0X:%d IrSense:%d\nGPIOExtender(AW9523):%d\n",
              VERSION,treadmill_name,
              min_speed,max_speed,min_incline,max_incline,belt_distance,
              hasReed,hasMPU6050, hasVL53L0X, hasIrSense, GPIOExtender.isAvailable());
  delay(4000);              
#endif
  DEBUG_PRINTF("ESP32 FTMS - %s - %s\nSpeed[%.2f-%.2f] Incline[%.2f-%.2f]\nDist/REED:%limm\nREED:%d MPU6050:%d VL53L0X:%d IrSense:%d\nGPIOExtender(AW9523):%d\n",
              VERSION,treadmill_name,
              min_speed,max_speed,min_incline,max_incline,belt_distance,
              hasReed,hasMPU6050, hasVL53L0X, hasIrSense, GPIOExtender.isAvailable());

}

void loop_handle_touch(void) {
#if defined (HAS_TOUCH_DISPLAY)
  int32_t touch_x = 0, touch_y = 0;

 // DEBUG_PRINTLN("loop handle touch..");
  tft.getTouch(&touch_x, &touch_y);

  // FIXME: switch to button array (touchButtons[]) and cycle through buttons here
  if (btnSpeedToggle.contains(touch_x, touch_y)) btnSpeedToggle.press(true);
  else                                           btnSpeedToggle.press(false);

  if (btnInclineToggle.contains(touch_x, touch_y)) btnInclineToggle.press(true);
  else                                             btnInclineToggle.press(false);

  if (btnSpeedUp.contains(touch_x, touch_y)) btnSpeedUp.press(true);
  else                                       btnSpeedUp.press(false);

  if (btnSpeedDown.contains(touch_x, touch_y)) btnSpeedDown.press(true);
  else                                         btnSpeedDown.press(false);

  if (btnInclineUp.contains(touch_x, touch_y)) btnInclineUp.press(true);
  else                                         btnInclineUp.press(false);

  if (btnInclineDown.contains(touch_x, touch_y)) btnInclineDown.press(true);
  else                                           btnInclineDown.press(false);


  if (btnSpeedToggle.justPressed()) {
    DEBUG_PRINTLN("speed mode toggle!");
    speedInclineMode ^= SPEED; // b'01 toggle bit
    if (speedInclineMode & SPEED) btnSpeedToggle.drawButton();
    else                          btnSpeedToggle.drawButton(true);
    updateHeader();
  }
  if (btnInclineToggle.justPressed()) {
    DEBUG_PRINTLN("incline mode toggle!");
    speedInclineMode ^= INCLINE; // b'10
    if (speedInclineMode & INCLINE) btnInclineToggle.drawButton();
    else                            btnInclineToggle.drawButton(true);
    updateHeader();
  }

  if (btnSpeedUp.justPressed()) {
    btnSpeedUp.drawButton(true);
    speedUp();
  }
  if (btnSpeedUp.justReleased()) {
    btnSpeedUp.drawButton();
  }
  if (btnSpeedDown.justPressed()) {
    btnSpeedDown.drawButton(true);
    speedDown();
  }
  if (btnSpeedDown.justReleased()) {
    btnSpeedDown.drawButton();
  }

  if (btnInclineUp.justPressed()) {
    btnInclineUp.drawButton(true);
    inclineUp();
  }
  if (btnInclineUp.justReleased()) {
    btnInclineUp.drawButton();
  }
  if (btnInclineDown.justPressed()) {
    btnInclineDown.drawButton(true);
    inclineDown();
  }
  if (btnInclineDown.justReleased()) {
    btnInclineDown.drawButton();
  }

  // if (tft.touch() && ((millis() - touch_timer) > 120)) {
  //   touch_timer = millis();

  //   if (tft.getTouch(&touch_x, &touch_y)) {
  //     DEBUG_PRINTF("touch: x=%.3d y=%.3d\n", touch_x, touch_y);

  //     if ((touch_x >= 250) && (touch_x <= 300) &&
  // 	  (touch_y >=  50) && (touch_y <= 80)) {
  // 	touch_b1 = true;

  // 	speedInclineMode += 1;
  // 	speedInclineMode %= _NUM_MODES_;
  // 	DEBUG_PRINT("speedInclineMode=");
  // 	DEBUG_PRINTLN(speedInclineMode);
  // 	updateHeader();
  // 	// reset to manual mode on any touch (as for now)
  //       // if ( speedInclineMode != MANUAL) {
  //       //   kmph = 0.5;
  //       //   incline = 0;
  //       //   grade_deg = 0;
  //       //   angle = 0;
  //       //   elevation = 0;
  //       //   elevation_gain = 0;
  //       //   speedInclineMode = MANUAL;
  //       // }
  //     }
  //     //tft.fillRect(touch_x-1, touch_y-1, 3, 3, TFT_WHITE);
  //     else if ((touch_x >= 250) && (touch_x <= 300) &&
  // 	       (touch_y >=  90) && (touch_y <= 120)) {
  // 	DEBUG_PRINTLN("Touch: Button 2 inclineUp()");
  // 	inclineUp();
  //     }
  //     else if ((touch_x >= 250) && (touch_x <= 300) &&
  // 	       (touch_y >= 130) && (touch_y <= 150)) {
  // 	DEBUG_PRINTLN("Touch: Button 3 inclineDown()");
  // 	inclineDown();
  //     }
  //     else if ((touch_x >= 330) && (touch_x <= 380) &&
  // 	       (touch_y >=  90) && (touch_y <= 120)) {
  // 	DEBUG_PRINTLN("Touch: Button 4 speedUp()");
  // 	speedUp();
  //     }
  //     else if ((touch_x >= 330) && (touch_x <= 380) &&
  // 	       (touch_y >= 130) && (touch_y <= 150)) {
  // 	DEBUG_PRINTLN("Touch: Button 5 speedDown()");
  // 	speedDown();
  //     }
  //   }
  // }
#endif
}