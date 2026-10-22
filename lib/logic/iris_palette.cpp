#include "iris_palette.h"

namespace {

constexpr Rgb8 hex(uint32_t v) { return {uint8_t(v >> 16), uint8_t(v >> 8), uint8_t(v)}; }

Rgb8 mix(Rgb8 a, Rgb8 b, int k, int n) {  // k/n of the way from a to b
  return {uint8_t(a.r + (b.r - a.r) * k / n), uint8_t(a.g + (b.g - a.g) * k / n),
          uint8_t(a.b + (b.b - a.b) * k / n)};
}

}  // namespace

IrisPalette irisPalette(Sky sky) {
  switch (sky) {
    case Sky::Sun:   return {hex(0x3a1a05), hex(0xb8560a), hex(0xffb300), hex(0xffe9a6)};
    case Sky::Cloud: return {hex(0x1b2630), hex(0x4f6b7a), hex(0x9fb8c6), hex(0xeef3f6)};
    case Sky::Rain:  return {hex(0x06213a), hex(0x0b5fa8), hex(0x4fc3f7), hex(0xe1f5fe)};
    case Sky::Snow:  return {hex(0x0d2f3a), hex(0x2a8fa8), hex(0x80deea), hex(0xffffff)};
  }
  return irisPalette(Sky::Cloud);
}

void buildIrisLut(Sky sky, Rgb8* lut, int n) {
  IrisPalette p = irisPalette(sky);
  const Rgb8 stops[4] = {p.top, p.mid, p.light, p.bottom};
  const int at[4] = {0, 450, 800, 1000};  // per mille
  int last = n - 1;
  for (int i = 0; i < n; ++i) {
    int t = last > 0 ? i * 1000 / last : 0;
    int s = t < at[1] ? 0 : t < at[2] ? 1 : 2;
    lut[i] = mix(stops[s], stops[s + 1], t - at[s], at[s + 1] - at[s]);
  }
}

uint8_t clampFace(int32_t stored, uint8_t count) {
  return stored >= 0 && stored < count ? uint8_t(stored) : 0;
}
