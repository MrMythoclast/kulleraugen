#pragma once

#include <Arduino.h>
#include <sky.h>

struct Forecast {
  int hour;
  Sky sky;
  int temp;
};

struct WeatherData {
  bool valid = false;
  Sky sky = Sky::Cloud;
  int temp = 0;
  Forecast next[4];   // +3h, +6h, +9h, +12h
  int utcOffset = 0;  // seconds, for the configured location
};

Sky skyFromCode(int wmoCode);
bool geocodeCity(const String& query, float& lat, float& lon, String& found);
bool fetchWeather(float lat, float lon, WeatherData& out);
