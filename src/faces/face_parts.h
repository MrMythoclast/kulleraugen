#pragma once

#include "anime_eye.h"

// Mouths, brows and other bits shared by the anime faces.

// Quadratic curve through (x0,y0) and (x1,y1), pulled towards (qx,qy); r = half line width.
void drawCurve(LGFX_Sprite& s, float x0, float y0, float qx, float qy, float x1, float y1, float r, Rgb8 col);

// Open mouth: gently curved top edge, rounded bottom. w = width, h = depth below the top edge.
struct OpenMouth {
  int cx, top, w, h;
  int topCurve;  // top edge dips by this many px at the sides (negative = smile corners up)
  bool teeth;
};
void drawOpenMouth(LGFX_Sprite& s, const OpenMouth& m);

void drawZigzagMouth(LGFX_Sprite& s, int cx, int cy, int w, uint32_t ms, Rgb8 col);  // chattering teeth
void drawWavyMouth(LGFX_Sprite& s, int cx, int cy, int w, Rgb8 col);
void drawQuestion(LGFX_Sprite& s, int x, int y, uint32_t ms, Rgb8 col);

// The mood with the largest share, for parts that switch instead of blending.
Mood dominantMood(const FaceState& st);

// Gentle up-and-down bobbing of the whole face, in px.
int bob(uint32_t ms);
