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
#ifndef NO_DISPLAY
  logText("Init display...");    
  tft.init();
#ifdef TFT_ROTATE
  tft.setRotation(TFT_ROTATE);
#else
  tft.setRotation(1);
#endif
#ifdef TFT_BL
  if (TFT_BL > 0) {
    // Backlight On
    pinMode(TFT_BL, OUTPUT);
    digitalWrite(TFT_BL, HIGH);
  }
#endif

  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_BLUE);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("Setup Started");

#if defined(HAS_TOUCH_DISPLAY) && defined(TOUCH_CALLIBRATION_AT_STARTUP)
  if (tft.touch()) {
    tft.setTextFont(4);
    tft.setCursor(20, tft.height()/2);
    tft.println("Press corner near arrow to callibrate touch");
    tft.setCursor(0, 0);
    tft.calibrateTouch(nullptr, TFT_WHITE, TFT_BLACK, std::max(tft.width(), tft.height()) >> 3);
  }
#endif
  delay(3000);
  logText("done\n");  
#else
  logText("No display build\n");
#endif  
}

void showInfo() {
#ifndef NO_DISPLAY    
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(2);
  tft.setCursor(5, 5);
  tft.printf("ESP32 FTMS - %s - %s\nSpeed[%.2f-%.2f] Incline[%.2f-%.2f]\nDist/REED:%limm\nREED:%d MPU6050:%d VL53L0X:%d IrSense:%d\nGPIOExtender(AW9523):%d\n",
              VERSION,configTreadmill.treadmill_name,
              configTreadmill.min_speed,configTreadmill.max_speed,configTreadmill.min_incline,configTreadmill.max_incline,configTreadmill.belt_distance,
              configTreadmill.hasReed,configTreadmill.hasMPU6050, configTreadmill.hasVL53L0X, configTreadmill.hasIrSense, GPIOExtender.isAvailable());
  delay(4000);              
#endif
  DEBUG_PRINTF("ESP32 FTMS - %s - %s\nSpeed[%.2f-%.2f] Incline[%.2f-%.2f]\nDist/REED:%limm\nREED:%d MPU6050:%d VL53L0X:%d IrSense:%d\nGPIOExtender(AW9523):%d\n",
              VERSION,configTreadmill.treadmill_name,
              configTreadmill.min_speed,configTreadmill.max_speed,configTreadmill.min_incline,configTreadmill.max_incline,configTreadmill.belt_distance,
              configTreadmill.hasReed,configTreadmill.hasMPU6050, configTreadmill.hasVL53L0X, configTreadmill.hasIrSense, GPIOExtender.isAvailable());

}

void loopHandleTouch(void) {
#ifndef NO_DISPLAY    
#if defined (HAS_TOUCH_DISPLAY)
  int32_t touch_x = 0, touch_y = 0;

  //DEBUG_PRINTLN("loop handle touch..");

  // Warning/Info to avoid TwoWire and LovyanGFX I2C collition make sure to fullt init/deinit I2C driver states
  // around the tft.getTouch() call
  lgfx::i2c::init(I2C_NUM_1, GPIO_NUM_18, GPIO_NUM_19);

  tft.getTouch(&touch_x, &touch_y);

  lgfx::i2c::release(I2C_NUM_1);

  //DEBUG_PRINTF("loopHandleTouch() -> %d,%d\n",touch_x,touch_y);

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
    DEBUG_PRINTLN("loopHandleTouch() speed mode toggle!");
    speedInclineMode ^= SPEED; // b'01 toggle bit
    if (speedInclineMode & SPEED) btnSpeedToggle.drawButton();
    else                          btnSpeedToggle.drawButton(true);
    gfxUpdateHeader();
  }
  if (btnInclineToggle.justPressed()) {
    DEBUG_PRINTLN("loopHandleTouch() incline mode toggle!");
    speedInclineMode ^= INCLINE; // b'10
    if (speedInclineMode & INCLINE) btnInclineToggle.drawButton();
    else                            btnInclineToggle.drawButton(true);
    gfxUpdateHeader();
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

#endif
#endif //#ifndef NO_DISPLAY    

}
