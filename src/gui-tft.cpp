/**
 *
 *
 * The MIT License (MIT)
 * Copyright © 2021, 2022, 2025, 2026 <Andreas Loeffler>
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

#if !defined(NO_DISPLAY) && !defined(GUI_LVGL)

#include "common.h"
#include "hardware.h"
#include "net-control.h"
#include "display.h"
#include "debug_print.h"

#define TFT_SETUP_FONT_SIZE 4
#define TFT_STATS_FONT_SIZE 2

#define CIRCLE_SPEED_X_POS   188
#define CIRCLE_INCLINE_X_POS 208
#define CIRCLE_BT_STAT_X_POS 227

#define CIRCLE_SPEED_Y_POS    11
#define CIRCLE_INCLINE_Y_POS  11
#define CIRCLE_BT_STAT_Y_POS  11
#define CIRCLE_Y_POS          11
#define CIRCLE_RADIUS          8

// with tft.setRotation(1); => landscape orientation (usb right side)

#if defined(DRAW_STATS_WIDTH) && defined(DRAW_STATS_HEIGHT)
const int DRAW_WIDTH  = DRAW_STATS_WIDTH;
const int DRAW_HEIGHT = DRAW_STATS_HEIGHT;
#else
const int DRAW_WIDTH  = TFT_HEIGHT;
const int DRAW_HEIGHT = TFT_WIDTH;
#endif

void tft_gfxUpdateHeader(void);

// how about sprites?
// Sprite with 8 bit colour depth the RAM needed is (width x height) bytes
// on larger displays maybe show incline profile or something
// pace over time ... etc
// gauge?

// add support for nextion display ... since I have one I'll give it a try ;)

#ifdef HAS_TOUCH_DISPLAY
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

void initLovyanGFXTouchAreas(void)
 {
  // for (unsigned n = 0; n < NUM_TOUCH_BUTTONS; ++n) {
  //     touchButtons[n] = LGFX_Button();
  // }
  // fixme: ...
  //logText("initLovyanGFXTouchAreas...");
  btnSpeedToggle   .initButtonUL(&tft, btnSpeedToggle_X,    btnSpeedToggle_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "SPEED");
  btnInclineToggle .initButtonUL(&tft, btnInclineToggle_X,  btnInclineToggle_Y, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "INCL.");
  btnSpeedUp       .initButtonUL(&tft, btnSpeedUp_X,        btnSpeedUp_Y,       100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnSpeedDown     .initButtonUL(&tft, btnSpeedDown_X,      btnSpeedDown_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
  btnInclineUp     .initButtonUL(&tft, btnInclineUp_X,      btnInclineUp_Y,     100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "UP");
  btnInclineDown   .initButtonUL(&tft, btnInclineDown_X,    btnInclineDown_Y,   100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "DOWN");
  //modeButton.initButtonUL(&tft, 260, 5, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "MODE");
  //modeButton.drawButton();
}
#endif



void tft_initDisplay(void)
{
  tft.fillScreen(TFT_BLACK);

  tft_gfxUpdateHeader();

  // Prepare for log
  tft.setTextColor(TFT_GOLD);
  tft.setTextFont(2);
  tft.setCursor(0, 22);

#ifdef HAS_TOUCH_DISPLAY
  initLovyanGFXTouchAreas();
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
}

void tft_gfxLogText(const char *text)
{
  if (!setupDone) {
    tft.print(text);
  }
}

void tft_gfxUpdateDisplay(bool clear)
{
#ifndef NO_DISPLAY
  if (clear) {
    tft.fillScreen(TFT_BLACK);
    tft_gfxUpdateHeader();
  }
  tft.startWrite();

  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);

// FIXME: is that a "good" way to handle different (touch) screen ...!??
#if defined(TARGET_WT32_SC01)
  if (clear) {
      // create buttons
      //modeButton.initButtonUL(&tft, 260, 5, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "MODE");

  // for (uint8_t row=0; row<5; row++) {
  //     for (uint8_t col=0; col<3; col++) {
  //	  buttons[col + row*3].initButton(&tft, BUTTON_X+col*(BUTTON_W+BUTTON_SPACING_X),
  //					  BUTTON_Y+row*(BUTTON_H+BUTTON_SPACING_Y),    // x, y, w, h, outline, fill, text
  //					  BUTTON_W, BUTTON_H, ILI9341_WHITE, buttoncolors[col+row*3], ILI9341_WHITE,
  //					  buttonlabels[col + row*3], BUTTON_TEXTSIZE);
  //	  buttons[col + row*3].drawButton();
  //   }
  // }

    // draw touch control buttons within DRAW_CTRL_WIDTH/HEIGHT

    // tft.fillRect(250,  50,  50,  30, TFT_WHITE);  // mode
    // tft.fillRect(250,  90,  50,  30, TFT_BLUE);   // incline up
    // tft.fillRect(250, 130,  50,  30, TFT_YELLOW); // incline down
    // tft.fillRect(330,  90,  50,  30, TFT_BLUE);   // speed up
    // tft.fillRect(330, 130,  50,  30, TFT_YELLOW); // speed down


    btnSpeedToggle   .drawButton();
    btnInclineToggle .drawButton();
    btnSpeedUp       .drawButton();
    btnSpeedDown     .drawButton();
    btnInclineUp     .drawButton();
    btnInclineDown   .drawButton();

  }

  if (clear) {
    //tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
    tft.drawFastVLine(DRAW_WIDTH / 2, 22,                   DRAW_HEIGHT - 22, TFT_WHITE);
    tft.drawFastHLine(1,              DRAW_HEIGHT / 2 + 20, DRAW_WIDTH  -  2, TFT_WHITE);

    tft.setTextFont(2);

    tft.setCursor(4, DRAW_HEIGHT / 2 - 36);
    tft.print("Speed:");

    tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 36);
    tft.print("Incline:");

    tft.setCursor(4, DRAW_HEIGHT - 42);
    tft.print("Dist:");

    tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 42);
    tft.print("Elev.gain:");
  }

  tft.setTextFont(4);

  // speed top left
  tft.fillRect(0, DRAW_HEIGHT / 2 - 20, DRAW_WIDTH / 4 + 10, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(kmph);

  // incline top right
  tft.fillRect(DRAW_WIDTH / 2 + 2, DRAW_HEIGHT / 2 - 20, DRAW_WIDTH / 4 + 10, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(incline);

  // dist bot left
  tft.fillRect(0, DRAW_HEIGHT - 26, DRAW_WIDTH / 4 + 10, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println(totalDistance/1000);

  // elevation bot right
  tft.fillRect(DRAW_WIDTH / 2 + 2, DRAW_HEIGHT - 26, DRAW_WIDTH / 4 + 10, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println((uint32_t)elevationGain);
#else
  if (clear) {
    //tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
    tft.drawFastVLine(DRAW_WIDTH / 2, 22,                   DRAW_HEIGHT - 22, TFT_WHITE);
    tft.drawFastHLine(1,              DRAW_HEIGHT / 2 + 20, DRAW_WIDTH  -  2, TFT_WHITE);

    tft.setTextFont(2);

    tft.setCursor(4, DRAW_HEIGHT / 2 - 36);
    tft.print("Speed:");

    tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 36);
    tft.print("Incline:");

    tft.setCursor(4, DRAW_HEIGHT - 42);
    tft.print("Dist:");

    tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 42);
    tft.print("Elev.gain:");
  }

  tft.setTextFont(4);

  // speed top left
  tft.fillRect(0, DRAW_HEIGHT / 2 - 20, DRAW_WIDTH / 4, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(kmph);

  // incline top right
  tft.fillRect(DRAW_WIDTH / 2 + 2, DRAW_HEIGHT / 2 - 20, DRAW_WIDTH / 4, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(incline);

  // dist bot left
  tft.fillRect(0, DRAW_HEIGHT - 26, DRAW_WIDTH / 4, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println(totalDistance/1000);

  // elevation bot right
  tft.fillRect(DRAW_WIDTH / 2 + 2, DRAW_HEIGHT - 26, DRAW_WIDTH / 4, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println((uint32_t)elevationGain);
#endif // display TARGET
  tft.endWrite();
#endif // NO_DISPLAY
}



void tft_loopHandleGfx(void)
{
#ifndef NO_DISPLAY
#if defined (HAS_TOUCH_DISPLAY)
  int32_t touch_x = 0, touch_y = 0;

  //logText("loop handle touch..\n");

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
    logText("loopHandleTouch() speed mode toggle!\n");
    speedInclineMode ^= SPEED; // b'01 toggle bit
    if (speedInclineMode & SPEED) btnSpeedToggle.drawButton();
    else                          btnSpeedToggle.drawButton(true);
    tft_gfxUpdateHeader();
  }
  if (btnInclineToggle.justPressed()) {
    logText("loopHandleTouch() incline mode toggle!\n");
    speedInclineMode ^= INCLINE; // b'10
    if (speedInclineMode & INCLINE) btnInclineToggle.drawButton();
    else                            btnInclineToggle.drawButton(true);
    tft_gfxUpdateHeader();
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


#ifndef NO_DISPLAY

static void tft_gfxUpdateHeaderWIFIStatus(const unsigned long reconnect_counter, const String &ip) {
  //tft.fillRect(2, 2, 60, 18, TFT_BLACK);
  int32_t cursorX = tft.getCursorX();
  int32_t cursorY = tft.getCursorY();
  //int32_t font = tft.getTextFont();
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(3, 4);
  tft.print("Wifi[");
  tft.print(reconnect_counter);
  tft.print("]: ");
  tft.print(ip.c_str());

  tft.setCursor(cursorX, cursorY);
  //tft.setTextFont(font);
}

static void tft_gfxUpdateHeaderBTStatus(bool connected)
{
  if (connected) {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
  else {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_BLACK);
    tft.drawCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
}

static void tft_gfxUpdateHeaderSpeedInclineMode(uint8_t mode)
{
  // clear upper status line
  // TODO trim this to size
  //      tft.fillRect(2, 2, DRAW_WIDTH-2, 18, TFT_BLACK);

  //tft.setTextColor(TFT_ORANGE);
  //tft.setTextFont(2);
  // tft.setCursor(170, 4);
  // tft.print(mode);

  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED) {
      tft.fillCircle(CIRCLE_SPEED_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_GREEN);
  }
  else if ((mode & SPEED) == 0) {
    tft.fillCircle(CIRCLE_SPEED_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_RED);
  }

  if (mode & INCLINE) {
      tft.fillCircle(CIRCLE_INCLINE_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_GREEN);
  }
  else if ((mode & INCLINE) == 0) {
    tft.fillCircle(CIRCLE_INCLINE_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_RED);
  }
}
#endif // NO_DISPLAY

void tft_gfxUpdateHeader()
{
#ifndef NO_DISPLAY
  tft.startWrite();
  // clear upper status line
  tft.fillRect(0, 0, DRAW_WIDTH, 21, TFT_BLACK);
  tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);

  tft_gfxUpdateHeaderWIFIStatus(wifiReconnectCounter, getWifiIpAddr());
  // indicate bt connection status ... offline
  tft_gfxUpdateHeaderBTStatus(bleClientConnected);
  // indicate manual/auto mode (green=auto/sensor, red=manual)
  tft_gfxUpdateHeaderSpeedInclineMode(speedInclineMode);
  tft.endWrite();
#endif // NO_DISPLAY
}
#endif // #if !defined(NO_DISPLAY) && !defined(GUI_LVGL)
