#ifndef _DISPLAY_H_
#define _DISPLAY_H_

// display is configured within platformio.ini
#ifndef TFT_WIDTH
#define TFT_WIDTH  135
#endif
#ifndef TFT_HEIGHT
#define TFT_HEIGHT 240
#endif

void initDisplay(void);
void loopHandleTouch(void);
void showInfo(void);

#endif