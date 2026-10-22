#include "anim.h"

#include <Arduino.h>

namespace {

FaceState st{Mood::Meh, Sky::Cloud, 0, 0, 0.38f, 0, 0, 0, 0, 0, 0, 0, {0, 1, 0, 0, 0}};
float targetX = 0, targetY = 0;
uint32_t nextLook = 0, nextBlink = 2000, blinkStart = 0, lastMs = 0;
bool blinkedOnce = false;

struct Lids {
  float top, tilt, bottom, shiver, asym;
};

Lids lidsFor(Mood m) {
  switch (m) {
    case Mood::Happy:    return {0.00f, 0, 1, 0, 0};
    case Mood::Meh:      return {0.30f, 0, 0, 0, 0};
    case Mood::Grumpy:   return {0.22f, 16, 0, 0, 0};
    case Mood::Cold:     return {0.20f, 0, 0, 1, 0};
    case Mood::Confused: return {0.08f, 0, 0, 0, 0.3f};
  }
  return {0.30f, 0, 0, 0, 0};
}

inline void ease(float& v, float target, float k) { v += (target - v) * k; }

}  // namespace

void animUpdate(uint32_t ms, Mood mood, Sky sky) {
  float dt = (ms - lastMs) / 1000.f;
  if (dt > 0.1f) dt = 0.1f;
  lastMs = ms;
  st.ms = ms;
  st.dt = dt;
  st.mood = mood;
  st.sky = sky;

  float k = dt * 6 > 1 ? 1 : dt * 6;
  Lids t = lidsFor(mood);
  ease(st.top, t.top, k);
  ease(st.tilt, t.tilt, k);
  ease(st.bottom, t.bottom, k);
  ease(st.shiver, t.shiver, k);
  ease(st.asym, t.asym, k);
  for (int i = 0; i < 5; ++i) ease(st.moodMix[i], i == int(mood) ? 1.f : 0.f, k);

  // Quick glances to random spots, more restless when confused.
  if (int32_t(ms - nextLook) >= 0) {
    targetX = random(-100, 101) / 100.f;
    targetY = random(-70, 71) / 100.f;
    nextLook = ms + (mood == Mood::Confused ? random(300, 900) : random(800, 3000));
  }
  float lk = dt * 12 > 1 ? 1 : dt * 12;
  ease(st.lookX, targetX, lk);
  ease(st.lookY, targetY, lk);

  if (int32_t(ms - nextBlink) >= 0) {
    blinkStart = ms;
    blinkedOnce = true;
    nextBlink = ms + (random(4) == 0 ? 260 : random(2500, 6000));  // sometimes a double blink
  }
  uint32_t bt = ms - blinkStart;
  st.blink = blinkedOnce && bt < 180 ? sinf(bt * PI / 180) : 0;
}

const FaceState& animState() { return st; }
