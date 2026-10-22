#include "debug_tools.h"

namespace {

char line[8];
uint8_t lineLen = 0;
int forcedMood = -1, forcedSky = -1;
bool holdFace = false, nextFace = false;
uint32_t frames = 0, fpsSince = 0;

void sendSnapshot(LGFX_Sprite& s) {
  Serial.printf("SNAP %d %d\n", s.width(), s.height());
  Serial.write(static_cast<const uint8_t*>(s.getBuffer()), size_t(s.width()) * s.height() * 2);
  Serial.print("\nEND\n");
  Serial.flush();
}

void runCommand(LGFX_Sprite& s) {
  char c = line[0];
  int arg = lineLen > 1 ? line[1] - '0' : -1;
  if (c == 's') sendSnapshot(s);
  else if (c == 'f') nextFace = true;
  else if (c == 'm' && arg >= 0 && arg <= 4) forcedMood = arg;
  else if (c == 'w' && arg >= 0 && arg <= 3) forcedSky = arg;
  else if (c == 'x') forcedMood = forcedSky = -1;
  else if (c == 'p') holdFace = !holdFace;
}

}  // namespace

void debugPoll(LGFX_Sprite& s) {
  while (Serial.available()) {
    char c = Serial.read();
    if (c == '\n' || c == '\r' || c == ' ') {
      if (lineLen) runCommand(s);
      lineLen = 0;
    } else if (lineLen < sizeof line - 1) {
      line[lineLen++] = c;
      line[lineLen] = 0;
    }
  }
}

void debugFrameDone(uint32_t ms) {
  ++frames;
  if (ms - fpsSince < 5000) return;
  Serial.printf("FPS %.1f\n", frames * 1000.f / (ms - fpsSince));
  frames = 0;
  fpsSince = ms;
}

bool debugForcedMood(Mood& out) {
  if (forcedMood < 0) return false;
  out = Mood(forcedMood);
  return true;
}

bool debugForcedSky(Sky& out) {
  if (forcedSky < 0) return false;
  out = Sky(forcedSky);
  return true;
}

bool debugHoldFace() { return holdFace; }

bool debugTakeNextFace() {
  bool n = nextFace;
  nextFace = false;
  return n;
}
