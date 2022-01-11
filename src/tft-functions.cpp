#include "common.h"


// with tft.setRotation(1); => landscape orientation (usb right side)
const int DRAW_WIDTH  = TFT_HEIGHT;
const int DRAW_HEIGHT = 128;  // TFT_WIDTH

// how about sprites?
// Sprite with 8 bit colour depth the RAM needed is (width x height) bytes
// on larger displays maybe show incline profile or something
// pace over time ... etc
// gauge?

// add support for nextion display ... since I have one I'lll give it a try ;)

void initSPIFFSDone()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initSPIFFS Done!");
}


void updateDisplay(bool clear)
{
  if (clear) {
    tft.fillScreen(TFT_BLACK);
  }

  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);

  if (clear) {
    tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
    tft.drawFastVLine(DRAW_WIDTH / 2, 22,                   DRAW_HEIGHT - 22, TFT_WHITE);
    tft.drawFastHLine(1,              DRAW_HEIGHT / 2 + 20, DRAW_WIDTH  -  2, TFT_WHITE);
  }

  tft.setTextFont(4);

  // speed top left
  tft.fillRect(0,                 30, 118, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT / 2 - 36);
  tft.setTextFont(2);
  tft.print("Speed:");  //TODO move labels printing to outside "draw everytime" code
  tft.setCursor(4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(kmph);

  // incline top right
  tft.fillRect(DRAW_WIDTH / 2 + 2, 30, 118, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 36);
  tft.setTextFont(2);
  tft.print("Incline:");
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(incline);

  // dist bot left
  tft.fillRect(0, DRAW_HEIGHT - 40, 118, 40, TFT_BLACK);
  tft.setCursor(4, DRAW_HEIGHT - 38);
  tft.setTextFont(2);
  tft.print("Dist:");
  tft.setCursor(4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println(total_distance/1000);

  // elevation bot right
  tft.fillRect(DRAW_WIDTH / 2 + 2, DRAW_HEIGHT - 40, 118, 40, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 38);
  tft.setTextFont(2);
  tft.print("Elev.gain:");
  tft.setCursor(DRAW_WIDTH / 2 + 4, DRAW_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println((uint32_t)elevation_gain);
}

void updateBTConnectionStatus(bool connected)
{
  if (connected) {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
  else {
    tft.fillCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_BLACK);
    tft.drawCircle(CIRCLE_BT_STAT_X_POS, CIRCLE_Y_POS, CIRCLE_RADIUS, TFT_SKYBLUE);
  }
}


void showSpeedInclineMode(uint8_t mode)
{
  // clear upper status line
  tft.fillRect(2, 2, DRAW_WIDTH-2, 18, TFT_BLACK);
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


void show_FPS(int fps){
  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);
  tft.fillRect(DRAW_WIDTH-10*8, DRAW_HEIGHT-18, 10*8, DRAW_HEIGHT, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH-10*8,DRAW_HEIGHT-18);
  tft.printf("fps:%03d", fps);
}


void show_WIFI(const unsigned long reconnect_counter, const String &ip) {
  // show reconnect counter in tft
  // if (wifi_reconnect_counter > wifi_reconnect_counter_prev) ... only update on change
  tft.fillRect(2, 2, 60, 18, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(3, 4);
  tft.print("Wifi[");
  tft.print(reconnect_counter);
  tft.print("]: ");
  tft.print(ip);
}
