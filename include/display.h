#ifndef _DISPLAY_H_
#define _DISPLAY_H_

// display is configured within platformio.ini
#ifndef TFT_WIDTH
#define TFT_WIDTH  135
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif

#ifndef NO_DISPLAY
#ifdef LGFX_USE_V1
#define LGFX_AUTODETECT
#include <LovyanGFX.hpp>
extern LGFX tft;
#ifdef HAS_TOUCH_DISPLAY
extern LGFX_Button touchButtons[];
extern LGFX_Button btnSpeedToggle;
extern LGFX_Button btnInclineToggle;
extern LGFX_Button btnSpeedUp;
extern LGFX_Button btnSpeedDown;
extern LGFX_Button btnInclineUp;
extern LGFX_Button btnInclineDown;
#endif
#endif
#endif

void initDisplay(void);
void loopHandleGfx(void);

void gfxLogText(const char *text);
void gfxUpdateDisplay(bool clear);
void gfxUpdateHeader();

//TODO make this some sort of class? LVGL or tft?  or maybe we can remove tft
#ifdef GUI_LVGL
void lvgl_initDisplay(void);
void lvgl_loopHandleGfx(void);
void lvgl_gfxLogText(const char *text);
void lvgl_gfxUpdateDisplay(bool clear);
void lvgl_gfxUpdateHeader(void);
#else
void tft_initDisplay(void);
void tft_loopHandleGfx(void);
void tft_gfxLogText(const char *text);
void tft_gfxUpdateDisplay(bool clear);
void tft_gfxUpdateHeader(void);
#endif

#endif
