[![PlatformIO CI status](https://github.com/lefty01/ESP32_TTGO_FTMS/actions/workflows/platformio-ci.yml/badge.svg)](https://github.com/lefty01/ESP32_TTGO_FTMS/actions/workflows/platformio-ci.yml)



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

Reed switch is installed next to the original treadmill reed contact and connected via external pull-up.

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

GPIO36 interrupt, changed reed-sensor to GPIO26
https://github.com/espressif/esp-idf/issues/4585


# PlatformIO

```
$ pio run -e ESP32_TTGO_DISPLAY_TFT_eSPI --list-targets
Environment                  Group     Name         Title                        Description
---------------------------  --------  -----------  ---------------------------  ------------------------------------------------------------
ESP32_TTGO_DISPLAY_TFT_eSPI  Advanced  compiledb    Compilation Database         Generate compilation database `compile_commands.json`
ESP32_TTGO_DISPLAY_TFT_eSPI  General   clean        Clean
ESP32_TTGO_DISPLAY_TFT_eSPI  General   cleanall     Clean All                    Clean a build environment and installed library dependencies
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  buildfs      Build Filesystem Image
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  erase        Erase Flash
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  size         Program Size                 Calculate program size
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  upload       Upload
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  uploadfs     Upload Filesystem Image
ESP32_TTGO_DISPLAY_TFT_eSPI  Platform  uploadfsota  Upload Filesystem Image OTA
```

```
$ pio run -e ESP32_TTGO_DISPLAY_TFT_eSPI
```

```
$ pio run -e ESP32_TTGO_DISPLAY_TFT_eSPI -t buildfs
```

```
$ pio run -e ESP32_TTGO_DISPLAY_TFT_eSPI -t upload
```


## Flash Files
* firmware:   .pio/build/ESP32_TTGO_DISPLAY_TFT_eSPI/firmware.bin
* filesystem: .pio/build/ESP32_TTGO_DISPLAY_TFT_eSPI/spiffs.bin


## Partition Table
### read partition table (newer systems might use 0x9000 address, same size)

```
$ esptool.py  read_flash 0x8000 0xc00 ptable_0x8000.img
esptool.py v3.2-dev
Found 1 serial ports
Serial port /dev/ttyUSB0
Connecting........_
Detecting chip type... ESP32
Chip is ESP32-D0WDQ6-V3 (revision 3)
Features: WiFi, BT, Dual Core, 240MHz, VRef calibration in efuse, Coding Scheme None
Crystal is 40MHz
MAC: 84:cc:a8:60:d5:3c
Uploading stub...
Running stub...
Stub running...
3072 (100 %)
3072 (100 %)
Read 3072 bytes at 0x8000 in 0.3 seconds (86.9 kbit/s)...
Hard resetting via RTS pin...
```

```
$ gen_esp32part.py --flash-size 4MB ptable_0x8000.img
Parsing binary partition input...
Verifying table...
# ESP-IDF Partition Table
# Name, Type, SubType, Offset, Size, Flags
nvs,data,nvs,0x9000,20K,
otadata,data,ota,0xe000,8K,
app0,app,ota_0,0x10000,1280K,
app1,app,ota_1,0x150000,1280K,
spiffs,data,spiffs,0x290000,1472K,
```

### New partition table used for ESP32_TTGO_DISPLAY_TFT_eSPI_SSL
Modified table by increasing app partition(s) and decrease data (spiffs) partition.
Currently data contains roughly 10k website data (html + js + css). New spiffs data partition has size of 448kB.
At time of this commit size of 'standard' (non-ssl) app is 1196070 bytes and ssl version is 1352014 bytes.
Available space for app with this table is: 1835008 (0x1C0000) bytes

```
# Name,   Type, SubType, Offset,  Size, Flags
nvs,      data, nvs,     0x009000,  0x005000,
otadata,  data, ota,     0x00e000,  0x002000,
app0,     app,  ota_0,   0x010000,  0x1C0000,
app1,     app,  ota_1,   0x1D0000,  0x1C0000,
spiffs,   data, spiffs,  0x390000,  0x070000,
```


# Thanks / Credits
... for providing inputs / thoughts / pull-requests
* Zingo Andersen
* Reiner Ziegler
