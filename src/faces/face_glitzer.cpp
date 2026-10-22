// Face 3: sparkly eyes with star and heart highlights, twinkling stars and floating hearts.
#include <math.h>

#include "face_list.h"
#include "face_parts.h"

namespace {

const Rgb8 LASH = px::hex(0x4a2f66);
const Rgb8 SPARK = px::hex(0xffe7ff);
const Rgb8 LINE = px::hex(0xf3d7c4);

struct Twinkle {
  int16_t x, y;
  uint16_t delay;  // ms
};

// The first four are always there when happy or bored; the rest only when happy.
const Twinkle STARS[] = {{36, 50, 0}, {204, 44, 700}, {214, 190, 1300}, {22, 196, 400},
                         {120, 214, 1000}, {64, 20, 1800}};

float twinkle(uint32_t ms, uint16_t delay) {
  float t = ((ms + delay) % 2200) / 2200.f;
  return 0.4f + 0.6f * (0.5f - 0.5f * cosf(t * 2 * PI));
}

void snowflake(LGFX_Sprite& s, float x, float y, float r) {
  for (int i = 0; i < 3; ++i) {
    float a = i * PI / 3;
    float pts[4] = {x - cosf(a) * r, y - sinf(a) * r, x + cosf(a) * r, y + sinf(a) * r};
    strokePolyline(s, pts, 2, 0.8f, {255, 255, 255});
  }
}

}  // namespace

void drawFaceGlitzer(LGFX_Sprite& s, const FaceState& st) {
  fillGradient(s, px::hex(0x1a0b26), px::hex(0x241030), px::hex(0x050208));
  int by = bob(st.ms);
  Mood m = dominantMood(st);
  float fl = sinf(st.ms * 2 * PI / 4000) * 3.5f;  // floating up and down

  if (m == Mood::Cold) {
    // Snowflakes drift down instead of twinkling stars.
    for (int i = 0; i < 6; ++i) {
      float y = fmodf(st.ms / 40.f + i * 47, 260) - 10;
      float x = STARS[i].x + sinf(st.ms / 700.f + i) * 6;
      snowflake(s, x, y, 4.5f);
    }
  } else if (m != Mood::Grumpy) {
    int n = m == Mood::Happy ? 6 : m == Mood::Meh ? 2 : 4;
    for (int i = 0; i < n; ++i)
      drawSparkle(s, STARS[i].x, STARS[i].y, lroundf(9 * twinkle(st.ms, STARS[i].delay)), SPARK);
  }

  if (m == Mood::Confused) {
    drawQuestion(s, 120, 30 + lroundf(fl), st.ms, px::hex(0xc58bff));
  } else if (m == Mood::Happy || m == Mood::Meh) {
    drawHeart(s, 120, 30 + lroundf(fl), 7, px::hex(0xff7aa8));
    if (m == Mood::Happy) drawHeart(s, 200, 104 - lroundf(fl), 5, px::hex(0xc58bff));
  }

  Highlight hl = m == Mood::Grumpy ? Highlight::Oval : Highlight::Star;
  AnimeEyeSpec e{68, int16_t(110 + by), 32, 36, 26, 30, 9, 12, hl, true, 6, 2, LASH, 0};
  drawAnimeEye(s, e, st, -1);
  e.cx = 172;
  drawAnimeEye(s, e, st, 1);

  int blush = m == Mood::Cold ? 330 : m == Mood::Grumpy ? 170 : 256;
  drawBlush(s, 46, 160 + by, 20, 10, false, blush);
  drawBlush(s, 194, 160 + by, 20, 10, false, blush);

  switch (m) {
    case Mood::Happy:
      drawOpenMouth(s, {120, 164 + by, 36, 25, -1, true});
      break;
    case Mood::Meh:
      drawOpenMouth(s, {120, 167 + by, 20, 11, 1, false});
      break;
    case Mood::Grumpy:
      drawCurve(s, 106, 178 + by, 120, 164 + by, 134, 178 + by, 1.8f, LINE);
      break;
    case Mood::Cold:
      drawZigzagMouth(s, 120, 174 + by, 28, st.ms, LINE);
      break;
    case Mood::Confused:
      drawWavyMouth(s, 120, 174 + by, 28, LINE);
      break;
  }
}
