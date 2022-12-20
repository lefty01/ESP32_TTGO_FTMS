#ifndef NO_DISPLAY
#include "ESP32_TTGO_FTMS.h"
#include "hardware.h"
#include "net-control.h"
#include "display.h"

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


// how about sprites?
// Sprite with 8 bit colour depth the RAM needed is (width x height) bytes
// on larger displays maybe show incline profile or something
// pace over time ... etc
// gauge?

// add support for nextion display ... since I have one I'll give it a try ;)

void gfxUpdateDisplay(bool clear)
{
#ifndef NO_DISPLAY
  if (clear) {
    tft.fillScreen(TFT_BLACK);
    gfxUpdateHeader();
  }

  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);

// FIXME: is that a "good" way to handle different (touch) screen ...!??
#if defined(TARGET_WT32_SC01)
  if (clear) {
      // create buttons
      //modeButton.initButtonUL(&tft, 260, 5, 100, 50, TFT_WHITE, TFT_BLUE, TFT_WHITE, "MODE");

  // for (uint8_t row=0; row<5; row++) {
  //     for (uint8_t col=0; col<3; col++) {
  // 	  buttons[col + row*3].initButton(&tft, BUTTON_X+col*(BUTTON_W+BUTTON_SPACING_X),
  // 					  BUTTON_Y+row*(BUTTON_H+BUTTON_SPACING_Y),    // x, y, w, h, outline, fill, text
  // 					  BUTTON_W, BUTTON_H, ILI9341_WHITE, buttoncolors[col+row*3], ILI9341_WHITE,
  // 					  buttonlabels[col + row*3], BUTTON_TEXTSIZE);
  // 	  buttons[col + row*3].drawButton();
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
    tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
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
    tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
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
#endif // NO_DISPLAY
}

#ifndef NO_DISPLAY

static void gfxUpdateHeaderWIFIStatus(const unsigned long reconnect_counter, const String &ip) {
  //tft.fillRect(2, 2, 60, 18, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(3, 4);
  tft.print("Wifi[");
  tft.print(reconnect_counter);
  tft.print("]: ");
  tft.print(ip.c_str());
}

static void gfxUpdateHeaderBTStatus(bool connected)
{
  if (connected) {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
  else {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_BLACK);
    tft.drawCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
}

static void gfxUpdateHeaderSpeedInclineMode(uint8_t mode)
{
  // clear upper status line
  // TODO trim this to size
  //      tft.fillRect(2, 2, DRAW_WIDTH-2, 18, TFT_BLACK);

  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);
  // tft.setCursor(170, 4);
  // tft.print(mode);

  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED) {
      tft.fillCircle(CIRCLE_SPEED_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_GREEN);
  }
  else if((mode & SPEED) == 0) {
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

void gfxUpdateHeader()
{
#ifndef NO_DISPLAY
  // clear upper status line
  tft.fillRect(2, 2, DRAW_WIDTH-2, 18, TFT_BLACK);

  gfxUpdateHeaderWIFIStatus(wifi_reconnect_counter, getWifiIpAddr());
  // indicate bt connection status ... offline
  gfxUpdateHeaderBTStatus(bleClientConnected);
  // indicate manual/auto mode (green=auto/sensor, red=manual)
  gfxUpdateHeaderSpeedInclineMode(speedInclineMode);
#endif // NO_DISPLAY
}
#endif