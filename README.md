# ESP32_TTGO_FTMS
ESP32 based treadmill speed and incline sensor and BLE Server exposed as FTMS Service

Based/Inspired on/by this project:

https://hackaday.io/project/175237-add-bluetooth-to-treadmill


## Parts Used
* Treadmill Speed Sensor (Reed-Switch)
  https://de.aliexpress.com/item/4000023371194.html?spm=a2g0s.9042311.0.0.556d4c4d8wMaUG
* Infrared-Sensor (Obstacle Avoidance Sensor)
  https://de.aliexpress.com/item/1005001285654366.html?spm=a2g0s.9042311.0.0.27424c4dPrwkYp
* 3 Axis analog gyro sensors+ 3 Axis Accelerometer GY-521 MPU-6050 MPU6050
  https://de.aliexpress.com/item/32340949017.html?spm=a2g0s.9042311.0.0.27424c4dPrwkYp
* Time-of-Flight Laser Ranging Sensor GY-530 VL53L0X (ToF)
  https://de.aliexpress.com/item/32738458924.html?spm=a2g0s.9042311.0.0.556d4c4d8wMaUG


## WIFI config
Create a file named

wifi_mqtt_creds.h

With your version of this content: (e.g your wlan settings)
```
#ifndef _wifi_mqtt_creds_h_
#define _wifi_mqtt_creds_h_

const char* wifi_ssid = "****";
const char* wifi_pass = "****";

const char* mqtt_host_int = "host.org";
const char* mqtt_user_int = "user";
const char* mqtt_pass_int = "passw0rd";
const unsigned mqtt_port_int = 8883;

#endif
```

## Filesystem

Follow the instructions to instal SPIFFS filesystem upload tools here:
https://randomnerdtutorials.com/install-esp32-filesystem-uploader-arduino-ide/

## arduino IDE settings
* Board: ESP32 Dev Module
* Partition: Huge App (no OTA 3MB/1MB SPIFFS)

# JSON serialize / deserialize
## calculate memory requirement
https://arduinojson.org/v6/assistant/

## serialize websocket data
```
{
  "speed": "22.00",
  "incline": "15.0",
  "speed_interval": "10.0",
  "sensor_mode": "0",
  "distance": "9999.99",
  "elevation": "9999.99",
  "hour": "00",
  "minute": "00",
  "second": "00"
}
```

# measurements via oscilloscope
```
km/h vs. Period ms
km/h 	 P ms
22	  40.8
20	  45.0
16	  56.0
12	  76.0
10	  90.4
 8	 112.8
 4	 225
 2	 450
 1	 900
 0.5	1840
```

From this we can also calculate an average distance the magnet travels on the motor wheel of 0.25088m -> 250mm (the circumference).


# Issues
IR-Sensor (here Sensor 1) is connected to GPIO12

GPIO36 interrupt, changed reed-sensor to GPIO32
https://github.com/espressif/esp-idf/issues/4585
