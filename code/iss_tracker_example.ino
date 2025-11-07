// ===== ESP32 ISS Tracker: LCD + LEDs + Buzzer (KM version) =====
// Board: ESP32 Dev Module

#include <Wire.h>
#include <WiFi.h>
#include <HTTPClient.h>
#include <WiFiClient.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>

// ---------- WiFi ----------
const char* ssid     = "your_wifi_ssid";
const char* password = "your_wifi_password";

// ---------- Your location (decimal degrees) ----------
float myLongitude = YOURLONGITUDE;  // e.g., 8.8078 - e.g. Bremen/Germany
float myLatitude  = YOURLATITUDE;   // e.g., 53.0752

// Simple "nearby box" (+/- 5 deg) — kept as-is for green LED+buzzer
float maxLat, minLat, maxLong, minLong;

// ---------- Pins ----------
const int greenLed = 33;   // green LED (you wired this)
const int whiteLed = 25;   // white LED (220 Ω to GND)
const int buzzer   = 23;   // active buzzer

// ---------- LCD (I2C) ----------
#define LCD_ADDR 0x27      // try 0x27 first; if blank, change to 0x3F
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- ISS API ----------
String serverName = "http://api.open-notify.org/iss-now.json";

// ---------- Timing ----------
unsigned long lastTime   = 0;
unsigned long timerDelay = 5000;  // poll every 5s

// ---------- Current ISS coords (globals) ----------
float longitude = -200.0f;   // invalid sentinel
float latitude  = -200.0f;

// ---------- Helpers ----------
static inline float deg2rad(float d) { return d * (float)M_PI / 180.0f; }

// Great-circle distance between two lat/lon points (km)
float haversineKm(float lat1, float lon1, float lat2, float lon2) {
  const float R = 6371.0f; // Earth radius (km)
  float dLat = deg2rad(lat2 - lat1);
  float dLon = deg2rad(lon2 - lon1);
  float a = sinf(dLat/2)*sinf(dLat/2) +
            cosf(deg2rad(lat1)) * cosf(deg2rad(lat2)) *
            sinf(dLon/2)*sinf(dLon/2);
  float c = 2.0f * atanf(sqrtf(a) / sqrtf(1.0f - a));
  return R * c;
}

std::pair<float, float> getCoordinates() {
  std::pair<float, float> coordinates(-200.0f, -200.0f);

  if (WiFi.status() == WL_CONNECTED) {
    WiFiClient client;
    HTTPClient http;
    if (http.begin(client, serverName)) {
      int code = http.GET();
      if (code > 0) {
        String payload = http.getString();
        JSONVar obj = JSON.parse(payload);
        if (JSON.typeof(obj) != "undefined") {
          const char* lonStr = (const char*)obj["iss_position"]["longitude"];
          const char* latStr = (const char*)obj["iss_position"]["latitude"];
          if (lonStr && latStr) {
            coordinates.first  = atof(lonStr);
            coordinates.second = atof(latStr);
          }
        }
      } else {
        Serial.print("HTTP error: "); Serial.println(code);
      }
      http.end();
    }
  }
  return coordinates;
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // Precompute box bounds (for green LED + buzzer)
  maxLat  = myLatitude + 5;
  minLat  = myLatitude - 5;
  maxLong = myLongitude + 5;
  minLong = myLongitude - 5;

  // GPIO
  pinMode(greenLed, OUTPUT); digitalWrite(greenLed, LOW);
  pinMode(whiteLed, OUTPUT); digitalWrite(whiteLed, LOW);
  pinMode(buzzer,   OUTPUT); digitalWrite(buzzer,   LOW);

  // I2C + LCD
  Wire.begin(21, 22);   // SDA = 21, SCL = 22
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("Connecting...");

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(250);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: "); Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0); lcd.print("WiFi connected");
  delay(800);
}

void loop() {
  static float lastKm = -1.0f;

  // Poll ISS API on timer
  if ((millis() - lastTime) > timerDelay) {
    auto coords = getCoordinates();
    longitude = coords.first;
    latitude  = coords.second;

    lcd.clear();
    if (longitude != -200.0f) {
      float km = haversineKm(myLatitude, myLongitude, latitude, longitude);
      lastKm = km;

      // Line 1: ISS lon,lat (rounded)
      lcd.setCursor(0, 0);
      lcd.print("ISS:");
      lcd.print(longitude, 1);
      lcd.print(",");
      lcd.print(latitude, 1);

      // Line 2: Distance in km
      lcd.setCursor(0, 1);
      lcd.print("Dist:");
      if (km < 9999.5f) {
        lcd.print((int)roundf(km));
        lcd.print(" km   ");
      } else {
        lcd.print(">9999km ");
      }

      Serial.printf("ISS lon: %.4f, lat: %.4f, dist: %.1f km\n", longitude, latitude, km);

      // Proximity box → green LED + buzzer (kept same logic)
      if ( (longitude > minLong && longitude < maxLong) &&
           (latitude  > minLat  && latitude  < maxLat) ) {
        digitalWrite(greenLed, HIGH);
        digitalWrite(buzzer,   HIGH);
      } else {
        digitalWrite(greenLed, LOW);
        digitalWrite(buzzer,   LOW);
      }
    } else {
      // Error state on LCD
      lcd.setCursor(0, 0); lcd.print("Fetch failed");
      lcd.setCursor(0, 1); lcd.print("Retrying...");
      digitalWrite(greenLed, LOW);
      digitalWrite(buzzer,   LOW);
      lastKm = -1.0f;
    }

    lastTime = millis();
  }

  // --- White LED proximity indicator (KM-based) ---
  static unsigned long lastBlink = 0;
  static bool ledState = false;

  // Blink tiers by distance to ISS subpoint
  //   <  500 km  → very fast
  //   < 1000 km  → fast
  //   < 2000 km  → medium
  //   else       → slow
  int blinkDelay = -1; // off by default
  if (WiFi.status() == WL_CONNECTED && longitude != -200.0f && lastKm >= 0.0f) {
    if      (lastKm < 500.0f)  blinkDelay = 150;
    else if (lastKm < 1000.0f) blinkDelay = 300;
    else if (lastKm < 2000.0f) blinkDelay = 600;
    else                       blinkDelay = 1000;

    if ((int)(millis() - lastBlink) >= blinkDelay) {
      ledState = !ledState;
      digitalWrite(whiteLed, ledState ? HIGH : LOW);
      lastBlink = millis();
    }
  } else {
    digitalWrite(whiteLed, LOW);
  }
}
