#include <Arduino.h>
#include <Preferences.h>
#include <WiFi.h>
#include <WiFiManager.h>
#include <boot_button.h>
#include <iris_palette.h>
#include <time.h>

#include "debug_tools.h"
#include "display.h"
#include "eyes.h"
#include "faces/faces.h"
#include "screens.h"
#include "weather.h"

LGFX tft;
LGFX_Sprite spr(&tft);

static const char* AP_NAME = "Kulleraugen";
static const int BOOT_PIN = 9;  // BOOT button on most ESP32-C3 boards
static const uint32_t WEATHER_EVERY_MS = 15UL * 60 * 1000;
static const uint32_t WEATHER_RETRY_MS = 60UL * 1000;

static Preferences prefs;
static float lat = NAN, lon = NAN;
static String city;
static WeatherData weather;
static bool fetchedOnce = false;
static uint32_t nextFetchAt = 0;
static BootButton bootButton;
static uint8_t rotation = 0;  // in 45° steps, 0..7
static uint8_t face = 0;      // see faces.h
static uint32_t cycleStart = 0;  // the screen cycle runs relative to this

// Multiples of 90° are done by the panel itself; the 45° in between by
// rotating the sprite while pushing it. The round display hides the corners.
static void show() {
  if (rotation & 1) {
    spr.pushRotateZoom(&tft, 119.5f, 119.5f, 45, 1, 1);
    return;
  }
  // Only the round part is visible, so skip the corners: about a fifth less to send.
  static uint8_t inset[240];
  static bool haveInset = false;
  if (!haveInset) {
    for (int y = 0; y < 240; ++y) {
      float dy = y - 119.5f;
      inset[y] = uint8_t(fmaxf(0, floorf(119.5f - sqrtf(fmaxf(0, 120 * 120 - dy * dy)))));
    }
    haveInset = true;
  }
  auto* buf = static_cast<const lgfx::swap565_t*>(spr.getBuffer());
  tft.startWrite();
  for (int y = 0; y < 240; ++y) {
    int x = inset[y];
    tft.pushImage(x, y, 240 - 2 * x, 1, buf + y * 240 + x);
  }
  tft.endWrite();
}

static void applyRotation() {
  tft.setRotation(rotation / 2);
  tft.fillScreen(TFT_BLACK);
}

static void rotateDisplay() {
  rotation = (rotation + 1) % 8;
  prefs.begin("kuller", false);
  prefs.putUChar("rot", rotation);
  prefs.end();
  applyRotation();
}

// Switches faces and restarts the screen cycle at the face, already fully faded in.
static void nextFace(uint32_t ms) {
  face = (face + 1) % FACE_COUNT;
  prefs.begin("kuller", false);
  prefs.putUChar("face", face);
  prefs.end();
  cycleStart = ms - 333;
}

static void loadSettings() {
  prefs.begin("kuller", true);
  city = prefs.getString("city", "");
  lat = prefs.getFloat("lat", NAN);
  lon = prefs.getFloat("lon", NAN);
  rotation = prefs.getUChar("rot", 0) % 8;
  face = clampFace(prefs.getUChar("face", 0), FACE_COUNT);
  prefs.end();
}

static void saveLocation(const String& name, float la, float lo) {
  prefs.begin("kuller", false);
  prefs.putString("city", name);
  prefs.putFloat("lat", la);
  prefs.putFloat("lon", lo);
  prefs.end();
  city = name;
  lat = la;
  lon = lo;
}

static void clearSettings() {
  prefs.begin("kuller", false);
  prefs.clear();
  prefs.end();
  WiFiManager wm;
  wm.resetSettings();
}

// Blocks until WiFi works and the entered place was found.
static void runSetupPortal() {
  for (;;) {
    WiFiManager wm;
    WiFiManagerParameter cityParam("city", "Wohnort (PLZ, Stadt oder Adresse)", city.c_str(), 64);
    wm.addParameter(&cityParam);
    std::vector<const char*> menu = {"wifi"};
    wm.setMenu(menu);
    wm.setTitle("Kulleraugen");
    wm.setClass("invert");

    drawSetupScreen(spr, AP_NAME);
    show();
    if (!wm.startConfigPortal(AP_NAME)) ESP.restart();

    String entered = cityParam.getValue();
    entered.trim();
    drawMessage(spr, "Suche Ort ...", entered);
    show();

    float la, lo;
    String found;
    if (geocodeCity(entered, la, lo, found)) {
      saveLocation(found, la, lo);
      drawMessage(spr, "Gefunden!", found);
      show();
      delay(2000);
      return;
    }
    drawMessage(spr, "Ort nicht gefunden", entered);
    show();
    delay(4000);
  }
}

static void applyUtcOffset(int seconds) {
  // POSIX TZ strings count the other way round: UTC+2 is "UTC-2".
  int a = abs(seconds);
  char tz[20];
  snprintf(tz, sizeof tz, "UTC%c%d:%02d", seconds >= 0 ? '-' : '+', a / 3600, a % 3600 / 60);
  setenv("TZ", tz, 1);
  tzset();
}

static void updateWeather(uint32_t ms) {
  if (WiFi.status() != WL_CONNECTED) return;
  if (fetchedOnce && int32_t(ms - nextFetchAt) < 0) return;
  WeatherData fresh;
  bool ok = fetchWeather(lat, lon, fresh);
  fetchedOnce = true;
  nextFetchAt = millis() + (ok ? WEATHER_EVERY_MS : WEATHER_RETRY_MS);
  if (ok) {
    weather = fresh;
    applyUtcOffset(weather.utcOffset);
  }
}

static void checkButton(uint32_t ms) {
  ButtonEvent ev = bootButton.update(digitalRead(BOOT_PIN) == LOW, ms);
  if (ev == ButtonEvent::Rotate) rotateDisplay();
  if (ev == ButtonEvent::DoubleClick || debugTakeNextFace()) nextFace(ms);
  if (ev != ButtonEvent::Reset) return;
  drawMessage(spr, "Reset", "Setup startet neu ...");
  show();
  clearSettings();
  delay(1500);
  ESP.restart();
}

enum class Screen { Face, Time, Forecast };

// Face for 8s, then time and forecast for 3.5s each, with short fades between.
static Screen pickScreen(uint32_t ms, bool haveTime, bool haveWeather, int& alpha) {
  struct Seg {
    Screen screen;
    uint32_t len;
  } segs[3];
  int n = 0;
  segs[n++] = {Screen::Face, 8000};
  if (haveTime) segs[n++] = {Screen::Time, 3500};
  if (haveWeather) segs[n++] = {Screen::Forecast, 3500};
  alpha = 256;
  if (n == 1) return Screen::Face;

  uint32_t total = 0;
  for (int i = 0; i < n; ++i) total += segs[i].len;
  uint32_t p = ms % total;
  for (int i = 0; i < n; ++i) {
    if (p < segs[i].len) {
      uint32_t edge = min(p, segs[i].len - p);
      if (edge < 333) alpha = edge * 256 / 333;
      return segs[i].screen;
    }
    p -= segs[i].len;
  }
  return Screen::Face;
}

void setup() {
  Serial.begin(115200);
  pinMode(BOOT_PIN, INPUT_PULLUP);
  randomSeed(esp_random());

  tft.init();
  tft.fillScreen(TFT_BLACK);
  spr.setColorDepth(16);
  // Grab the frame buffer before WiFi takes its share of the heap.
  if (!spr.createSprite(240, 240)) {
    Serial.println("Not enough RAM for the frame buffer");
    for (;;) delay(1000);
  }
  spr.setPivot(119.5f, 119.5f);

  loadSettings();
  applyRotation();
  if (isnan(lat) || isnan(lon)) {
    runSetupPortal();
  } else {
    WiFi.mode(WIFI_STA);
    WiFi.begin();  // credentials saved by the setup portal
  }
  WiFi.setAutoReconnect(true);
  Serial.printf("Ort: %s (%.4f, %.4f)\n", city.c_str(), lat, lon);

  configTime(0, 0, "pool.ntp.org", "time.google.com");
  setenv("TZ", "CET-1CEST,M3.5.0,M10.5.0/3", 1);  // until the weather reply brings the real offset
  tzset();
}

void loop() {
  uint32_t ms = millis();
  checkButton(ms);
  updateWeather(ms);

  bool online = WiFi.status() == WL_CONNECTED;
  Mood mood = !online ? Mood::Confused : weather.valid ? moodForSky(weather.sky) : Mood::Meh;
  Sky sky = weather.valid ? weather.sky : Sky::Cloud;
  debugForcedMood(mood);
  debugForcedSky(sky);
  facesSetMood(mood);
  facesSetPalette(sky);

  // Not getLocalTime(&now, 0): it fails whenever millis() ticks during the
  // call, which reshuffles the screen cycle for a frame.
  tm now;
  time_t t = time(nullptr);
  localtime_r(&t, &now);
  bool haveTime = now.tm_year > 120;
  int alpha;
  Screen screen = pickScreen(ms - cycleStart, haveTime, weather.valid, alpha);
  if (debugHoldFace()) {
    screen = Screen::Face;
    alpha = 256;
  }

  spr.fillScreen(TFT_BLACK);
  if (screen == Screen::Face) {
    facesDraw(spr, ms, face);
  } else if (screen == Screen::Time) {
    drawTimeScreen(spr, ms, now, weather);
  } else {
    drawForecastScreen(spr, weather, city);
  }
  fadeSprite(spr, alpha);

  uint32_t heldMs;
  if (bootButton.held(ms, heldMs) && heldMs >= ROTATE_MAX_MS) {
    drawResetRing(spr, float(heldMs) / RESET_HOLD_MS);
  }
  debugPoll(spr);
  show();
  debugFrameDone(ms);
}
