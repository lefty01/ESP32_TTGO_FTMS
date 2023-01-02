#include <Arduino.h>
#include "common.h"
#include "debug_print.h"
#include "hardware.h"
#include "display.h"
#include "config.h"

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
  logText("Init display\n");
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

#ifdef GUI_LVGL
  logText("Init LVGL display\n");
  lvgl_initDisplay();
#else
  logText("Init LovyanGFX display\n");
  tft_initDisplay();
#endif
#else
  logText("No display build\n");
#endif
}

void loopHandleGfx(void)
{
#ifndef NO_DISPLAY

#ifdef GUI_LVGL
  lvgl_loopHandleGfx();
#else
  tft_loopHandleGfx();
#endif

#endif
}

void gfxLogText(const char *text)
{
#ifndef NO_DISPLAY

#ifdef GUI_LVGL
  lvgl_gfxLogText(text);
#else
  tft_gfxLogText(text);
#endif

#endif
}


void gfxUpdateDisplay(bool clear)
{
#ifndef NO_DISPLAY
#ifdef GUI_LVGL
  lvgl_gfxUpdateDisplay(clear);
#else
  tft_gfxUpdateDisplay(clear);
#endif

#endif
}



void gfxUpdateHeader()
{
#ifndef NO_DISPLAY
#ifdef GUI_LVGL
  lvgl_gfxUpdateHeader();
#else
  tft_gfxUpdateHeader();
#endif

#endif
}
