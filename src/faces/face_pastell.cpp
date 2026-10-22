// Face 4: sleepy dreamer on a light pastel background, with sweat drop and floating "z".
#include <math.h>

#include "face_list.h"
#include "face_parts.h"

namespace {

const Rgb8 BG_TOP = px::hex(0xffd9ec);
const Rgb8 BG_MID = px::hex(0xe9d8ff);
const Rgb8 BG_LOW = px::hex(0xcfe9ff);
const Rgb8 INK = px::hex(0x6b3a5c);
const Rgb8 DROP = px::hex(0x9ad7ff);
const Rgb8 DROP_LINE = px::hex(0x5aa8e0);

Rgb8 bgAt(int y) { return y < 120 ? px::mix(BG_TOP, BG_MID, y * 256 / 120) : px::mix(BG_MID, BG_LOW, (y - 120) * 256 / 120); }

void bubble(LGFX_Sprite& s, int cx, int cy, int r) {
  uint16_t* b = px::buf(s);
  int r2 = r * r;
  for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
      int d2 = dx * dx + dy * dy;
      if (d2 >= r2) continue;
      int edge = (r2 - d2) * 256 / (2 * r);  // ~1px soft rim
      px::blend(b, cx + dx, cy + dy, {255, 255, 255}, (edge > 256 ? 256 : edge) * 90 >> 8);
    }
}

void sweatDrop(LGFX_Sprite& s, float x, float y) {
  s.fillTriangle(lroundf(x - 5.6f), lroundf(y + 11), lroundf(x + 5.6f), lroundf(y + 11), lroundf(x), lroundf(y),
                 lgfx::color565(DROP.r, DROP.g, DROP.b));
  s.fillSmoothCircle(lroundf(x), lroundf(y + 13), 6, lgfx::color565(DROP.r, DROP.g, DROP.b));
  float l[6] = {x - 5.6f, y + 11, x, y, x + 5.6f, y + 11};
  strokePolyline(s, l, 3, 0.6f, DROP_LINE);
  s.fillSmoothCircle(lroundf(x - 2), lroundf(y + 12), 1, TFT_WHITE);
}

// A "z" that rises to the upper right and fades out, repeating every 3.5 s.
void floatingZ(LGFX_Sprite& s, uint32_t ms, uint32_t delay, int x, int y, const lgfx::IFont* font) {
  float t = ((ms + delay) % 3500) / 3500.f;
  float a = t < 0.3f ? t / 0.3f : 1 - (t - 0.3f) / 0.7f;
  int zx = x + lroundf(t * 14), zy = y - lroundf(t * 26);
  Rgb8 c = px::mix(bgAt(zy), INK, int(a * 256));
  s.setFont(font);
  s.setTextDatum(textdatum_t::middle_center);
  s.setTextColor(lgfx::color565(c.r, c.g, c.b));
  s.drawString("z", zx, zy);
}

}  // namespace

void drawFacePastell(LGFX_Sprite& s, const FaceState& st) {
  fillGradient(s, BG_TOP, BG_MID, BG_LOW);
  bubble(s, 40, 60, 14);
  bubble(s, 206, 78, 9);
  bubble(s, 196, 196, 16);

  int by = bob(st.ms);
  Mood m = dominantMood(st);
  const float* mix = st.moodMix;

  // Sleepy heavy lids, least when happy.
  float heavy = mix[int(Mood::Meh)] * 0.2f + mix[int(Mood::Grumpy)] * 0.12f + mix[int(Mood::Cold)] * 0.1f +
                mix[int(Mood::Confused)] * 0.05f;
  AnimeEyeSpec e{68, int16_t(112 + by), 28, 32, 22, 27, 10, 13, Highlight::Oval, false, 5, 2, INK, heavy};
  drawAnimeEye(s, e, st, -1);
  e.cx = 172;
  drawAnimeEye(s, e, st, 1);

  drawBlush(s, 48, 162 + by, 24, 12, false, m == Mood::Cold ? 300 : 230);
  drawBlush(s, 192, 162 + by, 24, 12, false, m == Mood::Cold ? 300 : 230);

  switch (m) {
    case Mood::Happy:
      drawOpenMouth(s, {120, 166 + by, 20, 11, -2, false});
      break;
    case Mood::Meh:
      drawCurve(s, 108, 170 + by, 114, 164 + by, 120, 170 + by, 1.7f, INK);
      drawCurve(s, 120, 170 + by, 126, 176 + by, 132, 170 + by, 1.7f, INK);
      sweatDrop(s, 206, 50 + by);
      floatingZ(s, st.ms, 0, 176, 40, &fonts::FreeSansBold12pt7b);
      floatingZ(s, st.ms, 1700, 188, 32, &fonts::FreeSansBold9pt7b);
      break;
    case Mood::Grumpy:
      drawCurve(s, 108, 177 + by, 120, 165 + by, 132, 177 + by, 1.8f, INK);
      break;
    case Mood::Cold:
      drawZigzagMouth(s, 120, 172 + by, 24, st.ms, INK);
      break;
    case Mood::Confused:
      drawWavyMouth(s, 120, 172 + by, 24, INK);
      drawQuestion(s, 200, 52, st.ms, px::hex(0x8e6bbf));
      break;
  }
}
