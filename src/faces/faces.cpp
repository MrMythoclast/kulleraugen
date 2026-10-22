#include "faces.h"

#include "anim.h"
#include "face_list.h"

namespace {

Mood mood = Mood::Meh;
Sky sky = Sky::Cloud;

}  // namespace

void facesSetMood(Mood m) {
  mood = m;
  eyesSetMood(m);
}

void facesSetPalette(Sky s) {
  sky = s;
  eyesSetPalette(s);
}

void facesDraw(LGFX_Sprite& s, uint32_t ms, uint8_t face) {
  if (face == 0) {
    eyesDraw(s, ms);
    return;
  }
  animUpdate(ms, mood, sky);
  const FaceState& st = animState();
  switch (face) {
    case 1: drawFaceGlanz(s, st); break;
    case 2: drawFaceKatze(s, st); break;
    case 3: drawFaceGlitzer(s, st); break;
    default: drawFacePastell(s, st); break;
  }
}
