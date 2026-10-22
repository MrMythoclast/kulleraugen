#pragma once

#include <stdint.h>

#include "sky.h"

struct Rgb8 {
  uint8_t r, g, b;
};

// Anime-style iris: dark at the top, glowing towards the bottom.
struct IrisPalette {
  Rgb8 top, mid, light, bottom;
};

IrisPalette irisPalette(Sky sky);

// Vertical gradient with stops at 0, 0.45, 0.8 and 1; lut[0] = top, lut[n-1] = bottom.
void buildIrisLut(Sky sky, Rgb8* lut, int n);

// Stored face index, or 0 if it is out of range.
uint8_t clampFace(int32_t stored, uint8_t count);
