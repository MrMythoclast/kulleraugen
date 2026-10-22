#pragma once

#include "display.h"
#include "eyes.h"

// Serial commands, one per line:
//   s      send a snapshot of the sprite (SNAP 240 240, raw RGB565, END)
//   f      switch to the next face
//   m0..m4 force a mood, w0..w3 force the weather, x stop forcing
//   p      toggle holding the face screen
void debugPoll(LGFX_Sprite& s);
void debugFrameDone(uint32_t ms);  // prints "FPS <n>" every 5 s

bool debugForcedMood(Mood& out);
bool debugForcedSky(Sky& out);
bool debugHoldFace();
bool debugTakeNextFace();  // true once per "f"
