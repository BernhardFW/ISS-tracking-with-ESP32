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

<img src ="https://github.com/BernhardFW/ISS-tracking-with-ESP32/blob/main/images/20251107_123037.png" width=400px>

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

