# ISS-tracking-with-ESP32
My goal and target: See the International Space Station ISS flying over my head... (;-)

Thanks to Pushpendra of India I modified this Arduino script (assisted by chatgpt-thanks) to track the ISS and have chance to see it live if weather permits...

I was motivated by Pushpendra and his small project: https://www.instructables.com/ISSInternational-Space-Station-Tracker/

As I had a ESP32-WROOM-32 around but was too lazy to attach that many LEDs I used my LCD display with 2 lines and 16 block characters to show me the distance to the ISS and its position...

Displays live ISS coordinates, distance in km, and signals when it’s nearby.

## Setup
- ESP32-WROOM-32
- I2C 16x2 LCD display
- Green + White LEDs
- Active buzzer

## Demo

![ISS Tracker Setup](https://raw.githubusercontent.com/BernhardFW/ISS-tracking-with-ESP32/main/images/20251107_123037.png)

The code you will find in the above directory.
Make shure you have added libraries as imported on top...
The "wiring" is pretty much straight forward and you find it in the "wiring" directory.

If the display does not show anything, try to adjust the 

<img src ="https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/images/20251107_123037.png" width=400px>

# 🛰️ ISS Tracking with ESP32

[![Platform](https://img.shields.io/badge/platform-ESP32-blue.svg)](https://www.espressif.com/en/products/socs/esp32)
[![Language](https://img.shields.io/badge/language-C++-orange.svg)](https://www.arduino.cc/)
[![Made with Arduino](https://img.shields.io/badge/Made%20with-Arduino-00979D.svg?logo=arduino&logoColor=white)](https://www.arduino.cc/)
[![License](https://img.shields.io/github/license/BernhardFW/ISS-tracking-with-ESP32)](https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/LICENSE)
[![GitHub last commit](https://img.shields.io/github/last-commit/BernhardFW/ISS-tracking-with-ESP32)](https://github.com/BernhardFW/ISS-tracking-with-ESP32/commits/main)

> 🛰️ Track the International Space Station in real time using an ESP32, LCD display, LEDs, and a buzzer!  
> Displays live ISS position and distance in kilometers and gives visual/audible alerts when it’s nearby.

---

## 🧰 Hardware Setup
- **ESP32-WROOM-32** development board  
- **I²C 16×2 LCD** (`0x27` or `0x3F`)  
- **White LED** (GPIO 25) — blinks faster as the ISS gets closer  
- **Green LED** (GPIO 33) — lights when ISS is overhead  
- **Active buzzer** (GPIO 23) — sounds when ISS is within proximity  

---

## 🖼️ Demo

![ISS Tracker Setup](https://raw.githubusercontent.com/BernhardFW/ISS-tracking-with-ESP32/main/images/20251107_123037.png)

---

## ⚙️ Software
- Arduino IDE or PlatformIO
- Libraries:
  - `WiFi.h`
  - `HTTPClient.h`
  - `Arduino_JSON`
  - `LiquidCrystal_I2C`

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

