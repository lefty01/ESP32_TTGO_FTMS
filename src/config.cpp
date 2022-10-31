//#include <Arduino.h>  // for string type
#include <string.h> 
//#include <sd.h> 
#include "SPIFFS.h"

#include "config.h"
#include "ESP32_TTGO_FTMS.h"

#define FILE_NAME "treadmill.txt"

#define KEY_MAX_LENGTH    30 // change it if key is longer
#define VALUE_MAX_LENGTH  30 // change it if value is longer

#define default_treadmill_name "Treadmill"
#define default_max_speed  5.0
#define default_min_speed  0.5
#define default_max_incline 5.0
#define default_min_incline 0.0
#define default_speed_interval_min 0.1
#define default_incline_interval_min 1.0
#define default_belt_distance 250
#define default_hasMPU6050 false
#define default_hasVL53L0X false
#define default_hasIrSense false
#define default_hasReed false

#define abs_max_speed  16.0
#define abs_min_speed  0.0
#define abs_max_incline 12.0
#define abs_min_incline 0.0
#define abs_speed_interval_min 0.1
#define abs_incline_interval_min 1.0
#define abs_belt_distance 250

String treadmill_name;
float max_speed;
float min_speed;
float max_incline; // incline/grade in percent(!)
float min_incline;
float speed_interval_min;
float incline_interval_min;
long  belt_distance; // mm ... actually circumfence of motor wheel!
float incline_interval = incline_interval_min;
volatile float speed_interval = speed_interval_min;
bool hasMPU6050;
bool hasVL53L0X;
bool hasIrSense;
bool hasReed;
const char* VERSION = "0.0.22";

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

int SPIFFS_findKey(const __FlashStringHelper * key, char * value) 
{
  char key_string[KEY_MAX_LENGTH];
  char SPIFFS_buffer[KEY_MAX_LENGTH + VALUE_MAX_LENGTH + 1]; // 1 is = character
  int key_length = 0;
  int value_length = 0;

  File configFile = SPIFFS.open(FILE_NAME);

  if (!configFile) {
    logText("SPIFFS: error on opening file ");
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
    int buffer_length = configFile.readBytesUntil('\n', SPIFFS_buffer, 100);
    if (SPIFFS_buffer[buffer_length - 1] == '\r')
      buffer_length--; // trim the \r

    if (buffer_length > (key_length + 1)) { // 1 is = character
      if (memcmp(SPIFFS_buffer, key_string, key_length) == 0) { // equal
        if (SPIFFS_buffer[key_length] == '=') {
          value_length = buffer_length - key_length - 1;
          memcpy(value, SPIFFS_buffer + key_length + 1, value_length);
          break;
        }
      }
    }
  }

  configFile.close();  // close the file
  return value_length;
}


bool SPIFFS_available(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SPIFFS_findKey(key, value_string);
  return value_length > 0;
}

int SPIFFS_findInt(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SPIFFS_findKey(key, value_string);
  return HELPER_ascii2Int(value_string, value_length);
}

float SPIFFS_findFloat(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SPIFFS_findKey(key, value_string);
  return HELPER_ascii2Float(value_string, value_length);
}

String SPIFFS_findString(const __FlashStringHelper * key) {
  char value_string[VALUE_MAX_LENGTH];
  int value_length = SPIFFS_findKey(key, value_string);
  return HELPER_ascii2String(value_string, value_length);
}

void initConfig(void)
{
  logText("initConfig\n");

  treadmill_name = SPIFFS_findString(F("treadmillname"));
  max_speed  = SPIFFS_findFloat(F("max_speed"));
  min_speed  = SPIFFS_findFloat(F("min_speed"));
  max_incline  = SPIFFS_findFloat(F("max_incline"));
  min_incline  = SPIFFS_findFloat(F("min_incline"));
  speed_interval_min  = SPIFFS_findFloat(F("speed_interval_min"));
  incline_interval_min  = SPIFFS_findFloat(F("incline_interval_min"));
  belt_distance  = SPIFFS_findFloat(F("belt_distance"));  
  hasMPU6050    = SPIFFS_findInt(F("hasMPU6050"));
  hasVL53L0X    = SPIFFS_findInt(F("hasVL53L0X"));
  hasIrSense    = SPIFFS_findInt(F("hasIrSense"));
  hasReed    = SPIFFS_findInt(F("hasReed"));

  // check if values were valid, else set to safe defaults
  if (treadmill_name.length() == 0)
  {
    treadmill_name = default_treadmill_name;
    logText("no treadmill name defined, using default");
  }
  
  if ((max_speed > abs_max_speed) || (max_speed < 0))
  {
    max_speed = default_max_speed;
    logText("invalid max speed, using default");
  }

  if ((min_speed > abs_min_speed) || (min_speed < 0))
  {
    min_speed = default_min_speed;
    logText("invalid min speed, using default");  

  }

  if ((max_incline > abs_max_incline) || (max_incline < 0))
  {
    max_incline = default_max_incline;
    logText("invalid max incline, using default");
  }

  if ((min_incline > abs_min_incline) || (min_incline < 0))
  {
    min_incline = default_min_incline;
    logText("invalid min incline, using default");
  }

  if ((speed_interval_min > abs_speed_interval_min) || (speed_interval_min < 0))
  {
    speed_interval_min = default_speed_interval_min;
    logText("invalid min speed interval, using default");
  }

  if ((incline_interval_min > abs_speed_interval_min) || (incline_interval_min < 0))
  {
    incline_interval_min = default_incline_interval_min;
    logText("invalid min incline interval, using default");
  }

  if ((belt_distance > abs_belt_distance) || (belt_distance < 0))
  {
    belt_distance = default_belt_distance;
    logText("invalid belt distance, using default");
  }

  if ((hasMPU6050 != 0) || (hasMPU6050 != 1))
  {
    hasMPU6050 = default_hasMPU6050;
    logText("invalid MPU6050 setting, using default (false)");

  }

  if ((hasVL53L0X != 0) || (hasVL53L0X != 1))
  {
    hasVL53L0X = default_hasVL53L0X;
    logText("invalid VL53L0X setting, using default (false)");
  }

  if ((hasIrSense != 0) || (hasIrSense != 1))
  {
    hasIrSense = default_hasIrSense;
    logText("invalid Ir sense setting, using default (false)");
  }

  if ((hasReed != 0) || (hasReed != 1))
  {
    hasReed = default_hasReed;
    logText("invalid reed sensor setting, using default (false)");
  }

  logText("initConfig Done!\n");  
//  myInt_1    = SPIFFS_findInt(F("myInt_1"));
//  myInt_2    = S_findInt(F("myInt_2"));
//  max_speed  = SPIFFS_findFloat(F("max_speed"));
//  myFloat_2  = SPIFFS_findFloat(F("myFloat_2"));
//  myString_1 = SPIFFS_findString(F("myString_1"));
//  myString_2 = SPIFFS_findString(F("myString_2"));
}