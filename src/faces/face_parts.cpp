#include "face_parts.h"

#include <math.h>

namespace {

inline uint16_t c565(Rgb8 c) { return lgfx::color565(c.r, c.g, c.b); }

const Rgb8 MOUTH_IN = px::hex(0x5a1426);
const Rgb8 MOUTH_LINE = px::hex(0x3a0d18);
const Rgb8 TONGUE_TOP = px::hex(0xff8fa8);
const Rgb8 TONGUE_LOW = px::hex(0xe2456b);

}  // namespace

void drawCurve(LGFX_Sprite& s, float x0, float y0, float qx, float qy, float x1, float y1, float r, Rgb8 col) {
  constexpr int N = 10;
  float pts[2 * (N + 1)];
  for (int i = 0; i <= N; ++i) {
    float t = float(i) / N, u = 1 - t;
    pts[2 * i] = u * u * x0 + 2 * u * t * qx + t * t * x1;
    pts[2 * i + 1] = u * u * y0 + 2 * u * t * qy + t * t * y1;
  }
  strokePolyline(s, pts, N + 1, r, col);
}

void drawOpenMouth(LGFX_Sprite& s, const OpenMouth& m) {
  uint16_t* b = px::buf(s);
  int hw = m.w / 2;
  if (hw < 2 || m.h < 2) return;
  // Floats once per column; per pixel only 8.8 fixed point.
  constexpr int LINE_W = 307, FADE = 51;  // 1.2 px outline, soft outer 0.2 px
  const int teethF = int((m.h * 0.18f + 1) * 256);
  for (int dx = -hw - 1; dx <= hw + 1; ++dx) {
    float fx = float(dx) / hw;
    if (fabsf(fx) > 1.05f) continue;
    float f2 = fminf(1, fx * fx);
    float topE = m.top + m.topCurve * f2;               // top edge
    float botE = m.top + m.h * sqrtf(fmaxf(0, 1 - f2)); // rounded bottom
    if (botE < topE + 1) botE = topE + 1;
    const int topF = int(topE * 256), botF = int(botE * 256), span = botF - topF;
    const int sideF = fabsf(fx) > 0.98f ? int(((1.05f - fabsf(fx)) * hw - 0.5f) * 256) : 1 << 20;
    // Tongue in the lower part, a rounded hump.
    int tongueF = 1 << 20, tongueK = 0;
    if (f2 < 0.5f) {
      tongueF = topF + int((1 - 0.45f * sqrtf(1 - f2 / 0.5f)) * span);
      tongueK = (400 << 8) / span;
    }
    int y0 = int(floorf(topE)) - 2, y1 = int(ceilf(botE)) + 2;
    for (int y = y0; y <= y1; ++y) {
      const int yF = y * 256;
      int in = yF - topF < botF - yF ? yF - topF : botF - yF;
      if (sideF < in) in = sideF;
      if (in < -LINE_W) continue;
      Rgb8 col = MOUTH_IN;
      if (yF > tongueF) col = px::mix(TONGUE_TOP, TONGUE_LOW, (yF - tongueF) * tongueK >> 8);
      if (m.teeth && yF - topF < teethF) col = {255, 255, 255};
      if (in < LINE_W) col = MOUTH_LINE;  // outline
      px::blend(b, m.cx + dx, y, col, in >= -FADE ? 256 : in + LINE_W);
    }
  }
}

void drawZigzagMouth(LGFX_Sprite& s, int cx, int cy, int w, uint32_t ms, Rgb8 col) {
  int n = 6;
  float amp = 3.5f + sinf(ms / 45.f) * 1.5f;  // shivering
  float x0 = cx - w / 2.f, step = float(w) / n;
  float pts[2 * 7];
  for (int i = 0; i <= n; ++i) {
    pts[2 * i] = x0 + i * step;
    pts[2 * i + 1] = cy + (i & 1 ? amp : -amp);
  }
  strokePolyline(s, pts, n + 1, 1.6f, col);
}

void drawWavyMouth(LGFX_Sprite& s, int cx, int cy, int w, Rgb8 col) {
  float x0 = cx - w / 2.f;
  drawCurve(s, x0, cy, x0 + w * 0.25f, cy - 7, cx, cy, 1.7f, col);
  drawCurve(s, cx, cy, x0 + w * 0.75f, cy + 7, x0 + w, cy, 1.7f, col);
}

void drawQuestion(LGFX_Sprite& s, int x, int y, uint32_t ms, Rgb8 col) {
  s.setFont(&fonts::FreeSansBold18pt7b);
  s.setTextDatum(textdatum_t::middle_center);
  s.setTextColor(c565(col));
  s.drawString("?", x, y + lroundf(sinf(ms / 400.f) * 3));
}

Mood dominantMood(const FaceState& st) {
  int best = 0;
  for (int i = 1; i < 5; ++i)
    if (st.moodMix[i] > st.moodMix[best]) best = i;
  return Mood(best);
}

int bob(uint32_t ms) { return lroundf((sinf(ms * 2 * PI / 3000) + 1) * 1.0f); }
