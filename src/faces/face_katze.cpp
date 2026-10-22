// Face 2: chibi kitten with round eyes, twitching ears, whiskers and an ω mouth.
#include <math.h>

#include "face_list.h"
#include "face_parts.h"

namespace {

const Rgb8 EAR_OUT = px::hex(0xf3e3d6);
const Rgb8 EAR_IN = px::hex(0xffb3c7);
const Rgb8 LASH = px::hex(0x3d2f50);
const Rgb8 WHISKER = px::hex(0x8c837c);
const Rgb8 LINE = px::hex(0xf3e3d6);
const Rgb8 NOSE = px::hex(0xff8fab);

// Filled triangle with soft edges: hard fill plus a thin stroke of the same colour.
void softTriangle(LGFX_Sprite& s, const float* p, Rgb8 col) {
  s.fillTriangle(lroundf(p[0]), lroundf(p[1]), lroundf(p[2]), lroundf(p[3]), lroundf(p[4]), lroundf(p[5]),
                 lgfx::color565(col.r, col.g, col.b));
  float loop[8] = {p[0], p[1], p[2], p[3], p[4], p[5], p[0], p[1]};
  strokePolyline(s, loop, 4, 0.7f, col);
}

// Left ear, rotated by `angle` (radians, positive = tips outwards) around its base;
// side = +1 mirrors it to the right.
void drawEar(LGFX_Sprite& s, float angle, float drop, int side) {
  const float outer[6] = {24, 74, 42, 6, 96, 44};
  const float inner[6] = {38, 62, 46, 24, 80, 46};
  const float px0 = 60, py0 = 59;
  float c = cosf(-angle), sn = sinf(-angle);
  auto place = [&](const float* src, float* dst) {
    for (int i = 0; i < 3; ++i) {
      float x = src[2 * i] - px0, y = src[2 * i + 1] - py0;
      float rx = x * c - y * sn + px0, ry = x * sn + y * c + py0 + drop;
      dst[2 * i] = side < 0 ? rx : 240 - rx;
      dst[2 * i + 1] = ry;
    }
  };
  float o[6], in[6];
  place(outer, o);
  place(inner, in);
  softTriangle(s, o, EAR_OUT);
  softTriangle(s, in, EAR_IN);
}

// Short ear twitch every few seconds; the right ear lags behind the left.
float twitch(uint32_t ms, uint32_t offset) {
  uint32_t t = (ms + offset) % 3800;
  if (t > 300) return 0;
  return sinf(t * PI / 150) * 0.16f * (1 - t / 300.f);
}

}  // namespace

void drawFaceKatze(LGFX_Sprite& s, const FaceState& st) {
  fillGradient(s, px::hex(0x0b0912), px::hex(0x07060c), px::hex(0x040308));
  int by = bob(st.ms);
  Mood m = dominantMood(st);
  const float* mix = st.moodMix;

  // Ears: up when happy, a bit sideways when bored, flat back when grumpy, trembling when cold.
  float angle = mix[int(Mood::Meh)] * 0.18f + mix[int(Mood::Grumpy)] * 1.0f + mix[int(Mood::Confused)] * 0.08f;
  float drop = mix[int(Mood::Grumpy)] * 16;
  float shake = mix[int(Mood::Cold)] * sinf(st.ms / 40.f) * 0.05f;
  float conf = mix[int(Mood::Confused)] * sinf(st.ms / 160.f) * 0.15f;
  drawEar(s, angle + shake + twitch(st.ms, 0), drop + by, -1);
  drawEar(s, angle - shake + twitch(st.ms, 1700) + conf, drop + by, 1);

  AnimeEyeSpec e{70, int16_t(114 + by), 34, 34, 28, 28, 14, 14, Highlight::Oval, false, 5, 0, LASH, 0};
  drawAnimeEye(s, e, st, -1);
  e.cx = 170;
  drawAnimeEye(s, e, st, 1);

  // Whisker marks on the cheeks.
  const float wl[3][4] = {{14, 150, 34, 154}, {14, 162, 34, 162}, {16, 174, 34, 168}};
  for (auto& w : wl) {
    float l[4] = {w[0], w[1] + by, w[2], w[3] + by};
    float r[4] = {240 - w[0], w[1] + by, 240 - w[2], w[3] + by};
    strokePolyline(s, l, 2, 1.0f, WHISKER);
    strokePolyline(s, r, 2, 1.0f, WHISKER);
  }

  int blush = m == Mood::Cold ? 330 : m == Mood::Grumpy ? 170 : 256;
  drawBlush(s, 50, 162 + by, 18, 9, false, blush);
  drawBlush(s, 190, 162 + by, 18, 9, false, blush);

  float nose[6] = {115, 156.f + by, 125, 156.f + by, 120, 162.f + by};
  softTriangle(s, nose, NOSE);

  float y = 166 + by;
  switch (m) {
    case Mood::Happy: {
      drawCurve(s, 98, y, 109, y + 15, 120, y, 1.9f, LINE);
      drawCurve(s, 120, y, 131, y + 15, 142, y, 1.9f, LINE);
      float fang[6] = {127, y + 4, 133, y + 4, 130, y + 11};
      softTriangle(s, fang, {255, 255, 255});
      break;
    }
    case Mood::Meh:
      drawCurve(s, 104, y + 1, 112, y + 8, 120, y + 1, 1.7f, LINE);
      drawCurve(s, 120, y + 1, 128, y + 8, 136, y + 1, 1.7f, LINE);
      break;
    case Mood::Grumpy:
      drawCurve(s, 104, y + 10, 112, y + 1, 120, y + 7, 1.8f, LINE);
      drawCurve(s, 120, y + 7, 128, y + 1, 136, y + 10, 1.8f, LINE);
      break;
    case Mood::Cold:
      drawZigzagMouth(s, 120, y + 6, 26, st.ms, LINE);
      break;
    case Mood::Confused:
      drawWavyMouth(s, 120, y + 6, 26, LINE);
      drawQuestion(s, 206, 96, st.ms, px::hex(0x9fa8da));
      break;
  }
}
