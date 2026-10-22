#include "boot_button.h"

ButtonEvent BootButton::update(bool down, uint32_t ms) {
  if (clickPending_ && !down && ms - lastClickAt_ > CLICK_GAP_MS) clickPending_ = false;

  if (down && !down_) {
    down_ = true;
    downAt_ = ms;
    resetSent_ = false;
    // A second press only counts if it starts soon after the first click.
    if (clickPending_ && ms - lastClickAt_ > CLICK_GAP_MS) clickPending_ = false;
    return ButtonEvent::None;
  }

  if (down) {
    if (!resetSent_ && ms - downAt_ >= RESET_HOLD_MS) {
      resetSent_ = true;
      clickPending_ = false;
      return ButtonEvent::Reset;
    }
    return ButtonEvent::None;
  }

  if (!down_) return ButtonEvent::None;
  down_ = false;
  uint32_t len = ms - downAt_;
  if (len < DEBOUNCE_MS) {
    clickPending_ = false;
    return ButtonEvent::None;
  }
  if (len < CLICK_MAX_MS) {
    if (clickPending_) {
      clickPending_ = false;
      return ButtonEvent::DoubleClick;
    }
    clickPending_ = true;
    lastClickAt_ = ms;
    return ButtonEvent::None;
  }
  clickPending_ = false;
  if (len >= ROTATE_MIN_MS && len < ROTATE_MAX_MS) return ButtonEvent::Rotate;
  return ButtonEvent::None;
}

bool BootButton::held(uint32_t ms, uint32_t& heldMs) const {
  if (!down_) return false;
  heldMs = ms - downAt_;
  return true;
}
