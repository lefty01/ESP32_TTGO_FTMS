/**
 *
 *
 * The MIT License (MIT)
 * Copyright © 2021, 2022, 2025, 2026 <Andreas Loeffler>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the “Software”), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.
 *
 * THE SOFTWARE IS PROVIDED “AS IS”, WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#include "display.h"
#include "common.h"
#include "config.h"
#include "debug_print.h"
#include "hardware.h"
#include <Arduino.h>

#ifndef NO_DISPLAY
#if LGFX_USE_V1
LGFX tft;
LGFX_Sprite sprite(&tft);
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
