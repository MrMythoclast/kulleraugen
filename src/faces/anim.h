#pragma once

#include <sky.h>

#include "../eyes.h"

// Animation state shared by the anime faces: blended mood, gaze and blinking.
struct FaceState {
  Mood mood;
  Sky sky;
  uint32_t ms;
  float dt;
  float top, tilt, bottom, shiver, asym;  // blended lid values, same meaning as in eyes.cpp
  float lookX, lookY;                     // -1..1
  float blink;                            // 0..1
  float moodMix[5];                       // share of each mood, sums to 1, blends smoothly
};

void animUpdate(uint32_t ms, Mood mood, Sky sky);
const FaceState& animState();
