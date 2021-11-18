const int BORDER = 2;
const int HEADER = 16; // percent
const int HEADER_PX = HEADER * tft.height();

const int X_05 = tft.width()  / 2;
const int Y_05 = tft.height() / 2;


void initSPIFFSDone()
{
  tft.fillScreen(TFT_BLACK);
  tft.setTextColor(TFT_GREEN);
  tft.setTextFont(4);
  tft.setCursor(20, 40);
  tft.println("initSPIFFS Done!");


    // tft.setTextSize(2);
    // tft.setTextColor(TFT_GREEN, TFT_BLACK);
    // tft.setTextDatum(MC_DATUM);
    // tft.fillScreen(TFT_BLACK);

    // tft.setTextColor(TFT_YELLOW, TFT_BLACK);
    // tft.drawString("Button 0 Long Click Handler", x - 20, y + 30);

}


void updateDisplay(bool clear)
{
  if (clear) {
    tft.fillScreen(TFT_BLACK);
  }

  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(100, 4);

  tft.print(speedInclineMode);
  // if (set_speed) tft.print("SPEED");
  // else           tft.print("INCLINE");

  if (clear) {
    tft.drawRect(1, 1, 238, 20, TFT_GREENYELLOW);
    tft.drawFastVLine(TFT_WIDTH / 2, 22, TFT_HEIGHT - 22, TFT_WHITE);
    tft.drawFastHLine(1, TFT_HEIGHT / 2 + 20, 238, TFT_WHITE);
  }

  tft.setTextFont(4);

  // speed top left
  tft.fillRect(0,                 30, 118, 40, TFT_BLACK);
  tft.setCursor(4, TFT_HEIGHT / 2 - 36);
  tft.setTextFont(2);
  tft.print("Speed:");  //TODO move labels printing to outside "draw everytime" code
  tft.setCursor(4, TFT_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(kmph);

  // incline top right
  tft.fillRect(TFT_WIDTH / 2 + 2, 30, 118, 40, TFT_BLACK);
  tft.setCursor(TFT_WIDTH / 2 + 4, TFT_HEIGHT / 2 - 36);
  tft.setTextFont(2);
  tft.print("Incline:");
  tft.setCursor(TFT_WIDTH / 2 + 4, TFT_HEIGHT / 2 - 18);
  tft.setTextFont(4);
  tft.println(incline);

  // dist bot left
  tft.fillRect(0, TFT_HEIGHT - 40, 118, 40, TFT_BLACK);
  tft.setCursor(4, TFT_HEIGHT - 38);
  tft.setTextFont(2);
  tft.print("Dist:");
  tft.setCursor(4, TFT_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println(total_distance/1000);

  // elevation bot right
  tft.fillRect(TFT_WIDTH / 2 + 2, TFT_HEIGHT - 40, 118, 40, TFT_BLACK);
  tft.setCursor(TFT_WIDTH / 2 + 4, TFT_HEIGHT - 38);
  tft.setTextFont(2);
  tft.print("Elev.gain:");
  tft.setCursor(TFT_WIDTH / 2 + 4, TFT_HEIGHT - 20);
  tft.setTextFont(4);
  tft.println((uint32_t)elevation_gain);
}

void updateBTConnectionStatus()
{
}


void showSpeedInclineMode(uint8_t mode)
{
  // clear upper status line
  tft.fillRect(2, 2, 200, 18, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(100, 4);

  tft.print(mode);

  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED) {
      tft.fillCircle(190, 11, 9, TFT_GREEN);
  }
  else if((mode & SPEED) == 0) {
    tft.fillCircle(190, 11, 9, TFT_RED);
  }
  if (mode & INCLINE) {
      tft.fillCircle(210, 11, 9, TFT_GREEN);
  }
  else if ((mode & INCLINE) == 0) {
    tft.fillCircle(210, 11, 9, TFT_RED);
  }
}
