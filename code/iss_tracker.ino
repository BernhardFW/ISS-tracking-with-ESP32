// ===== ESP32 ISS Tracker (LCD + LEDs + Buzzer) =====
// Shows: Next pass local time + countdown + az/max elev, live elevation (°) & azimuth (°), and distance (km)

#include <Wire.h>
#include <WiFi.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>  // HTTPS for N2YO
#include <HTTPClient.h>
#include <Arduino_JSON.h>
#include <LiquidCrystal_I2C.h>
#include <math.h>
#include <time.h>

// ---------- WiFi ----------
const char* ssid = "FRITZ!Box 7490 BC";
const char* password = "Bremen2020";

// ---------- Your location (decimal degrees) ----------
float myLongitude = 10.36615;  // e.g., Bremen area
float myLatitude = 53.32575;

// --- N2YO (visible pass) ---
#define N2YO_API_KEY "SWUN2H-SRG6C2-KZNZZE-5LLT"
// observer altitude (meters above sea level); set ~10 for Bremen, or your true altitude
#define OBSERVER_ALT_M 10
// how many days ahead to search, and minimum seconds the pass is visible
#define N2YO_DAYS_AHEAD 7
#define N2YO_MIN_VIS_SECS 60

// ---------- Pins (ESP32) ----------
const int greenLed = 33;  // green LED
const int whiteLed = 25;  // white LED (distance-blink)
const int buzzer = 23;    // active buzzer

// ---------- LCD (I2C) ----------
#define LCD_ADDR 0x27  // try 0x27 first; if blank, change to 0x3F
LiquidCrystal_I2C lcd(LCD_ADDR, 16, 2);

// ---------- APIs ----------
String issNowURL = "http://api.open-notify.org/iss-now.json";

// ---------- Timing ----------
unsigned long lastNowPoll = 0;
unsigned long lastPassPoll = 0;
const unsigned long nowPollMs = 5000;               // iss-now every 5s
const unsigned long passPollMs = 10UL * 60 * 1000;  // next pass every 10 min

// ---------- Current ISS coords ----------
float issLon = -200.0f;
float issLat = -200.0f;

// ---------- Next pass (UTC epoch + duration + extra details) ----------
time_t nextRiseUtc = 0;
int nextDurSec = 0;
int nextStartAz = -1;  // degrees 0..360, 0=N, 90=E, 180=S
int nextMaxEl = -1;    // degrees above horizon

// ---------- NTP / Timezone ----------
const char* TZ_EU_BERLIN = "CET-1CEST,M3.5.0/2,M10.5.0/3";
bool timeReady = false;

// ---------- Alert threshold ----------
const float ALERT_KM = 1200.0f;  // turn on green LED + buzzer within 1200 km

// ---------- Math helpers ----------
static inline double deg2rad(double d) {
  return d * M_PI / 180.0;
}
static inline double rad2deg(double r) {
  return r * 180.0 / M_PI;
}

// Great-circle distance (km) using haversine
double haversineKm(double lat1, double lon1, double lat2, double lon2) {
  const double R = 6371.0;
  double dLat = deg2rad(lat2 - lat1);
  double dLon = deg2rad(lon2 - lon1);
  double a = sin(dLat / 2) * sin(dLat / 2) + cos(deg2rad(lat1)) * cos(deg2rad(lat2)) * sin(dLon / 2) * sin(dLon / 2);
  double c = 2.0 * atan2(sqrt(a), sqrt(1.0 - a));
  return R * c;
}

// Initial bearing (azimuth 0..360) from point 1 -> point 2 on sphere
double bearingDeg(double lat1, double lon1, double lat2, double lon2) {
  double φ1 = deg2rad(lat1), φ2 = deg2rad(lat2), Δλ = deg2rad(lon2 - lon1);
  double y = sin(Δλ) * cos(φ2);
  double x = cos(φ1) * sin(φ2) - sin(φ1) * cos(φ2) * cos(Δλ);
  double brng = atan2(y, x);
  double deg = fmod((rad2deg(brng) + 360.0), 360.0);
  return deg;  // 0=N, 90=E, 180=S, 270=W
}

// Approx elevation (deg) from observer to satellite using subsatellite point & assumed altitude
double elevationDegApprox(double obsLat, double obsLon, double satSubLat, double satSubLon) {
  const double Re = 6371.0;  // Earth radius (km)
  const double H = 420.0;    // ISS altitude (km) ~408–420
  double gamma = acos(
    sin(deg2rad(obsLat)) * sin(deg2rad(satSubLat)) + cos(deg2rad(obsLat)) * cos(deg2rad(satSubLat)) * cos(deg2rad(satSubLon - obsLon)));  // central angle (rad)
  double num = cos(gamma) - Re / (Re + H);
  double den = sin(gamma);
  double elev = atan2(num, den);
  return rad2deg(elev);  // negative when below horizon
}

// ---------- HTTP helpers ----------
bool httpGET(const String& url, String& bodyOut) {
  WiFiClient client;
  HTTPClient http;
  if (!http.begin(client, url)) return false;
  int code = http.GET();
  if (code <= 0) {
    http.end();
    return false;
  }
  bodyOut = http.getString();
  http.end();
  return (code == 200 && bodyOut.length() > 0);
}

bool fetchISSNow(float& lonOut, float& latOut) {
  String payload;
  if (!httpGET(issNowURL, payload)) return false;
  JSONVar obj = JSON.parse(payload);
  if (JSON.typeof(obj) == "undefined") return false;
  const char* lonStr = (const char*)obj["iss_position"]["longitude"];
  const char* latStr = (const char*)obj["iss_position"]["latitude"];
  if (!lonStr || !latStr) return false;
  lonOut = atof(lonStr);
  latOut = atof(latStr);
  return true;
}

// Fetch next visible pass from N2YO (HTTPS)
bool fetchNextPass(time_t& riseUtcOut, int& durSecOut) {
  if (String(N2YO_API_KEY).length() < 5) return false;

  // https://api.n2yo.com/rest/v1/satellite/visualpasses/25544/lat/lon/alt/days/minvis&apiKey=KEY
  String url = "https://api.n2yo.com/rest/v1/satellite/visualpasses/25544/";
  url += String(myLatitude, 6) + "/";
  url += String(myLongitude, 6) + "/";
  url += String((int)OBSERVER_ALT_M) + "/";
  url += String(N2YO_DAYS_AHEAD) + "/";
  url += String(N2YO_MIN_VIS_SECS);
  url += "/&apiKey=" + String(N2YO_API_KEY);

  WiFiClientSecure sclient;
  sclient.setInsecure();  // skip TLS cert validation
  HTTPClient http;
  if (!http.begin(sclient, url)) return false;

  int code = http.GET();
  if (code != 200) {
    http.end();
    return false;
  }
  String payload = http.getString();
  http.end();

  JSONVar obj = JSON.parse(payload);
  if (JSON.typeof(obj) == "undefined") return false;
  if (!obj.hasOwnProperty("passes")) return false;
  JSONVar passes = obj["passes"];
  if (passes.length() < 1) return false;

  // take the first visible pass
  JSONVar p = passes[0];

  long startUTC = (long)p["startUTC"];
  long duration = (long)p["duration"];
  if (startUTC <= 0 || duration <= 0) return false;

  riseUtcOut = (time_t)startUTC;
  durSecOut = (int)duration;

  // Extra details
  nextStartAz = p.hasOwnProperty("startAz") ? (int)p["startAz"] : -1;
  nextMaxEl = p.hasOwnProperty("maxEl") ? (int)p["maxEl"] : -1;

  return true;
}

// ---------- Time helpers ----------
void initTime() {
  configTzTime(TZ_EU_BERLIN, "pool.ntp.org", "time.nist.gov");
  for (int i = 0; i < 50; i++) {  // ~10s max
    time_t now = time(nullptr);
    if (now > 1700000000) {
      timeReady = true;
      break;
    }  // sanity (2023+)
    delay(200);
  }
}

// Smarter compact countdown:
// - >= 48h  → "T-4d02h"
// - >= 1h   → "T-98:20"  (hh:mm)
// - <  1h   → "T-12:34"  (mm:ss)
// During pass → "T+..", after pass → "passed"
String fmtTcompact(time_t startUtc, int durSec) {
  time_t now = time(nullptr);
  long diff = (long)difftime(startUtc, now);
  char buf[12];

  auto fmt_days = [&](long secs) {
    long d = secs / 86400;
    long h = (secs % 86400) / 3600;
    snprintf(buf, sizeof(buf), "%ldd%02lh", d, h);  // e.g. "4d02h"
  };
  auto fmt_hhmm = [&](long secs) {
    long h = secs / 3600;
    long m = (secs % 3600) / 60;
    snprintf(buf, sizeof(buf), "%ld:%02ld", h, m);  // "98:20"
  };
  auto fmt_mmss = [&](long secs) {
    long m = secs / 60, s = secs % 60;
    snprintf(buf, sizeof(buf), "%02ld:%02ld", m, s);  // "12:34"
  };

  if (diff >= 0) {  // before pass
    if (diff >= 48L * 3600L) {
      fmt_days(diff);
      return String("T-") + buf;
    }
    if (diff >= 3600L) {
      fmt_hhmm(diff);
      return String("T-") + buf;
    }
    fmt_mmss(diff);
    return String("T-") + buf;
  } else {  // pass has started or finished
    long past = -diff;
    if (past <= durSec) {
      // During pass → count up
      if (past >= 3600L) {
        fmt_hhmm(past);
        return String("T+") + buf;
      } else {
        fmt_mmss(past);
        return String("T+") + buf;
      }
    } else {
      return String("passed");
    }
  }
}

// Local time string (Berlin) if synced, else UTC with 'Z'
String fmtTimeLocalOrUTC(time_t t) {
  struct tm tmv;
  char buf[32];
  if (timeReady) {
    localtime_r(&t, &tmv);
    strftime(buf, sizeof(buf), "%d.%m %H:%M", &tmv);
  } else {
    gmtime_r(&t, &tmv);
    strftime(buf, sizeof(buf), "%d.%m %H:%MZ", &tmv);
  }
  return String(buf);
}

void setup() {
  Serial.begin(115200);
  delay(100);

  // GPIO
  pinMode(greenLed, OUTPUT);
  digitalWrite(greenLed, LOW);
  pinMode(whiteLed, OUTPUT);
  digitalWrite(whiteLed, LOW);
  pinMode(buzzer, OUTPUT);
  digitalWrite(buzzer, LOW);

  // I2C + LCD
  Wire.begin(21, 22);  // ESP32 default: SDA=21, SCL=22
  lcd.init();
  lcd.backlight();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("Connecting WiFi");

  // WiFi
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(300);
    Serial.print(".");
  }
  Serial.println();
  Serial.print("IP: ");
  Serial.println(WiFi.localIP());

  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print("WiFi OK");
  lcd.setCursor(0, 1);
  lcd.print(WiFi.localIP().toString());
  delay(800);

  // Time
  initTime();
  lcd.clear();
  lcd.setCursor(0, 0);
  lcd.print(timeReady ? "Time synced" : "Time pending");
  delay(600);
}
void loop() {
  static float lastKm = -1.0f;
  static unsigned long lastPageFlip = 0;
  static bool page = false;  // false=NOW page, true=NEXT PASS page

  // ---- Auto-clear old pass so a fresh one is fetched soon after it ends ----
  if (nextRiseUtc > 0 && nextDurSec > 0) {
    time_t nowt = time(nullptr);
    if ((long)difftime(nowt, nextRiseUtc) > (nextDurSec + 60)) {  // 60s buffer
      nextRiseUtc = 0;
      nextDurSec = 0;
      nextStartAz = -1;
      nextMaxEl = -1;
      // lastPassPoll = 0; // uncomment to force immediate refetch
    }
  }

  // ---- Poll current ISS position ----
  if (millis() - lastNowPoll > nowPollMs) {
    float lon, lat;
    bool ok = (WiFi.status() == WL_CONNECTED) && fetchISSNow(lon, lat);
    if (ok) {
      issLon = lon;
      issLat = lat;
      lastKm = (float)haversineKm(myLatitude, myLongitude, issLat, issLon);
    }
    lastNowPoll = millis();
  }

  // ---- Poll next pass (risetime & duration) ----
  if (millis() - lastPassPoll > passPollMs || nextRiseUtc == 0) {
    time_t r;
    int d;
    if (WiFi.status() == WL_CONNECTED && fetchNextPass(r, d)) {
      nextRiseUtc = r;
      nextDurSec = d;
    }
    lastPassPoll = millis();
  }

  // ---- Alternate LCD pages every ~4 seconds ----
  if (millis() - lastPageFlip > 4000) {
    page = !page;
    lastPageFlip = millis();
  }

  // ---- Render LCD ----
  lcd.clear();
  if (!page) {
    // ---- Page 1: NOW (live) ----
    double elev = -90.0, az = 0.0;
    if (issLon > -190.0) {  // valid coords received
      elev = elevationDegApprox(myLatitude, myLongitude, issLat, issLon);
      az = bearingDeg(myLatitude, myLongitude, issLat, issLon);
    }

    // Row 1: Elev & Az
    lcd.setCursor(0, 0);
    lcd.print("El:");
    lcd.print((int)round(elev));
    lcd.print((char)223);  // degree symbol
    lcd.print(" Az:");
    lcd.print((int)round(az));

    // Row 2: Dist
    lcd.setCursor(0, 1);
    lcd.print("Dist:");
    if (lastKm >= 0) {
      lcd.print((int)round(lastKm));
      lcd.print("km");
    } else {
      lcd.print("--");
    }

  } else {
    // ---- Page 2: NEXT PASS ----
    lcd.setCursor(0, 0);
    lcd.print("Next:");
    if (nextRiseUtc > 0) {
      lcd.print(fmtTimeLocalOrUTC(nextRiseUtc));
    } else {
      lcd.print("fetching...");
    }

    // Row 2: countdown + az + max elev
    lcd.setCursor(0, 1);
    if (nextRiseUtc > 0 && nextDurSec > 0) {
      String cd = fmtTcompact(nextRiseUtc, nextDurSec);  // e.g. "T-4d02h", "T-12:34"
      char line[17];
      if (nextStartAz >= 0 && nextMaxEl >= 0) {
        // exactly 16 cols: 8 for countdown + "A%03d M%02d"
        // e.g. "T-12:34A230 M62" or "T-4d02hA201 M13"
        snprintf(line, sizeof(line), "%-8sA%03d M%02d", cd.c_str(), nextStartAz, nextMaxEl);
      } else {
        snprintf(line, sizeof(line), "%-16s", cd.c_str());
      }
      line[16] = '\0';
      lcd.print(line);
    } else {
      lcd.print("--              ");
    }
  }

  // ---- Green LED + buzzer pattern (within 1200 km) ----
  static unsigned long buzzerTimer = 0;
  static int buzzerState = 0;  // 0=idle, 1=beep on, 2=beep off-in-burst, 3=pause
  static int beepCount = 0;

  if (lastKm >= 0 && lastKm <= ALERT_KM) {
    digitalWrite(greenLed, HIGH);

    unsigned long nowms = millis();
    switch (buzzerState) {
      case 0:  // start a burst
        digitalWrite(buzzer, HIGH);
        buzzerTimer = nowms + 200;  // beep 200 ms
        buzzerState = 1;
        break;

      case 1:  // short beep ON
        if (nowms >= buzzerTimer) {
          digitalWrite(buzzer, LOW);
          buzzerTimer = nowms + 200;  // 200 ms gap
          buzzerState = 2;
        }
        break;

      case 2:  // short gap between beeps
        if (nowms >= buzzerTimer) {
          beepCount++;
          if (beepCount < 3) {
            digitalWrite(buzzer, HIGH);
            buzzerTimer = nowms + 200;
            buzzerState = 1;
          } else {
            // finished three beeps, start long pause
            beepCount = 0;
            buzzerTimer = nowms + 5000;  // 5 s pause
            buzzerState = 3;
          }
        }
        break;

      case 3:  // long pause between bursts
        if (nowms >= buzzerTimer) {
          buzzerState = 0;  // start over
        }
        break;
    }
  } else {
    digitalWrite(greenLed, LOW);
    digitalWrite(buzzer, LOW);
    buzzerState = 0;
    beepCount = 0;
  }

  // ---- White LED: blink by distance (km) ----
  static unsigned long lastBlink = 0;
  static bool ledState = false;
  int blinkMs = -1;
  if (WiFi.status() == WL_CONNECTED && issLon > -190.0 && lastKm >= 0.0f) {
    if (lastKm < 500.0f) blinkMs = 150;
    else if (lastKm < 1000.0f) blinkMs = 300;
    else if (lastKm < 2000.0f) blinkMs = 600;
    else blinkMs = 1000;

    if ((int)(millis() - lastBlink) >= blinkMs) {
      ledState = !ledState;
      digitalWrite(whiteLed, ledState ? HIGH : LOW);
      lastBlink = millis();
    }
  } else {
    digitalWrite(whiteLed, LOW);
  }
}
