// Face 1: classic anime eyes with lashes, three highlights, hatched blush.
#include "face_list.h"
#include "face_parts.h"

namespace {

const Rgb8 LASH = px::hex(0x3d2b57);
const Rgb8 BROW = px::hex(0xc8b0a6);
const Rgb8 LINE = px::hex(0xf3d7c4);

struct Brow {
  float x0, y0, qx, qy, x1, y1;  // left brow; the right one is mirrored
};

// Per mood, in Mood order: Happy, Meh, Grumpy, Cold, Confused.
const Brow BROWS[5] = {
    {48, 56, 66, 47, 84, 54},
    {50, 62, 66, 55, 82, 60},
    {48, 58, 68, 58, 86, 70},
    {50, 62, 66, 54, 84, 54},
    {50, 62, 66, 55, 82, 60},
};

Brow blendBrow(const FaceState& st) {
  Brow b{0, 0, 0, 0, 0, 0};
  for (int i = 0; i < 5; ++i) {
    float k = st.moodMix[i];
    b.x0 += BROWS[i].x0 * k; b.y0 += BROWS[i].y0 * k;
    b.qx += BROWS[i].qx * k; b.qy += BROWS[i].qy * k;
    b.x1 += BROWS[i].x1 * k; b.y1 += BROWS[i].y1 * k;
  }
  return b;
}

}  // namespace

void drawFaceGlanz(LGFX_Sprite& s, const FaceState& st) {
  fillGradient(s, px::hex(0x14112a), px::hex(0x1b1830), px::hex(0x05040a));
  int by = bob(st.ms);
  Mood m = dominantMood(st);

  Brow b = blendBrow(st);
  float lift = st.moodMix[int(Mood::Confused)] * 8;  // one brow up when confused
  drawCurve(s, b.x0, b.y0 + by, b.qx, b.qy + by, b.x1, b.y1 + by, 1.6f, BROW);
  drawCurve(s, 240 - b.x0, b.y0 + by - lift, 240 - b.qx, b.qy + by - lift, 240 - b.x1, b.y1 + by - lift, 1.6f, BROW);

  AnimeEyeSpec e{68, int16_t(110 + by), 30, 37, 24, 31, 11, 15, Highlight::Oval, false, 7, 2, LASH, 0};
  drawAnimeEye(s, e, st, -1);
  e.cx = 172;
  drawAnimeEye(s, e, st, 1);

  int blush = m == Mood::Cold ? 330 : m == Mood::Grumpy ? 170 : 256;
  drawBlush(s, 44, 160 + by, 22, 11, true, blush);
  drawBlush(s, 196, 160 + by, 22, 11, true, blush);

  switch (m) {
    case Mood::Happy:
      drawOpenMouth(s, {120, 163 + by, 30, 19, -3, false});
      break;
    case Mood::Meh:
      drawCurve(s, 111, 170 + by, 120, 173 + by, 129, 170 + by, 1.6f, LINE);
      break;
    case Mood::Grumpy:
      drawCurve(s, 108, 176 + by, 120, 164 + by, 132, 176 + by, 1.8f, LINE);
      break;
    case Mood::Cold:
      drawZigzagMouth(s, 120, 172 + by, 26, st.ms, LINE);
      break;
    case Mood::Confused:
      drawWavyMouth(s, 120, 172 + by, 26, LINE);
      drawQuestion(s, 200, 50, st.ms, px::hex(0x9fa8da));
      break;
  }
}
