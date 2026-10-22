#pragma once

#include <sky.h>

#include "../display.h"
#include "../eyes.h"

// 0 Kulleraugen, 1 Anime-Glanzaugen, 2 Chibi-Kätzchen, 3 Sternenglitzer, 4 Pastell-Träumerchen
constexpr uint8_t FACE_COUNT = 5;

void facesSetMood(Mood m);
void facesSetPalette(Sky sky);
void facesDraw(LGFX_Sprite& s, uint32_t ms, uint8_t face);
