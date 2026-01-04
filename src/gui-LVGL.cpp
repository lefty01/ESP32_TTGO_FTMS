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

#if !defined(NO_DISPLAY) && defined(GUI_LVGL)

#include "common.h"
#include "config.h"
#include "display.h"
#include "net-control.h"
#include <sstream>
#include <string>

#define USE_LV_GAUGE
#include "lv_conf.h"
#include <lvgl.h>

// Setup screen resolution for LVGL
#ifndef BUFFER_LINES
#define BUFFER_LINES 8
#endif
//static const uint16_t screenWidth = tft.width();
//static const uint16_t screenHeight = tft.height();
static const uint16_t screenWidth = TFT_HEIGHT;
static const uint16_t screenHeight = TFT_WIDTH;
static lv_draw_buf_t draw_buf;
LV_ATTRIBUTE_MEM_ALIGN static lv_color_t buf[screenWidth * BUFFER_LINES * 2];

// Screens
static lv_obj_t* gfxLvScreenBoot = nullptr;
static lv_obj_t* gfxLvScreenMain = nullptr;
static lv_obj_t* gfxLvScreenCurrent = nullptr;

// Top Bar
static lv_obj_t* gfxLvTopText = nullptr;
static lv_obj_t* gfxLvVersionText = nullptr;
static lv_obj_t* gfxLvLedSpeed = nullptr;
static lv_obj_t* gfxLvLedIncline = nullptr;
static lv_obj_t* gfxLvLedBT = nullptr;

// Boot screen stuff
static lv_obj_t* gfxLvBootTopBarPlaceholder = nullptr;
static lv_obj_t* gfxLvLogTextArea = nullptr; // Show serial/debug/log output

// Main screen stuff
static lv_obj_t* gfxLvMainTopBarPlaceholder = nullptr;
static lv_obj_t* lvLabelSpeed = nullptr;
static lv_obj_t* lvLabelIncline = nullptr;
static lv_obj_t* lvLabelDistance = nullptr;
static lv_obj_t* lvLabelElevation = nullptr;

typedef struct {
  lv_obj_t* scale;
  lv_obj_t* needle;
  lv_obj_t* arc_blue;
  lv_obj_t* arc_red;
  uint32_t minVal;
  uint32_t maxVal;
} meter_t;

meter_t speedMeter;
meter_t inclineMeter;

#ifdef HAS_TOUCH_DISPLAY
static lv_obj_t* gfxLvSwitchSpeedControlMode = nullptr;
static lv_obj_t* gfxLvSwitchInclineControlMode = nullptr;
#endif

static lv_obj_t* lvGraph = nullptr;
static lv_chart_series_t* lvGraphSpeedSerie = nullptr;
static lv_chart_series_t* lvGraphInclineSerie = nullptr;
static const uint16_t graphDataPoints = 60;
static const uint16_t graphDataPointAdvanceTime = 60; //How many seconds until "scrolling" one step
static const bool graphCircular = false;

void lvgl_gfxUpdateHeader();

static void showGfxTopBar(lv_obj_t* parent)
{
  static lv_obj_t* gfxLvTopBar = nullptr;
  if (!gfxLvTopBar) {

    gfxLvTopBar = lv_obj_create(parent);
    lv_obj_set_size(gfxLvTopBar, lv_pct(100), 25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this
    lv_obj_center(gfxLvTopBar);
    lv_obj_set_scrollbar_mode(gfxLvTopBar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(gfxLvTopBar, 1, 0);
    lv_obj_set_flex_flow(gfxLvTopBar, LV_FLEX_FLOW_ROW);

    gfxLvTopText = lv_label_create(gfxLvTopBar);
    gfxLvVersionText = lv_label_create(gfxLvTopBar);
    //lv_obj_set_width(gfxLvTopText, lv_pct(100));
    lv_label_set_text(gfxLvTopText, "FTMS by lefty01");
    lv_obj_set_flex_grow(gfxLvTopText, 1);

    gfxLvLedSpeed = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedSpeed, 12, 12);
    lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedSpeed);

    gfxLvLedIncline = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedIncline, 12, 12);
    lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedIncline);

    gfxLvLedBT = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedBT, 12, 12);
    lv_led_set_color(gfxLvLedBT, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_led_off(gfxLvLedBT);

  } else {
    lv_obj_set_parent(gfxLvTopBar, parent);
  }
}

static lv_obj_t* createScreenBoot()
{
  lv_obj_t* screenBoot = lv_obj_create(NULL);

  lv_obj_set_style_pad_all(screenBoot, 1, 0);
  lv_obj_set_layout(screenBoot, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screenBoot, LV_FLEX_FLOW_COLUMN);

  gfxLvBootTopBarPlaceholder = lv_obj_create(screenBoot);
  lv_obj_set_size(gfxLvBootTopBarPlaceholder, lv_pct(100),
                  25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this in main
  lv_obj_center(gfxLvBootTopBarPlaceholder);
  lv_obj_set_style_pad_all(gfxLvBootTopBarPlaceholder, 1, 0);
  lv_obj_set_scrollbar_mode(gfxLvBootTopBarPlaceholder, LV_SCROLLBAR_MODE_OFF);

  gfxLvLogTextArea = lv_textarea_create(screenBoot);
  lv_obj_add_state(gfxLvLogTextArea, LV_STATE_FOCUSED); // show "cursor"
  lv_obj_set_size(gfxLvLogTextArea, lv_pct(100), lv_pct(100));
  lv_obj_center(gfxLvLogTextArea);
  lv_obj_set_flex_grow(gfxLvLogTextArea, 1);

  // TODO Could be good to have some progressbar also.
  return screenBoot;
}

static meter_t setupMeter(lv_obj_t* parent, uint32_t minVal, uint32_t maxVal, uint32_t minBlue, uint32_t maxRed)
{
  meter_t m = {0};
  uint32_t meterSize = (screenWidth / 2.4); //480 -> 200,  320 -> 133  // TODO something more clever?
  int32_t meterPosX = -15;
  int32_t meterPosY = (screenHeight > 135) ? 18 : -5;

  m.minVal = minVal;
  m.maxVal = maxVal;

  /* SCALE */
  m.scale = lv_scale_create(parent);
  lv_obj_set_size(m.scale, lv_pct(100), lv_pct(100));
  lv_scale_set_mode(m.scale, LV_SCALE_MODE_ROUND_INNER);
  lv_obj_set_size(m.scale, meterSize, meterSize);
  lv_obj_align(m.scale, LV_ALIGN_OUT_TOP_LEFT, meterPosX, meterPosY);

  lv_scale_set_range(m.scale, minVal, maxVal);
  lv_scale_set_angle_range(m.scale, 180);
  lv_scale_set_rotation(m.scale, 180);

  /* TICKS */
  lv_scale_set_total_tick_count(m.scale, (maxVal - minVal) * 2);
  lv_scale_set_major_tick_every(m.scale, 4);

  lv_obj_set_style_length(m.scale, 10, LV_PART_MAIN);
  lv_obj_set_style_length(m.scale, 15, LV_PART_ITEMS);
  lv_obj_set_style_line_width(m.scale, 2, LV_PART_MAIN);
  lv_obj_set_style_line_color(m.scale, lv_palette_main(LV_PALETTE_GREY), LV_PART_MAIN);

  /* BLUE ARC */
  m.arc_blue = lv_arc_create(parent);
  lv_obj_remove_style(m.arc_blue, NULL, LV_PART_KNOB);
  lv_arc_set_bg_angles(m.arc_blue, 180, 360);
  lv_arc_set_angles(m.arc_blue, lv_map(minBlue, minVal, maxVal, 180, 360), lv_map(minVal, minVal, maxVal, 180, 360));
  lv_obj_set_style_arc_width(m.arc_blue, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(m.arc_blue, lv_palette_main(LV_PALETTE_BLUE), LV_PART_INDICATOR);
  lv_obj_center(m.arc_blue);

  /* RED ARC */
  m.arc_red = lv_arc_create(parent);
  lv_obj_remove_style(m.arc_red, NULL, LV_PART_KNOB);
  lv_arc_set_bg_angles(m.arc_red, 180, 360);
  lv_arc_set_angles(m.arc_red, lv_map(maxRed, minVal, maxVal, 180, 360), lv_map(maxVal, minVal, maxVal, 180, 360));
  lv_obj_set_style_arc_width(m.arc_red, 6, LV_PART_INDICATOR);
  lv_obj_set_style_arc_color(m.arc_red, lv_palette_main(LV_PALETTE_RED), LV_PART_INDICATOR);
  lv_obj_center(m.arc_red);

  /* NEEDLE */
  m.needle = lv_line_create(parent);

  static lv_point_precise_t pts[] = {{0, 0}, {0, -50}};
  lv_line_set_points(m.needle, pts, 2);

  lv_obj_set_style_line_width(m.needle, 4, 0);
  lv_obj_set_style_line_color(m.needle, lv_palette_main(LV_PALETTE_GREY), 0);
  lv_obj_center(m.needle);

  return m;
}

#ifdef HAS_TOUCH_DISPLAY

static void gfxLvSwitchSpeedEventhandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      //On
      speedInclineMode &= ~SPEED;
      logText("speedInclineMode &= ~SPEED\n");
    } else {
      //Off
      speedInclineMode |= SPEED;
      logText("speedInclineMode |= SPEED\n");
    }
    lvgl_gfxUpdateHeader();
  }
}

static void gfxLvSwitchInclineEventhandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t* obj = lv_event_get_target(e);
  if (code == LV_EVENT_VALUE_CHANGED) {
    if (lv_obj_has_state(obj, LV_STATE_CHECKED)) {
      //On
      speedInclineMode &= ~INCLINE;
      logText("speedInclineMode &= ~INCLINE\n");
    } else {
      //Off
      speedInclineMode |= INCLINE;
      logText("speedInclineMode |= INCLINE\n");
    }
    lvgl_gfxUpdateHeader();
  }
}

static void gfxLvSpeedUpEventHandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    logText("gfxLvSpeedUpEventHandler: LV_EVENT_CLICKED\n");
    speedUp();
  }
}

static void gfxLvSpeedDownEventHandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    logText("gfxLvSpeedDownEventHandler: LV_EVENT_CLICKED\n");
    speedDown();
  }
}

static void gfxLvInclineUpEventHandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    logText("gfxLvInclineUpEventHandler: LV_EVENT_CLICKED\n");
    inclineUp();
  }
}

static void gfxLvInclineDownEventHandler(lv_event_t* e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if (code == LV_EVENT_CLICKED) {
    logText("gfxLvInclineDownEventHandler: LV_EVENT_CLICKED\n");
    inclineDown();
  }
}

#endif

static lv_obj_t* createScreenMain()
{
  lv_obj_t* screenMain = lv_obj_create(NULL);
  lv_obj_set_style_pad_column(screenMain, 0, 0);
  lv_obj_set_style_pad_row(screenMain, 0, 0);
  lv_obj_set_layout(screenMain, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screenMain, LV_FLEX_FLOW_COLUMN);

  gfxLvMainTopBarPlaceholder = lv_obj_create(screenMain);
  lv_obj_set_size(gfxLvMainTopBarPlaceholder, lv_pct(100),
                  25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this in main
  lv_obj_center(gfxLvMainTopBarPlaceholder);
  lv_obj_set_style_pad_all(gfxLvMainTopBarPlaceholder, 0, 0);
  lv_obj_set_scrollbar_mode(gfxLvMainTopBarPlaceholder, LV_SCROLLBAR_MODE_OFF);

  lv_obj_t* obj;
  lv_obj_t* label;
  lv_obj_t* objRow;
  objRow = lv_obj_create(screenMain);
  lv_obj_set_size(objRow, lv_pct(100), lv_pct(40));
  lv_obj_set_flex_grow(objRow, 1);
  lv_obj_set_layout(objRow, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(objRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(objRow, 0, 0);

  // -------------------
  /*Cell to 0;0*/
  obj = lv_obj_create(objRow);
  lv_obj_set_size(obj, lv_pct(50), lv_pct(100));
  lv_obj_set_flex_grow(obj, 1);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
  speedMeter = setupMeter(obj, configTreadmill.min_speed, configTreadmill.max_speed, 6, 16);

#ifdef HAS_TOUCH_DISPLAY
  gfxLvSwitchSpeedControlMode = lv_switch_create(obj);
  lv_obj_set_size(gfxLvSwitchSpeedControlMode, 40, 30);
  lv_obj_add_event_cb(gfxLvSwitchSpeedControlMode, gfxLvSwitchSpeedEventhandler, LV_EVENT_ALL, NULL);
#endif

  lvLabelSpeed = lv_label_create(obj);
#ifdef HAS_TOUCH_DISPLAY
  lv_obj_align_to(lvLabelSpeed, gfxLvSwitchSpeedControlMode, LV_ALIGN_TOP_RIGHT, 30, 0);
#endif
  lv_label_set_text(lvLabelSpeed, "Speed: --.-- km/h");

#ifdef HAS_TOUCH_DISPLAY
  lv_obj_t* buttonObj = lv_obj_create(obj);
  lv_obj_align(buttonObj, LV_ALIGN_TOP_RIGHT, 0, 40);
  lv_obj_set_size(buttonObj, 50, 100);
  lv_obj_set_layout(buttonObj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(buttonObj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(buttonObj, 0, 0);

  lv_obj_t* btn1;
  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvSpeedUpEventHandler, LV_EVENT_ALL, NULL);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "+");
  lv_obj_center(label);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvSpeedDownEventHandler, LV_EVENT_ALL, NULL);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "-");
  lv_obj_center(label);
#endif

  // -------------------
  /* Cell to 0;1 */
  obj = lv_obj_create(objRow);
  lv_obj_set_size(obj, lv_pct(50), lv_pct(100));
  lv_obj_set_flex_grow(obj, 1);
  lv_obj_set_style_pad_all(obj, 0, 0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);
  inclineMeter = setupMeter(obj, configTreadmill.min_incline, configTreadmill.max_incline, 2, 7);

#ifdef HAS_TOUCH_DISPLAY
  gfxLvSwitchInclineControlMode = lv_switch_create(obj);
  lv_obj_set_size(gfxLvSwitchInclineControlMode, 40, 30);
  lv_obj_add_event_cb(gfxLvSwitchInclineControlMode, gfxLvSwitchInclineEventhandler, LV_EVENT_ALL, NULL);
#endif

  lvLabelIncline = lv_label_create(obj);
#ifdef HAS_TOUCH_DISPLAY
  lv_obj_align_to(lvLabelIncline, gfxLvSwitchInclineControlMode, LV_ALIGN_TOP_RIGHT, 30, 0);
#endif
  lv_label_set_text(lvLabelIncline, "Incline: --.- %");

#ifdef HAS_TOUCH_DISPLAY
  buttonObj = lv_obj_create(obj);
  lv_obj_align(buttonObj, LV_ALIGN_TOP_RIGHT, 0, 40);
  lv_obj_set_size(buttonObj, 50, 100);
  lv_obj_set_layout(buttonObj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(buttonObj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(buttonObj, 0, 0);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvInclineUpEventHandler, LV_EVENT_ALL, NULL);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "+");
  lv_obj_center(label);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvInclineDownEventHandler, LV_EVENT_ALL, NULL);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "-");
  lv_obj_center(label);
#endif

  // -------------------
  /*Create a chart*/
  lvGraph = lv_chart_create(screenMain);
  lv_obj_set_width(lvGraph, lv_pct(100));
  lv_obj_set_flex_grow(lvGraph, 1);
  lv_chart_set_type(lvGraph, LV_CHART_TYPE_LINE); /*Show lines and points too*/
  lv_obj_set_style_pad_all(lvGraph, 0, 0);
  lv_chart_set_update_mode(lvGraph, graphCircular ? LV_CHART_UPDATE_MODE_CIRCULAR : LV_CHART_UPDATE_MODE_SHIFT);

  /*Add two data series*/
  lv_chart_set_point_count(lvGraph, graphDataPoints);
  lvGraphSpeedSerie = lv_chart_add_series(lvGraph, lv_palette_main(LV_PALETTE_RED), LV_CHART_AXIS_PRIMARY_Y);
  lvGraphInclineSerie = lv_chart_add_series(lvGraph, lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_SECONDARY_Y);

  lv_chart_set_range(lvGraph, LV_CHART_AXIS_PRIMARY_Y, configTreadmill.min_speed, configTreadmill.max_speed);
  lv_chart_set_range(lvGraph, LV_CHART_AXIS_SECONDARY_Y, configTreadmill.min_incline, configTreadmill.max_incline);

  /* Clear data in graphs*/
  lv_chart_set_all_value(lvGraph, lvGraphSpeedSerie, 0);
  lv_chart_set_all_value(lvGraph, lvGraphInclineSerie, 0);
  //lv_chart_refresh(lvGraph); /*Required after direct set*/

  lvLabelDistance = lv_label_create(lvGraph);
  lv_obj_align(lvLabelDistance, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_label_set_text(lvLabelDistance, "Dist: --.- km");

  lvLabelElevation = lv_label_create(lvGraph);
  lv_obj_align(lvLabelElevation, LV_ALIGN_TOP_RIGHT, 0, 0);
  lv_label_set_text(lvLabelElevation, "Elevation gain: ---- m");

  //lv_obj_invalidate(...); // force flush callback
  return screenMain;
}

void lvgl_gfxShowScreenBoot()
{
  if (gfxLvScreenBoot == nullptr) {
    gfxLvScreenBoot = createScreenBoot();
  }
  if ((gfxLvScreenBoot != nullptr) && (gfxLvScreenCurrent != gfxLvScreenBoot)) {
    lv_screen_load(gfxLvScreenBoot);
    showGfxTopBar(gfxLvBootTopBarPlaceholder);

    gfxLvScreenCurrent = gfxLvScreenBoot;
  }
}

void lvgl_gfxShowScreenMain()
{
  if (gfxLvScreenMain == nullptr) {
    gfxLvScreenMain = createScreenMain();
  }
  if ((gfxLvScreenMain != nullptr) && (gfxLvScreenCurrent != gfxLvScreenMain)) {
    lv_screen_load(gfxLvScreenMain);
    showGfxTopBar(gfxLvMainTopBarPlaceholder);

    lv_obj_set_size(gfxLvMainTopBarPlaceholder, lv_pct(100), 25 /*LV_SIZE_CONTENT*/);
    gfxLvScreenCurrent = gfxLvScreenMain;
    //lv_obj_invalidate(lv_screen_active());
  }
}

/*** Display callback to flush the buffer to screen ***/
void lvgl_displayFlushCallBack(lv_display_t* disp, const lv_area_t* area, uint8_t* px_map)
{
  //  uint32_t w = (area->x2 - area->x1 + 1);
  //  uint32_t h = (area->y2 - area->y1 + 1);

  uint32_t w = lv_area_get_width(area);
  uint32_t h = lv_area_get_height(area);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushPixels((uint16_t*)px_map, w * h, true);
  tft.endWrite();

  lv_display_flush_ready(disp);
}

void meter_set_value(meter_t* m, uint32_t value)
{
  int angle = lv_map(value, m->minVal, m->maxVal, 180, 360);
  lv_obj_set_style_transform_angle(m->needle, angle * 10, 0);
}

#ifdef HAS_TOUCH_DISPLAY
/*** Touchpad callback to read the touchpad ***/
void lvgl_touchPadReadCallback(lv_indev_t* indev, lv_indev_data_t* data)
{
  uint16_t touchX = 0;
  uint16_t touchY = 0;
  bool touched = tft.getTouch(&touchX, &touchY);

  if (!touched) {
    data->state = LV_INDEV_STATE_RELEASED;
  } else {
    data->state = LV_INDEV_STATE_PRESSED;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;
    // fixme: in case rotated? data->point.x = screenWidth - touchX; data->point.y = screenHeight - touchY;

    // Serial.printf("Touch (x,y): (%03d,%03d)\n",touchX,touchY );
  }
}
#endif

void lvgl_initDisplay()
{
  // TFT init has already been done
  lv_init();
  lv_display_t* disp = lv_display_create(screenWidth, screenHeight);
  lv_display_set_default(disp); // force active display

  uint32_t stride = lv_draw_buf_width_to_stride(screenWidth, LV_COLOR_FORMAT_RGB565);

  // init draw buffer
  lv_draw_buf_init(&draw_buf, screenWidth, BUFFER_LINES, LV_COLOR_FORMAT_RGB565, stride, buf, sizeof(buf));

  lv_display_set_flush_cb(disp, lvgl_displayFlushCallBack);
  lv_display_set_draw_buffers(disp, &draw_buf, NULL);

#ifdef HAS_TOUCH_DISPLAY
  lv_indev_t* indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, lvgl_touchPadReadCallback);
#endif
  lvgl_gfxShowScreenBoot();
}

void lvgl_loopHandleGfx()
{
  static uint32_t last_ms = 0;
  uint32_t now = millis();
  uint32_t diff = now - last_ms;
  last_ms = now;
  lv_tick_inc(diff);

  lv_timer_handler();
}

void lvgl_gfxLogText(const char* text)
{
  if (gfxLvLogTextArea) {
    // On screen console
    lv_textarea_add_text(gfxLvLogTextArea, text);

    // Trigger a GFX update if visible
    if (!setupDone && gfxLvScreenCurrent == gfxLvScreenBoot) {
      // gfxLvScreenBoot is shown before the main loop and gfx is not updated periodically
      lvgl_loopHandleGfx();
    }
  }
}

// Called each second
void lvgl_gfxUpdateDisplay(bool clean)
{
  if (clean) {
    lvgl_gfxShowScreenMain();
    lvgl_gfxUpdateHeader();
  }

  static int count = 0;
  count++;

  lv_label_set_text_fmt(lvLabelSpeed, "Speed: %2.2f km/h", kmph);
  meter_set_value(&speedMeter, kmph);

  lv_label_set_text_fmt(lvLabelIncline, "Incline:  %2.2f %%", incline);
  lv_label_set_text_fmt(lvLabelDistance, "Dist %2.2f km", totalDistance / 1000);
  lv_label_set_text_fmt(lvLabelElevation, "Elevation gain %d m", (uint32_t)elevationGain);
  meter_set_value(&inclineMeter, incline);

  // Update graph
  if (count >= graphDataPointAdvanceTime) {
    if (kmph > 0) { // scroll right
      lv_chart_set_next_value(lvGraph, lvGraphSpeedSerie, kmph);
      lv_chart_set_next_value(lvGraph, lvGraphInclineSerie, incline);

      uint16_t nextPointID = lv_chart_get_x_start_point(lvGraph, lvGraphSpeedSerie); //should be the same for both
      // clear "free-up" point
      lv_chart_set_value_by_id(lvGraph, lvGraphSpeedSerie, nextPointID, LV_CHART_POINT_NONE);
      lv_chart_set_value_by_id(lvGraph, lvGraphInclineSerie, nextPointID, LV_CHART_POINT_NONE);
    }
    count = 0;
  }
  // set current point directly
  uint16_t updatePointID = lv_chart_get_x_start_point(lvGraph, lvGraphSpeedSerie); //should be the same for both
  updatePointID = (updatePointID - 1 + graphDataPoints) % graphDataPoints;

  lv_chart_set_value_by_id(lvGraph, lvGraphSpeedSerie, updatePointID, kmph);
  lv_chart_set_value_by_id(lvGraph, lvGraphInclineSerie, updatePointID, incline);

  lv_chart_refresh(lvGraph); /*Required after direct set*/
}

void lvgl_gfxUpdateBTConnectionStatus(bool connected)
{
  if (connected) {
    lv_led_on(gfxLvLedBT);
  } else {
    lv_led_off(gfxLvLedBT);
  }
}

void lvgl_gfxUpdateVersion(const char* version)
{}

static void lvgl_gfxUpdateSpeedInclineMode(uint8_t mode)
{
  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED) {
    lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedSpeed);
#ifdef HAS_TOUCH_DISPLAY
    lv_obj_clear_state(gfxLvSwitchSpeedControlMode, LV_STATE_CHECKED);
#endif
  } else if ((mode & SPEED) == 0) {
    lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_RED));
    lv_led_on(gfxLvLedSpeed);
#ifdef HAS_TOUCH_DISPLAY
    lv_obj_add_state(gfxLvSwitchSpeedControlMode, LV_STATE_CHECKED);
#endif
  }

  if (mode & INCLINE) {
    lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedIncline);
#ifdef HAS_TOUCH_DISPLAY
    lv_obj_clear_state(gfxLvSwitchInclineControlMode, LV_STATE_CHECKED);
#endif
  } else if ((mode & INCLINE) == 0) {
    lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_RED));
    lv_led_on(gfxLvLedIncline);
#ifdef HAS_TOUCH_DISPLAY
    lv_obj_add_state(gfxLvSwitchInclineControlMode, LV_STATE_CHECKED);
#endif
  }
}

void lvgl_gfxUpdateWIFI(const unsigned long reconnect_counter, const String& ip)
{
  // show reconnect counter in tft
  // if (wifi_reconnect_counter > wifi_reconnect_counter_prev) ... only update on change
  std::ostringstream oss;
  oss << "Wifi[" << reconnect_counter << "]: " << ip.c_str();
  oss << " v" << VERSION;
  std::string str = oss.str();
  lv_label_set_text(gfxLvTopText, str.c_str());
}

void lvgl_gfxUpdateHeader()
{
  // indicate manual/auto mode (green=auto/sensor, red=manual)
  lvgl_gfxUpdateSpeedInclineMode(speedInclineMode);

  // indicate bt connection status ... offline
  lvgl_gfxUpdateBTConnectionStatus(bleClientConnected);

  lvgl_gfxUpdateWIFI(wifiReconnectCounter, getWifiIpAddr());

  lvgl_gfxUpdateVersion(VERSION);
}

#endif //#if !defined(NO_DISPLAY) && defined(GUI_LVGL)
