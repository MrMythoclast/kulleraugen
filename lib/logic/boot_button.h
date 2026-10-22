#pragma once

#include <stdint.h>

enum class ButtonEvent : uint8_t { None, DoubleClick, Rotate, Reset };

constexpr uint32_t CLICK_MAX_MS = 400;   // a click is shorter than this
constexpr uint32_t CLICK_GAP_MS = 400;   // second click must start within this
constexpr uint32_t ROTATE_MIN_MS = 1500;
constexpr uint32_t ROTATE_MAX_MS = 3000;
constexpr uint32_t RESET_HOLD_MS = 5000;
constexpr uint32_t DEBOUNCE_MS = 30;

// Turns the raw BOOT level into gestures. Call once per frame.
class BootButton {
 public:
  ButtonEvent update(bool down, uint32_t ms);
  bool held(uint32_t ms, uint32_t& heldMs) const;

 private:
  bool down_ = false;
  bool resetSent_ = false;
  uint32_t downAt_ = 0;
  uint32_t lastClickAt_ = 0;  // release time of a pending first click
  bool clickPending_ = false;
};
