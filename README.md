# 🛰️ ISS Tracking with ESP32

[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/language-C++-orange.svg)](https://www.arduino.cc/)
[![Made with Arduino](https://img.shields.io/badge/Made%20with-Arduino-00979D.svg?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![License](https://img.shields.io/github/license/BernhardFW/ISS-tracking-with-ESP32)](https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/license)
[![GitHub last commit](https://img.shields.io/github/last-commit/BernhardFW/ISS-tracking-with-ESP32)](https://github.com/BernhardFW/ISS-tracking-with-ESP32/commits/main)

> 🛰️ Track the International Space Station in real time using an ESP32, LCD display, LEDs, and a buzzer!  
> Displays live ISS position and distance in kilometers and gives visual/audible alerts when it’s nearby.

Thanks to Pushpendra of India I modified the Arduino script (assisted by chatgpt-thanks) to track the ISS and have a chance to see it live if weather permitting...
Pushpendra has a pretty good documentation on this here: https://www.instructables.com/ISSInternational-Space-Station-Tracker/


---

## 🧰 Hardware Setup
- **ESP32-WROOM-32** development board  
- **I²C 16×2 LCD** (`0x27` or `0x3F`)  
- **White LED** (GPIO 25) — blinks faster as the ISS gets closer  
- **Green LED** (GPIO 33) — lights when ISS is overhead  
- **Active buzzer** (GPIO 23) — sounds when ISS is within proximity  

---

## 🖼️ Demo

<img src ="https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/images/20251107_123037.png" width=400px>

In Code Version3 (iss_trackerv3.ino) I have added two additional alternating pages on the display as follows:

<img src ="https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/images/20251109_161727.png" width=400px>
Showing better understandable names of the actual ISS locations over-flying like "Atlantic Ocean" or "Read Sea" or "Kuwait City" or "Harad/Iran"...
This is based on a "free reverse-geocode API" provided by <a target=new href="https://bigdatacloud.com">https://bigdatacloud.com</a> - thanks to them.

<img src ="https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/images/20251109_161733.png" width=400px>  
showing the Next: 12.11.18:59 date overflying me... (in Germany) and the 
2nd line: T-3d (in 3 days) and A202 (in 202 dgr = SouthSouthWest) and M14 (Max. 14 dgr above the horizon.

---

## ⚙️ Software
- Arduino IDE or PlatformIO
- Libraries:
  - `WiFi.h`
  - `HTTPClient.h`
  - `Arduino_JSON`
  - `LiquidCrystal_I2C`

The code you will find in the above directory.
Make shure you have added libraries as imported on top...
The "wiring" is pretty much straight forward and you find it in the "wiring" directory.

If the display does not show anything, try to adjust the potentiometer.

---

## 🌍 Features
✅ Fetches live ISS coordinates from `api.open-notify.org`  
✅ Calculates great-circle distance (Haversine) in km  
✅ Displays position + distance on LCD  
✅ LEDs + buzzer indicate proximity  
✅ Simple, educational, fun hardware project  

---

## 🧠 Future ideas
- Add OLED or TFT screen with orbit map  
- Add NeoPixel ring for distance visualization  
- Add MQTT or WiFi dashboard output  

---

