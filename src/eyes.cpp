#include "eyes.h"

#include <math.h>

// The ESP32-C3 has no FPU, so the per-pixel work below is all integer math.
// Floats are only used for a handful of per-frame values.

namespace {

struct Rgb {
  uint8_t r, g, b;
};

struct LidParams {
  float top;     // how far the upper lid is closed, 0..1
  float tilt;    // pixels the inner corner drops (grumpy)
  float bottom;  // lower lid pushed up into a smile, 0..1
  float shiver;  // horizontal trembling, 0..1
  float asym;    // extra closing on the right eye (confused)
};

constexpr int CX = 120, CY = 114;
constexpr int EYE_DX = 50;
constexpr int RX = 30, RY = 40;  // sclera half-size
constexpr int IRIS_R = 25;
constexpr int LUT_MAX = 34 * 34;  // farthest squared distance from the gradient center
constexpr Rgb SCLERA{244, 246, 251};
constexpr Rgb PUPIL{5, 7, 13};
constexpr Rgb IRIS_EDGE{11, 29, 51};
constexpr Rgb LID_SHADOW{20, 30, 50};

// Ellipse tests compare against 1024 using precomputed reciprocals (<<16).
constexpr uint32_t INV_RX2 = (1024u << 16) / (RX * RX);
constexpr uint32_t INV_RY2 = (1024u << 16) / (RY * RY);
constexpr uint32_t INV_BX2 = (1024u << 16) / (54 * 54);  // smiling lower lid
constexpr uint32_t INV_BY2 = (1024u << 16) / (42 * 42);
constexpr uint32_t INV_H1X = (1024u << 16) / (7 * 7);  // big highlight
constexpr uint32_t INV_H1Y = (1024u << 16) / (9 * 9);

Rgb lut[LUT_MAX + 1];
bool haveLut = false;
Sky lutSky = Sky::Cloud;

Mood mood = Mood::Meh;
LidParams cur{0.38f, 0, 0, 0, 0};

float lookX = 0, lookY = 0, targetX = 0, targetY = 0;
uint32_t nextLook = 0, nextBlink = 2000, blinkStart = 0, lastMs = 0;
bool blinkedOnce = false;

LidParams paramsFor(Mood m) {
  switch (m) {
    case Mood::Happy:    return {0.00f, 0, 1, 0, 0};
    case Mood::Meh:      return {0.38f, 0, 0, 0, 0};
    case Mood::Grumpy:   return {0.20f, 24, 0, 0, 0};
    case Mood::Cold:     return {0.28f, 0, 0, 1, 0};
    case Mood::Confused: return {0.12f, 0, 0, 0, 0.3f};
  }
  return {0.38f, 0, 0, 0, 0};
}

Rgb mix(Rgb a, Rgb b, float f) {
  return {uint8_t(a.r + (b.r - a.r) * f), uint8_t(a.g + (b.g - a.g) * f),
          uint8_t(a.b + (b.b - a.b) * f)};
}

void buildLut(Sky sky) {
  Rgb light, dark;
  switch (sky) {
    case Sky::Sun:   light = {255, 243, 196}; dark = {255, 179, 0};   break;
    case Sky::Cloud: light = {236, 239, 241}; dark = {120, 144, 156}; break;
    case Sky::Rain:  light = {225, 245, 254}; dark = {2, 136, 209};   break;
    case Sky::Snow:  light = {255, 255, 255}; dark = {128, 222, 234}; break;
  }
  for (int g2 = 0; g2 <= LUT_MAX; ++g2) {
    float t = sqrtf(float(g2)) / 26.f;
    if (t > 1) t = 1;
    lut[g2] = t < 0.55f ? mix(light, dark, t / 0.55f) : mix(dark, IRIS_EDGE, (t - 0.55f) / 0.45f);
  }
  lutSky = sky;
  haveLut = true;
}

inline void blend(int& r, int& g, int& b, Rgb c, int k) {  // k: 0..256 towards c
  r = (r * (256 - k) + c.r * k) >> 8;
  g = (g * (256 - k) + c.g * k) >> 8;
  b = (b * (256 - k) + c.b * k) >> 8;
}

void drawEye(LGFX_Sprite& s, int ex, int ey, int side, float blink, int ix, int iy, int pupil2) {
  // Upper lid: a straight edge per column; the inner corner drops for "grumpy".
  float cover = cur.top + (side > 0 ? cur.asym : 0);
  if (cover > 1) cover = 1;
  if (blink > cover) cover = blink;
  float outer = -42 + cover * 84;
  float inner = outer + cur.tilt * (1 - blink);
  float yl = side < 0 ? outer : inner;
  float yr = side < 0 ? inner : outer;
  int lidY[2 * RX + 1];
  for (int c = 0; c <= 2 * RX; ++c) {
    float ly = yl + (yr - yl) * (c - RX + 65) / 130.f;
    lidY[c] = int(ly * 256);
  }
  // Smiling lower lid: an ellipse that rises from below as "bottom" goes to 1.
  int by = 58 + int((1 - cur.bottom) * 70);

  for (int dy = -RY; dy <= RY; ++dy) {
    for (int dx = -RX; dx <= RX; ++dx) {
      uint32_t e2 = (uint32_t(dx * dx) * INV_RX2 + uint32_t(dy * dy) * INV_RY2) >> 16;
      if (e2 >= 1024) continue;
      int a = 256;
      if (e2 > 1024 - 68) a = int(1024 - e2) * 256 / 68;  // ~1px soft edge

      int dist = dy * 256 - lidY[dx + RX];
      if (dist <= 0) continue;
      if (dist < 256) a = a * dist >> 8;

      int bdy = dy - by;
      uint32_t hb = (uint32_t(dx * dx) * INV_BX2 + uint32_t(bdy * bdy) * INV_BY2) >> 16;
      if (hb < 1024) continue;
      if (hb < 1024 + 70) a = a * int(hb - 1024) / 70;
      if (a <= 0) continue;

      int r = SCLERA.r, g = SCLERA.g, b = SCLERA.b;
      int qx = dx - ix, qy = dy - iy;
      int r2 = qx * qx + qy * qy;
      if (r2 < IRIS_R * IRIS_R) {
        if (r2 < pupil2) {
          r = PUPIL.r; g = PUPIL.g; b = PUPIL.b;
        } else {
          int g2 = qx * qx + (qy - 8) * (qy - 8);
          if (g2 > LUT_MAX) g2 = LUT_MAX;
          r = lut[g2].r; g = lut[g2].g; b = lut[g2].b;
        }
        int h1x = qx + 9, h1y = qy + 10;
        int h2x = qx - 8, h2y = qy - 8;
        if (abs(h1x) < 7 && abs(h1y) < 9 &&
            ((uint32_t(h1x * h1x) * INV_H1X + uint32_t(h1y * h1y) * INV_H1Y) >> 16) < 1024) {
          r = g = b = 255;
        } else if (h2x * h2x + h2y * h2y < 13) {
          r = g = b = 255;
        }
        if (r2 > (IRIS_R - 1) * (IRIS_R - 1)) {  // soften the iris rim
          int k = 256 - (IRIS_R * IRIS_R - r2) * 256 / (2 * IRIS_R - 1);
          blend(r, g, b, SCLERA, k);
        }
      }
      if (dist < 22 * 256) {  // soft shadow just under the upper lid
        int k = ((22 * 256 - dist) / 22) * 115 >> 8;
        blend(r, g, b, LID_SHADOW, k);
      }
      r = r * a >> 8; g = g * a >> 8; b = b * a >> 8;
      s.drawPixel(ex + dx, ey + dy, lgfx::color565(r, g, b));
    }
  }
}

}  // namespace

Mood moodForSky(Sky sky) {
  switch (sky) {
    case Sky::Sun:   return Mood::Happy;
    case Sky::Cloud: return Mood::Meh;
    case Sky::Rain:  return Mood::Grumpy;
    case Sky::Snow:  return Mood::Cold;
  }
  return Mood::Meh;
}

void eyesSetMood(Mood m) { mood = m; }

void eyesSetPalette(Sky sky) {
  if (!haveLut || sky != lutSky) buildLut(sky);
}

void eyesDraw(LGFX_Sprite& s, uint32_t ms) {
  if (!haveLut) buildLut(lutSky);
  float dt = (ms - lastMs) / 1000.f;
  if (dt > 0.1f) dt = 0.1f;
  lastMs = ms;

  LidParams target = paramsFor(mood);
  float k = dt * 6 > 1 ? 1 : dt * 6;
  cur.top += (target.top - cur.top) * k;
  cur.tilt += (target.tilt - cur.tilt) * k;
  cur.bottom += (target.bottom - cur.bottom) * k;
  cur.shiver += (target.shiver - cur.shiver) * k;
  cur.asym += (target.asym - cur.asym) * k;

  // Quick glances to random spots, more restless when confused.
  if (int32_t(ms - nextLook) >= 0) {
    targetX = random(-100, 101) / 100.f;
    targetY = random(-70, 71) / 100.f;
    nextLook = ms + (mood == Mood::Confused ? random(300, 900) : random(800, 3000));
  }
  float lk = dt * 12 > 1 ? 1 : dt * 12;
  lookX += (targetX - lookX) * lk;
  lookY += (targetY - lookY) * lk;

  if (int32_t(ms - nextBlink) >= 0) {
    blinkStart = ms;
    blinkedOnce = true;
    nextBlink = ms + (random(5) == 0 ? 260 : random(2500, 6000));  // sometimes a double blink
  }
  float blink = 0;
  uint32_t bt = ms - blinkStart;
  if (blinkedOnce && bt < 180) blink = sinf(bt * PI / 180);

  int shiver = lroundf(sinf(ms / 35.f) * 1.5f * cur.shiver);
  int ix = lroundf(lookX * 9);
  int iy = lroundf(lookY * 8) + 4;
  float pr = 10 + sinf(ms / 1300.f) * 1.5f;
  int pupil2 = int(pr * pr);

  drawEye(s, CX - EYE_DX + shiver, CY, -1, blink, ix, iy, pupil2);
  drawEye(s, CX + EYE_DX + shiver, CY, 1, blink, ix, iy, pupil2);

  if (mood == Mood::Confused) {
    s.setFont(&fonts::FreeSansBold18pt7b);
    s.setTextDatum(textdatum_t::middle_center);
    s.setTextColor(lgfx::color565(130, 150, 180));
    s.drawString("?", 196, 52 + lroundf(sinf(ms / 400.f) * 3));
  }
}
