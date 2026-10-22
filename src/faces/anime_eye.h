#pragma once

#include <iris_palette.h>

#include "../display.h"
#include "anim.h"

// Direct access to the sprite's RGB565 buffer (stored big-endian), so shapes can
// be blended over any background with soft edges.
namespace px {

constexpr int W = 240, H = 240;

inline uint16_t* buf(LGFX_Sprite& s) { return static_cast<uint16_t*>(s.getBuffer()); }

inline Rgb8 get(const uint16_t* b, int x, int y) {
  uint16_t v = b[y * W + x];
  v = uint16_t((v >> 8) | (v << 8));
  return {uint8_t((v >> 11) << 3 | (v >> 13)), uint8_t(((v >> 5) & 63) << 2 | ((v >> 9) & 3)),
          uint8_t((v & 31) << 3 | ((v >> 2) & 7))};
}

inline void put(uint16_t* b, int x, int y, int r, int g, int bl) {
  uint16_t v = uint16_t((r >> 3) << 11 | (g >> 2) << 5 | (bl >> 3));
  b[y * W + x] = uint16_t((v >> 8) | (v << 8));
}

// a: 0 = keep, 256 = c
inline void blend(uint16_t* b, int x, int y, Rgb8 c, int a) {
  if (a <= 0 || unsigned(x) >= W || unsigned(y) >= H) return;
  if (a >= 256) {
    put(b, x, y, c.r, c.g, c.b);
    return;
  }
  Rgb8 o = get(b, x, y);
  put(b, x, y, o.r + ((c.r - o.r) * a >> 8), o.g + ((c.g - o.g) * a >> 8), o.b + ((c.b - o.b) * a >> 8));
}

inline Rgb8 mix(Rgb8 a, Rgb8 b, int k) {  // k: 0..256 towards b
  return {uint8_t(a.r + ((b.r - a.r) * k >> 8)), uint8_t(a.g + ((b.g - a.g) * k >> 8)),
          uint8_t(a.b + ((b.b - a.b) * k >> 8))};
}

inline Rgb8 scale(Rgb8 c, int k) { return {uint8_t(c.r * k >> 8), uint8_t(c.g * k >> 8), uint8_t(c.b * k >> 8)}; }

constexpr Rgb8 hex(uint32_t v) { return {uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)}; }

}  // namespace px

enum class Highlight : uint8_t { Oval, Star, Heart };

struct AnimeEyeSpec {
  int16_t cx, cy;  // eye centre
  int16_t rx, ry;  // white of the eye, half axes
  int16_t irisRx, irisRy, pupilRx, pupilRy;
  Highlight main;     // big highlight top left; Star also gets a heart bottom right
  bool innerRing;     // light ring inside the iris
  uint8_t lashWidth;  // upper lash line, 0 = none
  uint8_t lashFlicks; // lash tips at the outer corner
  Rgb8 lashColor;
  float heavyLid;     // extra closing, 0..1
};

// side: -1 left eye, +1 right eye (the inner corner faces the nose)
void drawAnimeEye(LGFX_Sprite& s, const AnimeEyeSpec& e, const FaceState& st, int side);
// Anti-aliased line through n points (xy = x0,y0,x1,y1,...); r = half line width.
void strokePolyline(LGFX_Sprite& s, const float* xy, int n, float r, Rgb8 col);
void drawBlush(LGFX_Sprite& s, int cx, int cy, int rx, int ry, bool hatching, int strength = 256);
void drawSparkle(LGFX_Sprite& s, int cx, int cy, int r, Rgb8 col);  // four-pointed star
void drawHeart(LGFX_Sprite& s, int cx, int cy, int size, Rgb8 col);
void fillGradient(LGFX_Sprite& s, Rgb8 top, Rgb8 mid, Rgb8 bottom);  // vertical, mid at half height
