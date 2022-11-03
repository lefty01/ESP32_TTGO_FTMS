#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Arduino.h>   //  for string type
#warning fix include error for string type remove arduino
//#include <string.h> 

extern const char* VERSION;

extern String treadmill_name;
extern float max_speed;
extern float min_speed;
extern float max_incline; // incline/grade in percent(!)
extern float min_incline;
extern float speed_interval_min;
extern long  belt_distance; // mm ... actually circumfence of motor wheel!
extern float incline_interval;
extern volatile float speed_interval;
extern bool hasMPU6050;
extern bool hasVL53L0X;
extern bool hasIrSense;
extern bool hasReed;

void initConfig(void);

#endif