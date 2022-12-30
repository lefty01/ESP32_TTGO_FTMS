#include "common.h"
#include "display.h"
#include "config.h"
#include "net-control.h"
#include <string>
#include <sstream>

#define USE_LV_GAUGE
#include <lvgl.h>
#include "lv_conf.h"

// Setup screen resolution for LVGL
static const uint16_t screenWidth = TFT_HEIGHT;
static const uint16_t screenHeight = TFT_WIDTH;
static lv_disp_draw_buf_t draw_buf;
static lv_color_t buf[screenWidth * 10];


// Screens
static lv_obj_t * gfxLvScreenBoot = nullptr;
static lv_obj_t * gfxLvScreenMain = nullptr;
static lv_obj_t * gfxLvScreenCurrent = nullptr;

// Top Bar
static lv_obj_t * gfxLvTopText = nullptr;
static lv_obj_t * gfxLvLedSpeed = nullptr;
static lv_obj_t * gfxLvLedIncline = nullptr;
static lv_obj_t * gfxLvLedBT = nullptr;

// Boot screen stuff
static lv_obj_t * gfxLvBootTopBarPlaceholder = nullptr;
static lv_obj_t * gfxLvLogTextArea = nullptr;  // Show serial/debug/log output

// Main screen stuff
static lv_obj_t * gfxLvMainTopBarPlaceholder = nullptr;
static lv_obj_t * lvLabelSpeed = nullptr;
static lv_obj_t * lvLabelIncline = nullptr;
static lv_obj_t * lvLabelDistance = nullptr;
static lv_obj_t * lvLabelElevation = nullptr;
static lv_obj_t * gfxLvSpeedMeter = nullptr;  // Show speed
static lv_obj_t * gfxLvInclineMeter = nullptr;  // Show Incline
static lv_obj_t * gfxLvSwitchSpeedControlMode = nullptr;
static lv_obj_t * gfxLvSwitchInclineControlMode = nullptr;
static lv_meter_indicator_t *gfxLvSpeedMeterIndicator = nullptr;
static lv_meter_indicator_t *gfxLvInclineMeterIndicator = nullptr;

static lv_obj_t * lvGraph = nullptr;
static lv_chart_series_t * lvGraphSpeedSerie = nullptr;
static lv_chart_series_t * lvGraphInclineSerie = nullptr;
static const uint16_t graphDataPoints = 60;
static const uint16_t graphDataPointAdvanceTime = 60; //How many seconds until "scrolling" one step
static const bool graphCircular = false;


void lvgl_gfxUpdateHeader();

static void showGfxTopBar(lv_obj_t *parent)
{
  static lv_obj_t * gfxLvTopBar = nullptr;
  if (!gfxLvTopBar)
  {

    gfxLvTopBar = lv_obj_create(parent);
    lv_obj_set_size(gfxLvTopBar, lv_pct(100), 25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this
    lv_obj_center(gfxLvTopBar);
    lv_obj_set_scrollbar_mode(gfxLvTopBar, LV_SCROLLBAR_MODE_OFF);
    lv_obj_set_style_pad_all(gfxLvTopBar,1,0);
    lv_obj_set_flex_flow(gfxLvTopBar, LV_FLEX_FLOW_ROW);

    gfxLvTopText = lv_label_create(gfxLvTopBar);
    //lv_obj_set_width(gfxLvTopText, lv_pct(100));
    lv_label_set_text(gfxLvTopText, "FTMS");
    lv_obj_set_flex_grow(gfxLvTopText, 1); 

    gfxLvLedSpeed  = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedSpeed, 12, 12);
    lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedSpeed);

    gfxLvLedIncline  = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedIncline, 12, 12);
    lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_GREEN));
    lv_led_on(gfxLvLedIncline);

    gfxLvLedBT  = lv_led_create(gfxLvTopBar);
    lv_obj_set_size(gfxLvLedBT, 12, 12);
    lv_led_set_color(gfxLvLedBT, lv_palette_main(LV_PALETTE_LIGHT_BLUE));
    lv_led_off(gfxLvLedBT);

  }
  else
  {
    lv_obj_set_parent(gfxLvTopBar, parent);
  }
}

static lv_obj_t * createScreenBoot() {
  lv_obj_t * screenBoot = lv_obj_create(NULL);

  lv_obj_set_style_pad_all(screenBoot,1,0);
  lv_obj_set_layout(screenBoot, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screenBoot, LV_FLEX_FLOW_COLUMN);

  gfxLvBootTopBarPlaceholder = lv_obj_create(screenBoot);
  lv_obj_set_size(gfxLvBootTopBarPlaceholder, lv_pct(100), 25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this in main
  lv_obj_center(gfxLvBootTopBarPlaceholder);
  lv_obj_set_style_pad_all(gfxLvBootTopBarPlaceholder,1,0);
  lv_obj_set_scrollbar_mode(gfxLvBootTopBarPlaceholder, LV_SCROLLBAR_MODE_OFF);


  gfxLvLogTextArea = lv_textarea_create(screenBoot);
  lv_obj_add_state(gfxLvLogTextArea, LV_STATE_FOCUSED);  // show "cursor"
  lv_obj_set_size(gfxLvLogTextArea, lv_pct(100), lv_pct(100));
  lv_obj_center(gfxLvLogTextArea);
  lv_obj_set_flex_grow(gfxLvLogTextArea, 1); 

  // TODO Could be good to have some progressbar also.
  return screenBoot;
}

static lv_meter_indicator_t * setupMeter(lv_obj_t * meter, uint32_t minVal, uint32_t maxVal, uint32_t minBlue, uint32_t maxRed)
{
  lv_obj_set_scrollbar_mode(meter, LV_SCROLLBAR_MODE_OFF);

  /*Add a scale first*/
  lv_meter_scale_t * scale = lv_meter_add_scale(meter);
  lv_meter_set_scale_ticks(meter, scale, (maxVal-minVal)*2, 2, 10, lv_palette_main(LV_PALETTE_GREY));
  lv_meter_set_scale_major_ticks(meter, scale, 4, 4, 15, lv_palette_main(LV_PALETTE_GREY), 10);
  lv_meter_set_scale_range(meter, scale, minVal, maxVal, 180, 180);

  lv_meter_indicator_t * indic;

  /*Add a blue arc to the start*/
  indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_BLUE), 0);
  lv_meter_set_indicator_start_value(meter, indic, minVal);
  lv_meter_set_indicator_end_value(meter, indic, minBlue);

  /*Make the tick lines blue at the start of the scale*/
  indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_BLUE), lv_palette_main(LV_PALETTE_BLUE), false, 0);
  lv_meter_set_indicator_start_value(meter, indic, minVal);
  lv_meter_set_indicator_end_value(meter, indic, minBlue);

  /*Add a red arc to the end*/
  indic = lv_meter_add_arc(meter, scale, 3, lv_palette_main(LV_PALETTE_RED), 0);
  lv_meter_set_indicator_start_value(meter, indic, maxRed);
  lv_meter_set_indicator_end_value(meter, indic, maxVal);

  /*Make the tick lines red at the end of the scale*/
  indic = lv_meter_add_scale_lines(meter, scale, lv_palette_main(LV_PALETTE_RED), lv_palette_main(LV_PALETTE_RED), false, 0);
  lv_meter_set_indicator_start_value(meter, indic, maxRed);
  lv_meter_set_indicator_end_value(meter, indic, maxVal);

  /*Add a needle line indicator*/
  indic = lv_meter_add_needle_line(meter, scale, 4, lv_palette_main(LV_PALETTE_GREY), -10);

  return indic;
}

static void gfxLvSwitchSpeedEventhandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    if(lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
      //On
      speedInclineMode &= ~SPEED;
      logText("speedInclineMode &= ~SPEED\n");
    }
    else
    {
      //Off
      speedInclineMode |= SPEED;
      logText("speedInclineMode |= SPEED\n");
    }
    lvgl_gfxUpdateHeader();
  }
}

static void gfxLvSwitchInclineEventhandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);
  lv_obj_t * obj = lv_event_get_target(e);
  if(code == LV_EVENT_VALUE_CHANGED) {
    if(lv_obj_has_state(obj, LV_STATE_CHECKED))
    {
      //On
      speedInclineMode &= ~INCLINE;
      logText("speedInclineMode &= ~INCLINE\n");
    }
    else
    {
      //Off
      speedInclineMode |= INCLINE;
      logText("speedInclineMode |= INCLINE\n");
    }
    lvgl_gfxUpdateHeader();
  }
}

static void gfxLvSpeedUpEventHandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if(code == LV_EVENT_CLICKED)
  {
    logText("gfxLvSpeedUpEventHandler: LV_EVENT_CLICKED\n");
    speedUp();
  }
}

static void gfxLvSpeedDownEventHandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if(code == LV_EVENT_CLICKED)
  {
    logText("gfxLvSpeedDownEventHandler: LV_EVENT_CLICKED\n");
    speedDown();
  }
}

static void gfxLvInclineUpEventHandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if(code == LV_EVENT_CLICKED)
  {
    logText("gfxLvInclineUpEventHandler: LV_EVENT_CLICKED\n");
    inclineUp();
  }
}

static void gfxLvInclineDownEventHandler(lv_event_t * e)
{
  lv_event_code_t code = lv_event_get_code(e);

  if(code == LV_EVENT_CLICKED)
  {
    logText("gfxLvInclineDownEventHandler: LV_EVENT_CLICKED\n");
    inclineDown();
  }
}

static lv_obj_t * createScreenMain()
{
  lv_obj_t * screenMain = lv_obj_create(NULL);
  lv_obj_set_style_pad_all(screenMain,0,0);
  lv_obj_set_layout(screenMain, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(screenMain, LV_FLEX_FLOW_COLUMN);

  gfxLvMainTopBarPlaceholder = lv_obj_create(screenMain);
  lv_obj_set_size(gfxLvMainTopBarPlaceholder, lv_pct(100), 25); //TODO 25 should be LV_SIZE_CONTENT but got problem when reusing this in main
  lv_obj_center(gfxLvMainTopBarPlaceholder);
  lv_obj_set_style_pad_all(gfxLvMainTopBarPlaceholder,0,0);
  lv_obj_set_scrollbar_mode(gfxLvMainTopBarPlaceholder, LV_SCROLLBAR_MODE_OFF);


  lv_obj_t * obj;
  lv_obj_t * label;
  lv_obj_t * objRow;
  objRow = lv_obj_create(screenMain);
  lv_obj_set_size(objRow, lv_pct(100), lv_pct(40));
  lv_obj_set_flex_grow(objRow, 1);
  lv_obj_set_layout(objRow, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(objRow, LV_FLEX_FLOW_ROW);
  lv_obj_set_style_pad_all(objRow,0,0);

  // -------------------
  /*Cell to 0;0*/
  obj = lv_obj_create(objRow);
  lv_obj_set_size(obj, lv_pct(50), lv_pct(100));
  lv_obj_set_flex_grow(obj, 1); 
  lv_obj_set_style_pad_all(obj,0,0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

  gfxLvSpeedMeter = lv_meter_create(obj);
  lv_obj_set_size(gfxLvSpeedMeter, 200, 200); //TODO would be good if this was not hardcoded, BUT make sure its square if not, rendering gets messed up if not (at least in lvgl 8.3)
  // TODO ?? make it square by first let i span 100% then use the smalest number
  //uint32_t size = std::min(lv_obj_get_width(gfxLvSpeedMeter),lv_obj_get_height(gfxLvSpeedMeter));
  //lv_obj_set_size(gfxLvSpeedMeter, size, size);
  lv_obj_align(gfxLvSpeedMeter, LV_ALIGN_OUT_TOP_LEFT, -15, 18);
  gfxLvSpeedMeterIndicator = setupMeter(gfxLvSpeedMeter, configTreadmill.min_speed, configTreadmill.max_speed, 6, 16);

  gfxLvSwitchSpeedControlMode = lv_switch_create(obj);
  //lv_obj_align_to(gfxLvSwitchSpeedControlMode, obj, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(gfxLvSwitchSpeedControlMode, 40, 30);
  lv_obj_add_event_cb(gfxLvSwitchSpeedControlMode, gfxLvSwitchSpeedEventhandler, LV_EVENT_ALL, NULL);

  lvLabelSpeed = lv_label_create(obj);
  lv_obj_align_to(lvLabelSpeed, gfxLvSwitchSpeedControlMode, LV_ALIGN_TOP_RIGHT, 30, 0);
  lv_label_set_text(lvLabelSpeed, "Speed: --.-- km/h");

  
  lv_obj_t * buttonObj = lv_obj_create(obj);
  //lv_obj_align_to(buttonObj, obj, LV_ALIGN_TOP_RIGHT, -50, 40);
  lv_obj_align(buttonObj, LV_ALIGN_TOP_RIGHT, 0, 40);
  lv_obj_set_size(buttonObj, 50, 100);
  lv_obj_set_layout(buttonObj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(buttonObj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(buttonObj,0,0);

  lv_obj_t * btn1;
  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvSpeedUpEventHandler, LV_EVENT_ALL, NULL);
  //lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "+");
  lv_obj_center(label);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvSpeedDownEventHandler, LV_EVENT_ALL, NULL);
  //lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "-");
  lv_obj_center(label);

  // -------------------
  /* Cell to 0;1 */
  obj = lv_obj_create(objRow);
  lv_obj_set_size(obj, lv_pct(50), lv_pct(100));
  lv_obj_set_flex_grow(obj, 1);
  lv_obj_set_style_pad_all(obj,0,0);
  lv_obj_set_scrollbar_mode(obj, LV_SCROLLBAR_MODE_OFF);

  gfxLvInclineMeter = lv_meter_create(obj);
  lv_obj_set_size(gfxLvInclineMeter, 200, 200); //TODO would be good if this was not hardcoded, BUT make sure its square if not, rendering gets messed up if not (at least in lvgl 8.3)
  // TODO ?? make it square by first let i span 100% then use the smalest number
  //uint32_t size = std::min(lv_obj_get_width(gfxLvInclineMeter),lv_obj_get_height(gfxLvInclineMeter));
  //lv_obj_set_size(gfxLvInclineMeter, size, size);
  lv_obj_align(gfxLvInclineMeter, LV_ALIGN_OUT_TOP_LEFT, -15, 18);
  gfxLvInclineMeterIndicator = setupMeter(gfxLvInclineMeter, configTreadmill.min_incline, configTreadmill.max_incline, 2, 7);

  gfxLvSwitchInclineControlMode = lv_switch_create(obj);
//  lv_obj_align_to(gfxLvSwitchInclineControlMode, obj, LV_ALIGN_TOP_LEFT, 0, 0);
  lv_obj_set_size(gfxLvSwitchInclineControlMode, 40, 30);
  lv_obj_add_event_cb(gfxLvSwitchInclineControlMode, gfxLvSwitchInclineEventhandler, LV_EVENT_ALL, NULL);

  lvLabelIncline = lv_label_create(obj);
  lv_obj_align_to(lvLabelIncline, gfxLvSwitchInclineControlMode, LV_ALIGN_TOP_RIGHT, 30, 0);
  lv_label_set_text(lvLabelIncline, "Incline: --.- %");

  buttonObj = lv_obj_create(obj);
  lv_obj_align(buttonObj, LV_ALIGN_TOP_RIGHT, 0, 40);
  lv_obj_set_size(buttonObj, 50, 100);
  lv_obj_set_layout(buttonObj, LV_LAYOUT_FLEX);
  lv_obj_set_flex_flow(buttonObj, LV_FLEX_FLOW_COLUMN);
  lv_obj_set_style_pad_all(buttonObj,0,0);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvInclineUpEventHandler, LV_EVENT_ALL, NULL);
  //lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "+");
  lv_obj_center(label);

  btn1 = lv_btn_create(buttonObj);
  lv_obj_add_event_cb(btn1, gfxLvInclineDownEventHandler, LV_EVENT_ALL, NULL);
  //lv_obj_align(btn1, LV_ALIGN_CENTER, 0, -40);
  label = lv_label_create(btn1);
  lv_label_set_text(label, "-");
  lv_obj_center(label);


  // -------------------
  /*Create a chart*/
  lvGraph = lv_chart_create(screenMain);
  lv_obj_set_width(lvGraph, lv_pct(100));
  lv_obj_set_flex_grow(lvGraph, 1);
  lv_chart_set_type(lvGraph, LV_CHART_TYPE_LINE);   /*Show lines and points too*/
  lv_obj_set_style_pad_all(lvGraph,1,1);
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

  return screenMain;
}


void lvgl_gfxShowScreenBoot() {
  if (gfxLvScreenCurrent != gfxLvScreenBoot)
  {
    lv_scr_load(gfxLvScreenBoot);
    showGfxTopBar(gfxLvBootTopBarPlaceholder);

    gfxLvScreenCurrent = gfxLvScreenBoot;
  }
}

void lvgl_gfxShowScreenMain() {
  if (gfxLvScreenCurrent != gfxLvScreenMain)
  {
    lv_scr_load(gfxLvScreenMain);
    showGfxTopBar(gfxLvMainTopBarPlaceholder);
    lv_obj_set_size(gfxLvMainTopBarPlaceholder, lv_pct(100), LV_SIZE_CONTENT);
    gfxLvScreenCurrent = gfxLvScreenMain;
  }
}

/*** Display callback to flush the buffer to screen ***/
static void lvgl_displayFlushCallBack(lv_disp_drv_t * disp, const lv_area_t *area, lv_color_t *color_p)
{
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  tft.pushPixels((uint16_t *)&color_p->full, w * h, true);
  tft.endWrite();

  lv_disp_flush_ready(disp);
}

/*** Touchpad callback to read the touchpad ***/
static void lvgl_touchPadReadCallback(lv_indev_drv_t * indev_driver, lv_indev_data_t * data)
{
  uint16_t touchX, touchY;
  bool touched = tft.getTouch(&touchX, &touchY);

  if (!touched)
  {
    data->state = LV_INDEV_STATE_REL;
  }
  else
  {
    data->state = LV_INDEV_STATE_PR;

    /*Set the coordinates*/
    data->point.x = touchX;
    data->point.y = touchY;

    // Serial.printf("Touch (x,y): (%03d,%03d)\n",touchX,touchY );
  }
}

void lvgl_initDisplay()
{
  // TFT init has already been done
  lv_init();

  /* LVGL : Setting up buffer to use for display */
  lv_disp_draw_buf_init(&draw_buf, buf, NULL, screenWidth * 10);

  /*** LVGL : Setup & Initialize the display device driver ***/
  static lv_disp_drv_t disp_drv;
  lv_disp_drv_init(&disp_drv);
  disp_drv.hor_res = screenWidth;
  disp_drv.ver_res = screenHeight;
  disp_drv.flush_cb = lvgl_displayFlushCallBack;
  disp_drv.draw_buf = &draw_buf;
  lv_disp_drv_register(&disp_drv);

  /*** LVGL : Setup & Initialize the input device driver ***/
  static lv_indev_drv_t indev_drv;
  lv_indev_drv_init(&indev_drv);
  indev_drv.type = LV_INDEV_TYPE_POINTER;
  indev_drv.read_cb = lvgl_touchPadReadCallback;
  lv_indev_drv_register(&indev_drv);

  //delayWithDisplayUpdate(3000);

  // Create all global gfxLv objects here
  // Could be moved to first use, and it would be nice but then it could be
  // easy to miss creating some if they are used on different places.
  gfxLvScreenBoot = createScreenBoot();
  gfxLvScreenMain = createScreenMain();
  lvgl_gfxShowScreenBoot();
}

void lvgl_loopHandleGfx()
{
  lv_timer_handler();
}

void lvgl_gfxLogText(const char *text)
{
  if (gfxLvLogTextArea)
  {
    // On screen console
    lv_textarea_add_text(gfxLvLogTextArea, text);

    // Trigger a GFX update if visible
    if (!setupDone && gfxLvScreenCurrent == gfxLvScreenBoot)
    {
      // gfxLvScreenBoot is shown before the main loop and gfx is not updated periodically
      lvgl_loopHandleGfx();
    }
  }
}

// Called each second
void lvgl_gfxUpdateDisplay(bool clean)
{
  if (clean)
  {
    lvgl_gfxShowScreenMain();
    lvgl_gfxUpdateHeader();
  }
  //static float kmphCopy = 0.0;
  static int count = 0;
  count++;

  // Update if changed
  //if (kmphCopy != kmph)
  //{
    lv_label_set_text_fmt(lvLabelSpeed, "Speed: %2.2f km/h",kmph);
    lv_meter_set_indicator_value(gfxLvSpeedMeter, gfxLvSpeedMeterIndicator, kmph);
    //kmphCopy = kmph;
  //}

  lv_label_set_text_fmt(lvLabelIncline, "Incline:  %2.2f %%",incline);
  lv_label_set_text_fmt(lvLabelDistance, "Dist %2.2f km",totalDistance/1000);
  lv_label_set_text_fmt(lvLabelElevation, "Elevation gain %d m",(uint32_t)elevationGain);

  lv_meter_set_indicator_value(gfxLvInclineMeter, gfxLvInclineMeterIndicator, incline);

  // Update graph
  if (count >= graphDataPointAdvanceTime)
  {
    // scroll
    if (kmph > 0)
    {
      lv_chart_set_next_value(lvGraph, lvGraphSpeedSerie, kmph);
      lv_chart_set_next_value(lvGraph, lvGraphInclineSerie, incline);
        uint16_t nextPointID = lv_chart_get_x_start_point(lvGraph, lvGraphSpeedSerie); //should be the same for both
        lvGraphSpeedSerie->y_points[nextPointID] = LV_CHART_POINT_NONE;
        lvGraphInclineSerie->y_points[nextPointID] = LV_CHART_POINT_NONE;
      //}
    }
    count = 0;
  }
  uint16_t updatePointID = lv_chart_get_x_start_point(lvGraph, lvGraphSpeedSerie); //should be the same for both
  updatePointID = (updatePointID -1) % graphDataPoints;
  lvGraphSpeedSerie->y_points[updatePointID] = kmph;
  lvGraphInclineSerie->y_points[updatePointID] = incline;
  lv_chart_refresh(lvGraph); /*Required after direct set*/
}


void lvgl_gfxUpdateBTConnectionStatus(bool connected)
{
  if (connected)
  {
    lv_led_on(gfxLvLedBT);
  }
  else
  {
    lv_led_off(gfxLvLedBT);
  }
}


static void lvgl_gfxUpdateSpeedInclineMode(uint8_t mode)
{
  // two circles indicate weather speed and/or incline is measured
  // via sensor or controlled via website
  // green: sensor/auto mode
  // red  : manual mode (via website, or buttons)
  if (mode & SPEED)
  {
      lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_GREEN));
      lv_led_on(gfxLvLedSpeed);
      lv_obj_clear_state(gfxLvSwitchSpeedControlMode, LV_STATE_CHECKED);
  }
  else if((mode & SPEED) == 0)
  {
      lv_led_set_color(gfxLvLedSpeed, lv_palette_main(LV_PALETTE_RED));
      lv_led_on(gfxLvLedSpeed);
      lv_obj_add_state(gfxLvSwitchSpeedControlMode, LV_STATE_CHECKED);
  }
  if (mode & INCLINE)
  {
      lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_GREEN));
      lv_led_on(gfxLvLedIncline);
      lv_obj_clear_state(gfxLvSwitchInclineControlMode, LV_STATE_CHECKED);
  }
  else if ((mode & INCLINE) == 0) {
      lv_led_set_color(gfxLvLedIncline, lv_palette_main(LV_PALETTE_RED));
      lv_led_on(gfxLvLedIncline);
      lv_obj_add_state(gfxLvSwitchInclineControlMode, LV_STATE_CHECKED);
  }
}

void lvgl_gfxUpdateWIFI(const unsigned long reconnect_counter, const String &ip) {
  // show reconnect counter in tft
  // if (wifi_reconnect_counter > wifi_reconnect_counter_prev) ... only update on change
  std::ostringstream oss;
  oss << "Wifi[" << reconnect_counter << "]: " << ip.c_str();
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
}
