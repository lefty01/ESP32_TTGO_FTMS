# ESP32_TTGO_FTMS
ESP32 based treadmill speed and incline sensor and BLE Server exposed as FTMS Service


# JSON serialize / deserialize
https://arduinojson.org/v6/assistant/

## serialize
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

### memory / code
Data structures	144	Bytes needed to stores the JSON objects and arrays in memory 
Strings	43	Bytes needed to stores the strings in memory 
Total (minimum)	187	Minimum capacity for the JsonDocument.
Total (recommended)	256	Including some slack in case the strings change, and rounded to a power of two

```
StaticJsonDocument<256> doc;

doc["speed"] = "22.00";
doc["incline"] = "15.0";
doc["speed_interval"] = "10.0";
doc["sensor_mode"] = "0";
doc["distance"] = "9999.99";
doc["elevation"] = "9999.99";
doc["hour"] = "00";
doc["minute"] = "00";
doc["second"] = "00";

serializeJson(doc, output);
```

# measurements

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

this results in diameter of "wheel" magnet-to-axis distance in mm: 25
