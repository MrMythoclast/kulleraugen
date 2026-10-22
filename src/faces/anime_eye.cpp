#include "anime_eye.h"

#include <math.h>
#include <string.h>

// Per-pixel work is integer math on the sprite buffer (no FPU on the C3);
// floats only appear once per column or for the few pixels of a small shape.


namespace {

constexpr int RX_MAX = 40;

Rgb8 lut[64];
Sky lutSky = Sky::Cloud;
bool haveLut = false;

const Rgb8* irisLut(Sky sky) {
  if (!haveLut || sky != lutSky) {
    buildIrisLut(sky, lut, 64);
    lutSky = sky;
    haveLut = true;
  }
  return lut;
}

uint8_t isq[256];  // sqrt(v) * 16
bool haveIsq = false;

inline int isqrt16(int v) {
  if (!haveIsq) {
    for (int i = 0; i < 256; ++i) isq[i] = uint8_t(fminf(255, sqrtf(i) * 16));
    haveIsq = true;
  }
  return isq[v < 0 ? 0 : v > 255 ? 255 : v];
}

inline uint32_t inv2(int r) { return (1024u << 16) / uint32_t(r * r); }

inline uint32_t ell(int x, int y, uint32_t ix2, uint32_t iy2) {
  return (uint32_t(x * x) * ix2 + uint32_t(y * y) * iy2) >> 16;
}

// 0..256 coverage of a four-pointed star (astroid-like) of radius r at (x, y).
inline int starCover(int x, int y, int r) {
  int ax = abs(x) * 4, ay = abs(y) * 4;
  if (ax > 255 || ay > 255) return 0;
  int lim = isqrt16(r * 4);
  int d = lim - isqrt16(ax) - isqrt16(ay);
  if (d <= 0) return 0;
  return d >= 12 ? 256 : d * 256 / 12;
}

// 0..256 coverage of a heart of half-size r centred at (x, y), point downwards.
int heartShape(int x, int y, float r) {
  float X = x / r * 1.15f, Y = -y / r * 1.15f + 0.25f;
  float a = X * X + Y * Y - 1;
  float f = a * a * a - X * X * Y * Y * Y;
  if (f >= 0.04f) return 0;
  if (f <= -0.04f) return 256;
  return int((0.04f - f) * 3200);
}

// The heart formula needs floats, so each size is computed once into a mask.
constexpr int HEART_MAX = 10, HEART_HALF = HEART_MAX * 3 / 2, HEART_DIM = 2 * HEART_HALF + 1;
struct HeartMask {
  int8_t size = -1;
  uint8_t a[HEART_DIM * HEART_DIM];
};
HeartMask hearts[4];

const HeartMask& heartMask(int size) {
  if (size < 1) size = 1;
  if (size > HEART_MAX) size = HEART_MAX;
  for (auto& h : hearts)
    if (h.size == size) return h;
  static int next = 0;
  HeartMask& h = hearts[next];
  next = (next + 1) % 4;
  h.size = int8_t(size);
  for (int y = -HEART_HALF; y <= HEART_HALF; ++y)
    for (int x = -HEART_HALF; x <= HEART_HALF; ++x) {
      int k = heartShape(x, y, float(size));
      h.a[(y + HEART_HALF) * HEART_DIM + x + HEART_HALF] = uint8_t(k > 255 ? 255 : k);
    }
  return h;
}

inline int heartCover(int x, int y, const HeartMask& h) {
  if (abs(x) > HEART_HALF || abs(y) > HEART_HALF) return 0;
  int a = h.a[(y + HEART_HALF) * HEART_DIM + x + HEART_HALF];
  return a + (a >> 7);
}

uint8_t cov[64 * 64];  // per-stroke coverage, so joints between segments do not double up

}  // namespace

void IRAM_ATTR strokePolyline(LGFX_Sprite& s, const float* xy, int n, float r, Rgb8 col) {
  if (n < 2) return;
  float minx = xy[0], maxx = xy[0], miny = xy[1], maxy = xy[1];
  for (int i = 1; i < n; ++i) {
    minx = fminf(minx, xy[2 * i]);
    maxx = fmaxf(maxx, xy[2 * i]);
    miny = fminf(miny, xy[2 * i + 1]);
    maxy = fmaxf(maxy, xy[2 * i + 1]);
  }
  int x0 = int(floorf(minx - r - 1)), y0 = int(floorf(miny - r - 1));
  int w = int(ceilf(maxx + r + 1)) - x0 + 1, h = int(ceilf(maxy + r + 1)) - y0 + 1;
  if (w > 64 || h > 64) {  // too big for the coverage buffer: split
    if (n > 2) {
      int m = n / 2;
      strokePolyline(s, xy, m + 1, r, col);
      strokePolyline(s, xy + 2 * m, n - m, r, col);
    } else {
      float mx = (xy[0] + xy[2]) / 2, my = (xy[1] + xy[3]) / 2;
      float a[4] = {xy[0], xy[1], mx, my}, c[4] = {mx, my, xy[2], xy[3]};
      strokePolyline(s, a, 2, r, col);
      strokePolyline(s, c, 2, r, col);
    }
    return;
  }
  // cov stays all zero between calls; per row we remember which span got touched,
  // so blending and clearing skip the empty parts of a long diagonal's box.
  int8_t rowMin[64], rowMax[64];
  memset(rowMin, 127, h);
  memset(rowMax, -1, h);
  // Fixed point, 1/16 px. Coverage ramps over one pixel around the edge.
  const int R = int(r * 16), rp = R + 8, rm = R > 8 ? R - 8 : 0;
  const int rp2 = rp * rp, rm2 = rm * rm, band = rp2 - rm2;
  for (int i = 0; i + 1 < n; ++i) {
    int pax = int((xy[2 * i] - x0) * 16), pay = int((xy[2 * i + 1] - y0) * 16);
    int pbx = int((xy[2 * i + 2] - x0) * 16), pby = int((xy[2 * i + 3] - y0) * 16);
    // Long diagonal segments are cut into short pieces so each scans only a small box.
    int len = abs(pbx - pax) > abs(pby - pay) ? abs(pbx - pax) : abs(pby - pay);
    int pieces = len / (8 * 16) + 1;
    for (int j = 0; j < pieces; ++j) {
    int ax = pax + (pbx - pax) * j / pieces, ay = pay + (pby - pay) * j / pieces;
    int bx = pax + (pbx - pax) * (j + 1) / pieces, by = pay + (pby - pay) * (j + 1) / pieces;
    int dx = bx - ax, dy = by - ay, l2 = dx * dx + dy * dy;
    int sx0 = ((ax < bx ? ax : bx) - rp) >> 4, sx1 = ((ax > bx ? ax : bx) + rp) >> 4;
    int sy0 = ((ay < by ? ay : by) - rp) >> 4, sy1 = ((ay > by ? ay : by) + rp) >> 4;
    if (sx0 < 0) sx0 = 0;
    if (sy0 < 0) sy0 = 0;
    if (sx1 >= w) sx1 = w - 1;
    if (sy1 >= h) sy1 = h - 1;
    for (int y = sy0; y <= sy1; ++y) {
      int qy = y * 16 + 8 - ay;
      for (int x = sx0; x <= sx1; ++x) {
        int qx = x * 16 + 8 - ax;
        int t = l2 ? (qx * dx + qy * dy) * 256 / l2 : 0;
        if (t < 0) t = 0;
        if (t > 256) t = 256;
        int ex = qx - (dx * t >> 8), ey = qy - (dy * t >> 8);
        int d2 = ex * ex + ey * ey;
        if (d2 >= rp2) continue;
        int a = d2 <= rm2 ? 255 : (rp2 - d2) * 255 / band;
        uint8_t& c = cov[y * w + x];
        if (a > c) c = uint8_t(a);
        if (x < rowMin[y]) rowMin[y] = int8_t(x);
        if (x > rowMax[y]) rowMax[y] = int8_t(x);
      }
    }
    }
  }
  uint16_t* b = px::buf(s);
  for (int y = 0; y < h; ++y)
    for (int x = rowMin[y]; x <= rowMax[y]; ++x) {
      uint8_t& c = cov[y * w + x];
      int a = c;
      c = 0;
      if (a) px::blend(b, x0 + x, y0 + y, col, a + (a >> 7));
    }
}

void IRAM_ATTR drawAnimeEye(LGFX_Sprite& s, const AnimeEyeSpec& e, const FaceState& st, int side) {
  uint16_t* b = px::buf(s);
  const Rgb8* L = irisLut(st.sky);
  const IrisPalette pal = irisPalette(st.sky);

  const int rx = e.rx > RX_MAX ? RX_MAX : e.rx, ry = e.ry;
  const int cx = e.cx + lroundf(sinf(st.ms / 35.f) * 1.5f * st.shiver), cy = e.cy;

  float cover = st.top + e.heavyLid + (side > 0 ? st.asym : 0);
  if (cover > 1) cover = 1;
  if (st.blink > cover) cover = st.blink;
  const float tilt = st.tilt * (1 - st.blink);

  // Visible top edge per column: the lid or the eye outline, whichever is lower.
  int topY[2 * RX_MAX + 1];
  int lashW[2 * RX_MAX + 1];  // lash thickness in 1/256 px
  for (int c = 0; c <= 2 * rx; ++c) {
    float fx = float(c - rx) / rx;
    float outline = -ry * sqrtf(fmaxf(0, 1 - fx * fx));
    float inner = side < 0 ? (fx + 1) / 2 : (1 - fx) / 2;  // 1 at the corner next to the nose
    float lid = -ry + cover * 2 * ry + tilt * inner + cover * (1 - cover) * fx * fx * ry * 0.5f;
    topY[c] = int(fmaxf(lid, outline) * 256);
    float fade = fabsf(fx) > 0.9f ? (1 - fabsf(fx)) * 10 : 1;
    lashW[c] = int(e.lashWidth * (0.55f + 0.7f * (1 - inner)) * fade * 256);
  }

  // Smiling lower lid rising from below (Happy).
  const int by = int(ry * 1.45f + (1 - st.bottom) * ry * 1.75f);
  const uint32_t ibx = inv2(rx * 18 / 10), iby = inv2(ry * 105 / 100);
  const uint32_t irx = inv2(rx), iry = inv2(ry);
  const uint32_t iix = inv2(e.irisRx), iiy = inv2(e.irisRy);
  const uint32_t ipx = inv2(e.pupilRx), ipy = inv2(e.pupilRy);

  const int ix = lroundf(st.lookX * e.irisRx * 0.34f);
  const int iy = lroundf(st.lookY * e.irisRy * 0.22f) + ry / 10;

  // Highlights relative to the iris centre.
  const int h1x = -e.irisRx * 38 / 100, h1y = -e.irisRy * 40 / 100;
  const int h1rx = e.irisRx * 34 / 100, h1ry = e.irisRy * 30 / 100;
  const uint32_t ih1x = inv2(h1rx), ih1y = inv2(h1ry);
  const int h2x = e.irisRx * 42 / 100, h2y = e.irisRy * 42 / 100, h2r = e.irisRx * 17 / 100 + 1;
  const int h3x = e.irisRx * 20 / 100, h3y = -e.irisRy * 56 / 100, h3r = e.irisRx / 12 + 1;
  const int starR = e.irisRx * 46 / 100;
  const HeartMask& smallHeart = heartMask(lroundf(e.irisRx * 0.24f));
  const HeartMask& bigHeart = heartMask(lroundf(starR * 0.8f));
  const bool smile = by - ry * 105 / 100 < ry + 1;  // lower lid reaches into the eye at all

  const Rgb8 white{255, 255, 255};
  const Rgb8 scleraLow = px::hex(0xdfe6f3);
  const Rgb8 shadow = px::mix(e.lashColor, px::hex(0x5a5078), 128);
  const Rgb8 pupilCol = px::scale(pal.top, 110);
  const Rgb8 rimCol = px::scale(pal.top, 170);
  const int lashMax = e.lashWidth + 2;

  for (int dy = -ry - lashMax; dy <= ry; ++dy) {
    int y = cy + dy;
    if (unsigned(y) >= px::H) continue;
    Rgb8 scl = px::mix(white, scleraLow, (dy + ry) * 128 / ry > 256 ? 256 : (dy + ry) * 128 / ry);
    int irisIdx = (dy - iy + e.irisRy) * 63 / (2 * e.irisRy);
    const Rgb8 irisRow = L[irisIdx < 0 ? 0 : irisIdx > 63 ? 63 : irisIdx];
    for (int dx = -rx; dx <= rx; ++dx) {
      int x = cx + dx;
      if (unsigned(x) >= px::W) continue;
      int c = dx + rx;
      int d = dy * 256 - topY[c];  // > 0 below the visible top edge
      int lw = lashW[c];
      bool inLash = lw > 0 && d <= 256 && d > -lw;
      if (d <= 0 && !inLash) continue;

      int bm = 256;
      if (smile) {
        uint32_t hb = ell(dx, dy - by, ibx, iby);
        if (hb <= 1024) continue;
        if (hb < 1094) bm = int(hb - 1024) * 256 / 70;
      }

      Rgb8 col{0, 0, 0};
      int a = 0;
      uint32_t e2 = ell(dx, dy, irx, iry);
      if (e2 < 1024 && d > 0) {
        a = e2 > 1024 - 68 ? int(1024 - e2) * 256 / 68 : 256;
        if (d < 256) a = a * d >> 8;
        col = scl;

        int qx = dx - ix, qy = dy - iy;
        uint32_t ir = ell(qx, qy, iix, iiy);
        if (ir < 1024) {
          Rgb8 ic = irisRow;
          if (qy > 0 && ir > 520 && ir < 800) {  // glowing arc in the lower half
            int k = 140 - abs(int(ir) - 660);
            if (k > 0) ic = px::mix(ic, pal.bottom, k * 100 / 140);
          }
          if (e.innerRing && ir > 360 && ir < 480) {
            int k = 60 - abs(int(ir) - 420);
            if (k > 0) ic = px::mix(ic, pal.bottom, k * 150 / 60);
          }
          uint32_t pr = ell(qx, qy + e.pupilRy / 6, ipx, ipy);
          if (pr < 1024) ic = px::mix(ic, pupilCol, pr > 900 ? int(1024 - pr) * 256 / 124 : 256);
          if (ir > 820) ic = px::mix(ic, rimCol, ir > 1000 ? 256 : int(ir - 820) * 256 / 180);
          col = ir > 980 ? px::mix(col, ic, int(1024 - ir) * 256 / 44) : ic;
        }

        // Highlights sit inside the iris; skip the tests everywhere else.
        int hx = qx - h1x, hy = qy - h1y, k = 0;
        if (ir < 1024) {
        if (e.main == Highlight::Oval) {
          if (abs(hx) < h1rx && abs(hy) < h1ry) {
            uint32_t hr = ell(hx, hy, ih1x, ih1y);
            if (hr < 1024) k = hr > 880 ? int(1024 - hr) * 256 / 144 : 256;
          }
        } else if (e.main == Highlight::Star) {
          if (abs(hx) <= starR && abs(hy) <= starR) k = starCover(hx, hy, starR);
        } else {
          k = heartCover(hx, hy, bigHeart);
        }
        if (!k) {
          int sx = qx - h2x, sy = qy - h2y;
          if (e.main == Highlight::Star) {
            k = heartCover(sx, sy, smallHeart);
          } else {
            int r2 = sx * sx + sy * sy, lim = h2r * h2r;
            if (r2 < lim) k = r2 > lim - 2 * h2r ? (lim - r2) * 256 / (2 * h2r) : 256;
          }
        }
        if (!k) {
          int tx = qx - h3x, ty = qy - h3y;
          int r2 = tx * tx + ty * ty, lim = h3r * h3r;
          if (r2 < lim) k = 230;
        }
        }
        if (k) col = px::mix(col, white, k);

        if (d < 14 * 256) col = px::mix(col, shadow, (14 * 256 - d) / 14 * 110 >> 8);
      }

      if (inLash) {
        int la = 256;
        if (d < -lw + 256) la = d + lw;
        if (d > 0) la = 256 - d;
        if (la > 0) {
          col = a ? px::mix(col, e.lashColor, la) : e.lashColor;
          if (la > a) a = la;
        }
      }
      px::blend(b, x, y, col, a * bm >> 8);
    }
  }

  // Lash tips at the outer corner, only while the eye is reasonably open.
  if (e.lashFlicks && cover < 0.75f) {
    float lift = 1 - cover;
    for (int i = 0; i < e.lashFlicks; ++i) {
      float f = 0.80f + 0.12f * i;
      int c = rx + side * int(f * rx);
      float x0 = cx + side * f * rx, y0 = cy + topY[c] / 256.f + 1;
      float len = (11 - 3 * i) * lift;
      float pts[4] = {x0, y0, x0 + side * len, y0 - len * (0.8f - 0.25f * i)};
      strokePolyline(s, pts, 2, e.lashWidth * 0.32f + 0.6f, e.lashColor);
    }
  }
}

void IRAM_ATTR drawBlush(LGFX_Sprite& s, int cx, int cy, int rx, int ry, bool hatching, int strength) {
  uint16_t* b = px::buf(s);
  const Rgb8 pink = px::hex(0xff7aa8);
  const uint32_t ix = inv2(rx), iy = inv2(ry);
  for (int dy = -ry; dy <= ry; ++dy) {
    for (int dx = -rx; dx <= rx; ++dx) {
      uint32_t e2 = ell(dx, dy, ix, iy);
      if (e2 >= 1024) continue;
      int a = int(1024 - e2) * strength >> 10;
      px::blend(b, cx + dx, cy + dy, pink, a * 200 >> 8);
    }
  }
  if (!hatching) return;
  for (int i = -1; i <= 1; ++i) {
    float x = cx + i * rx * 0.42f;
    float pts[4] = {x - 2.5f, cy + ry * 0.45f, x + 2.5f, cy - ry * 0.45f};
    strokePolyline(s, pts, 2, 0.9f, px::hex(0xffb0cc));
  }
}

void drawSparkle(LGFX_Sprite& s, int cx, int cy, int r, Rgb8 col) {
  uint16_t* b = px::buf(s);
  for (int dy = -r; dy <= r; ++dy) {
    for (int dx = -r; dx <= r; ++dx) {
      int k = starCover(dx, dy, r);
      int g2 = dx * dx + dy * dy, gr = r * r / 5 + 1;  // soft glow in the middle
      if (g2 < gr) k += (gr - g2) * 120 / gr;
      if (k) px::blend(b, cx + dx, cy + dy, col, k > 256 ? 256 : k);
    }
  }
}

void drawHeart(LGFX_Sprite& s, int cx, int cy, int size, Rgb8 col) {
  uint16_t* b = px::buf(s);
  const HeartMask& h = heartMask(size);
  int r = size * 3 / 2;
  for (int dy = -r; dy <= r; ++dy)
    for (int dx = -r; dx <= r; ++dx) {
      int k = heartCover(dx, dy, h);
      if (k) px::blend(b, cx + dx, cy + dy, col, k);
    }
}

void IRAM_ATTR fillGradient(LGFX_Sprite& s, Rgb8 top, Rgb8 mid, Rgb8 bottom) {
  // Ordered dithering hides the 16-bit colour steps in dark gradients.
  // Every row and column averages the same, so the pattern adds no stripes of its own.
  static const uint8_t BAYER[4][4] = {{0, 14, 3, 13}, {11, 5, 8, 6}, {12, 2, 15, 1}, {7, 9, 4, 10}};
  uint16_t* b = px::buf(s);
  const int half = px::H / 2;
  for (int y = 0; y < px::H; ++y) {
    const Rgb8 &a = y < half ? top : mid, &c = y < half ? mid : bottom;
    int t = (y < half ? y : y - half) * 256 / half;
    // Channels in 1/16 steps of their 565 quantum (8 for red/blue, 4 for green).
    int r = (a.r * 256 + (c.r - a.r) * t) * 2 / 256, g = (a.g * 256 + (c.g - a.g) * t) * 4 / 256,
        bl = (a.b * 256 + (c.b - a.b) * t) * 2 / 256;
    const uint8_t* d = BAYER[y & 3];
    uint16_t pat[4];
    for (int i = 0; i < 4; ++i) {
      int k = d[i];
      int r5 = (r + k) >> 4, g6 = (g + k) >> 4, b5 = (bl + k) >> 4;
      if (r5 > 31) r5 = 31;
      if (g6 > 63) g6 = 63;
      if (b5 > 31) b5 = 31;
      uint16_t v = uint16_t(r5 << 11 | g6 << 5 | b5);
      pat[i] = uint16_t((v >> 8) | (v << 8));
    }
    // Two pixels per 32-bit write; the buffer is word aligned and 240 is a multiple of 4.
    uint32_t w0 = pat[0] | uint32_t(pat[1]) << 16, w1 = pat[2] | uint32_t(pat[3]) << 16;
    uint32_t* row = reinterpret_cast<uint32_t*>(b + y * px::W);
    for (int x = 0; x < px::W / 2; x += 2) {
      row[x] = w0;
      row[x + 1] = w1;
    }
  }
}
