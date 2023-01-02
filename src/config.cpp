#include <string.h> 
#include "debug_print.h"
#include <LittleFS.h>

#include "config.h"
#include "common.h"

#define FILE_NAME "/treadmill.txt"

#define KEY_MAX_LENGTH    30 // change it if key is longer
#define VALUE_MAX_LENGTH  30 // change it if value is longer

#define default_treadmill_name "Treadmill"
#define default_max_speed  5.0
#define default_min_speed  0.5
#define default_max_incline 5.0
#define default_min_incline 0.0
#define default_speed_interval_step 0.1
#define default_incline_interval_step 1.0
#define default_belt_distance 250
#define default_hasMPU6050 false
#define default_hasIrSense false
#define default_hasReed false

#define abs_max_speed  22.0
#define abs_min_speed  0.0
#define abs_max_incline 12.0
#define abs_min_incline 0.0
#define abs_speed_interval_step 1.0
#define abs_incline_interval_step 1.0
#define abs_belt_distance 250

struct TreadmillConfiguration configTreadmill;

const char* VERSION = "0.2.0";


void dump_settings(void)
{
  DEBUG_PRINT("treadmillname: "); 
  DEBUG_PRINTLN(configTreadmill.treadmill_name);
  DEBUG_PRINT("max_speed: "); 
  DEBUG_PRINTLN(configTreadmill.max_speed);
  DEBUG_PRINT("min_speed: "); 
  DEBUG_PRINTLN(configTreadmill.min_speed);
  DEBUG_PRINT("max_incline: "); 
  DEBUG_PRINTLN(configTreadmill.max_incline);
  DEBUG_PRINT("min_incline: "); 
  DEBUG_PRINTLN(configTreadmill.min_incline);
  DEBUG_PRINT("speed_interval step: "); 
  DEBUG_PRINTLN(configTreadmill.speed_interval_step);
  DEBUG_PRINT("incline_interval step: "); 
  DEBUG_PRINTLN(configTreadmill.incline_interval_step);
  DEBUG_PRINT("belt_distance: "); 
  DEBUG_PRINTLN(configTreadmill.belt_distance);
  DEBUG_PRINT("hasMPU6050: "); 
  DEBUG_PRINTLN(configTreadmill.hasMPU6050);
  DEBUG_PRINT("hasIrSense: "); 
  DEBUG_PRINTLN(configTreadmill.hasIrSense);
  DEBUG_PRINT("hasReed: "); 
  DEBUG_PRINTLN(configTreadmill.hasReed);
  DEBUG_PRINTLN("");
}

int HELPER_ascii2Int(char *ascii, int length) {
  int sign = 1;
  int number = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c >= '0' && c <= '9')
        number = number * 10 + (c - '0');
    }
  }

  return number * sign;
}

float HELPER_ascii2Float(char *ascii, int length) {
  int sign = 1;
  int decimalPlace = 0;
  float number  = 0;
  float decimal = 0;

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    if (i == 0 && c == '-')
      sign = -1;
    else {
      if (c == '.')
        decimalPlace = 1;
      else if (c >= '0' && c <= '9') {
        if (!decimalPlace)
          number = number * 10 + (c - '0');
        else {
          decimal += ((float)(c - '0') / pow(10.0, decimalPlace));
          decimalPlace++;
        }
      }
    }
  }

  return (number + decimal) * sign;
}

String HELPER_ascii2String(char *ascii, int length) {
  String str;
  str.reserve(length);
  str = "";

  for (int i = 0; i < length; i++) {
    char c = *(ascii + i);
    str += String(c);
  }

  return str;
}

int LittleFS_findKey(const __FlashStringHelper * key, char * value) 
{
  char key_string[KEY_MAX_LENGTH];
  char LittleFS_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1]; // 1 is = character
  int key_length = 0;
  int value_length = 0;
  File configFile;

  configFile = LittleFS.open(FILE_NAME, "r");  

  if (!configFile) {
    logText("LittleFS: error on opening file ");    
    logText(FILE_NAME);
    return 0;
  }

  // Flash string to string
  PGM_P keyPointer;
  keyPointer = reinterpret_cast<PGM_P>(key);
  byte ch;
  do {
    ch = pgm_read_byte(keyPointer++);
    if (ch != 0)
      key_string[key_length++] = ch;
  } while (ch != 0);

  // check line by line
  while (configFile.available()) {
    int buffer_length = configFile.readBytesUntil('\n', LittleFS_buffer, 100);
    if (LittleFS_buffer[buffer_length - 1] == '\r')
      buffer_length--; // trim the \r

    if (buffer_length > (key_length + 1)) { // 1 is = character
      if (memcmp(LittleFS_buffer, key_string, key_length) == 0) { // equal
        if (LittleFS_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, LittleFS_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}

bool LittleFS_available(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = LittleFS_findKey(key, value_string);
  return value_length > 0;
}

int LittleFS_findInt(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = LittleFS_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float LittleFS_findFloat(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = LittleFS_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String LittleFS_findString(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = LittleFS_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}

void initConfig(void)
{
  logText("initConfig...");
  // Get Treadmill config
  configTreadmill.treadmill_name         = LittleFS_findString(F("treadmillname"));
  configTreadmill.max_speed              = LittleFS_findFloat(F("max_speed"));
  configTreadmill.min_speed              = LittleFS_findFloat(F("min_speed"));
  configTreadmill.max_incline            = LittleFS_findFloat(F("max_incline"));
  configTreadmill.min_incline            = LittleFS_findFloat(F("min_incline"));
  configTreadmill.speed_interval_step    = LittleFS_findFloat(F("speed_interval_step"));
  configTreadmill.incline_interval_step  = LittleFS_findFloat(F("incline_interval_step"));
  configTreadmill.belt_distance          = LittleFS_findFloat(F("belt_distance"));
  // Get Your HW config, e.g. interface to Treadmill and other added HW
  configTreadmill.hasMPU6050             = LittleFS_findInt(F("hasMPU6050"));
  configTreadmill.hasIrSense             = LittleFS_findInt(F("hasIrSense"));
  configTreadmill.hasReed                = LittleFS_findInt(F("hasReed"));

  // check if values were valid, else set to safe defaults
  if (configTreadmill.treadmill_name.length() == 0) {
    configTreadmill.treadmill_name = default_treadmill_name;
    logText("no treadmill name defined, using default\n");
  }
  
  if ((configTreadmill.max_speed > abs_max_speed) || (configTreadmill.max_speed < 0)) {
    configTreadmill.max_speed = default_max_speed;
    logText("invalid max speed, using default\n");
  }

  if ((configTreadmill.min_speed < abs_min_speed) || (configTreadmill.min_speed < 0)) {
    configTreadmill.min_speed = default_min_speed;
    logText("invalid min speed, using default\n");  
  }

  if ((configTreadmill.max_incline > abs_max_incline) || (configTreadmill.max_incline < 0)) {
    configTreadmill.max_incline = default_max_incline;
    logText("invalid max incline, using default\n");
  }

  if ((configTreadmill.min_incline < abs_min_incline) || (configTreadmill.min_incline < 0)) {
    configTreadmill.min_incline = default_min_incline;
    logText("invalid min incline, using default\n");
  }

  if ((configTreadmill.speed_interval_step > abs_speed_interval_step) || (configTreadmill.speed_interval_step < 0)) {
    configTreadmill.speed_interval_step = default_speed_interval_step;
    logText("invalid speed interval step, using default\n");
  }

  if ((configTreadmill.incline_interval_step > abs_incline_interval_step) || (configTreadmill.incline_interval_step < 0)) {
    configTreadmill.incline_interval_step = default_incline_interval_step;
    logText("invalid incline interval step, using default\n");
  }

  if ((configTreadmill.belt_distance > abs_belt_distance) || (configTreadmill.belt_distance < 0)) {
    configTreadmill.belt_distance = default_belt_distance;
    logText("invalid belt distance, using default\n");
  }

  if ((configTreadmill.hasMPU6050 < 0) || (configTreadmill.hasMPU6050 > 1)) {
    configTreadmill.hasMPU6050 = default_hasMPU6050;
    logText("invalid MPU6050 setting, using default (false)\n");

  }

  if ((configTreadmill.hasIrSense < 0) || (configTreadmill.hasIrSense > 1)) {
    configTreadmill.hasIrSense = default_hasIrSense;
    logText("invalid Ir sense setting, using default (false)\n");
  }

  if ((configTreadmill.hasReed < 0) || (configTreadmill.hasReed > 1)) {
    configTreadmill.hasReed = default_hasReed;
    logText("invalid reed sensor setting, using default (false)\n");
  }

  speedInclineMode = configTreadmill.hasReed ? SPEED : MANUAL;

  dump_settings();
  logText("done\n");  
}
