#pragma once

#include "display.h"
#include "weather.h"

enum class Mood : uint8_t { Happy, Meh, Grumpy, Cold, Confused };

Mood moodForSky(Sky sky);

// Mood and palette changes blend in smoothly over the next frames.
void eyesSetMood(Mood mood);
void eyesSetPalette(Sky sky);

// Advances blinking/looking and draws both eyes into the sprite.
void eyesDraw(LGFX_Sprite& s, uint32_t ms);
