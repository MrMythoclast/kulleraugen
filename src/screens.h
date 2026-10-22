#pragma once

#include <time.h>

#include "display.h"
#include "weather.h"

void drawTimeScreen(LGFX_Sprite& s, uint32_t ms, const tm& now, const WeatherData& w);
void drawForecastScreen(LGFX_Sprite& s, const WeatherData& w, const String& place);
void drawSetupScreen(LGFX_Sprite& s, const char* apName);
void drawMessage(LGFX_Sprite& s, const char* title, const String& detail);
void drawResetRing(LGFX_Sprite& s, float progress);

// Darkens the whole sprite; alpha 0 = black, 256 = unchanged.
void fadeSprite(LGFX_Sprite& s, int alpha);
