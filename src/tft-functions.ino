//const int BORDER = 2;
//const int HEADER = 16; // percent
//const int HEADER_PX = HEADER * tft.height();
//const int X_05 = tft.width()  / 2;
//const int Y_05 = tft.height() / 2;

const int DRAW_WIDTH = TFT_HEIGHT;
const int DRAW_HEIGHT = 128; //TFT_WIDTH

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

  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);
  tft.setCursor(229+15, 4);

  tft.print(speedInclineMode);
  // if (set_speed) tft.print("SPEED");
  // else           tft.print("INCLINE");

  if (clear) {
    tft.drawRect(1, 1, DRAW_WIDTH - 2, 20, TFT_GREENYELLOW);
    tft.drawFastVLine(DRAW_WIDTH / 2, 22, DRAW_HEIGHT - 22, TFT_WHITE);
    tft.drawFastHLine(1, DRAW_HEIGHT / 2 + 20, DRAW_WIDTH - 2, TFT_WHITE);
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

void updateBTConnectionStatus()
{
}


void showSpeedInclineMode(uint8_t mode)
{
  // clear upper status line
  tft.fillRect(2, 2, DRAW_WIDTH-2, 18, TFT_BLACK);
  tft.setTextColor(TFT_WHITE);
  tft.setTextFont(2);
  tft.setCursor(229+15, 4);

  tft.print(mode);

  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED) {
      tft.fillCircle(190, 11, 8, TFT_GREEN);
  }
  else if((mode & SPEED) == 0) {
    tft.fillCircle(190, 11, 8, TFT_RED);
  }
  if (mode & INCLINE) {
      tft.fillCircle(210, 11, 8, TFT_GREEN);
  }
  else if ((mode & INCLINE) == 0) {
    tft.fillCircle(210, 11, 8, TFT_RED);
  }
}

void show_FPS(int fps){
  tft.setTextColor(TFT_ORANGE);
  tft.setTextFont(2);
  tft.fillRect(DRAW_WIDTH-10*8, DRAW_HEIGHT-18, 10*8, DRAW_HEIGHT, TFT_BLACK);
  tft.setCursor(DRAW_WIDTH-10*8,DRAW_HEIGHT-18);
  tft.printf("fps:%03d", fps);
}
