#ifndef _CONFIG_H_
#define _CONFIG_H_

#include <Arduino.h>   //  for string type
//#warning fix include error for string type remove arduino (use std:string?? it might be faster)
//#include <string.h>

extern const char* VERSION;

struct TreadmillConfiguration
{
    public:
    String treadmill_name;
    float max_speed;
    float min_speed;
    float max_incline; // incline/grade in percent(!)
    float min_incline;
    float incline_interval_step;
    float speed_interval_step;
    long  belt_distance; // mm ... actually circumfence of motor wheel!
    bool  hasMPU6050;
    bool  hasMPU6050inAngle;
    bool  hasIrSense;
    bool  hasReed;
};

extern TreadmillConfiguration configTreadmill;

void initConfig(void);

#endif
