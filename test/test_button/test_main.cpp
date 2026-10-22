#include <Arduino.h>
#include <boot_button.h>
#include <unity.h>

#include <vector>

namespace {

struct Ev {
  ButtonEvent ev;
  uint32_t at;
};

struct Sim {
  BootButton btn;
  uint32_t t = 1000;
  std::vector<Ev> evs;

  void run(bool down, uint32_t len) {
    for (uint32_t end = t + len; t < end; t += 10) {
      ButtonEvent e = btn.update(down, t);
      if (e != ButtonEvent::None) evs.push_back({e, t});
    }
  }
  void press(uint32_t len) { run(true, len); }
  void idle(uint32_t len) { run(false, len); }
  int count(ButtonEvent e) const {
    int n = 0;
    for (auto& x : evs) n += x.ev == e;
    return n;
  }
};

}  // namespace

void test_single_click_nothing() {
  Sim s;
  s.press(100);
  s.idle(1000);
  TEST_ASSERT_EQUAL(0, s.evs.size());
}

void test_double_click() {
  Sim s;
  s.press(100);
  s.idle(200);
  s.press(100);
  s.idle(1000);
  TEST_ASSERT_EQUAL(1, s.evs.size());
  TEST_ASSERT_EQUAL(1, s.count(ButtonEvent::DoubleClick));
}

void test_gap_too_long() {
  Sim s;
  s.press(100);
  s.idle(500);
  s.press(100);
  s.idle(1000);
  TEST_ASSERT_EQUAL(0, s.evs.size());
}

void test_second_press_too_long() {
  Sim s;
  s.press(100);
  s.idle(200);
  s.press(600);
  s.idle(1000);
  TEST_ASSERT_EQUAL(0, s.evs.size());
}

void test_triple_click_one_switch() {
  Sim s;
  for (int i = 0; i < 3; ++i) {
    s.press(100);
    s.idle(200);
  }
  s.idle(1000);
  TEST_ASSERT_EQUAL(1, s.evs.size());
  TEST_ASSERT_EQUAL(1, s.count(ButtonEvent::DoubleClick));
}

void test_hold_rotates() {
  Sim s;
  s.press(2000);
  s.idle(500);
  TEST_ASSERT_EQUAL(1, s.evs.size());
  TEST_ASSERT_EQUAL(1, s.count(ButtonEvent::Rotate));
}

void test_hold_too_long_no_rotate() {
  Sim s;
  s.press(3500);
  s.idle(500);
  TEST_ASSERT_EQUAL(0, s.evs.size());
}

void test_reset_at_5s() {
  Sim s;
  uint32_t start = s.t;
  s.press(5200);
  s.idle(500);
  TEST_ASSERT_EQUAL(1, s.evs.size());
  TEST_ASSERT_EQUAL(1, s.count(ButtonEvent::Reset));
  TEST_ASSERT_TRUE(s.evs[0].at - start >= 5000);
}

void test_double_then_hold_rotates_only() {
  Sim s;
  s.press(100);
  s.idle(200);
  s.press(2000);
  s.idle(1000);
  TEST_ASSERT_EQUAL(1, s.evs.size());
  TEST_ASSERT_EQUAL(1, s.count(ButtonEvent::Rotate));
}

void test_bounce_ignored() {
  Sim s;
  s.press(10);
  s.idle(200);
  s.press(100);
  s.idle(1000);
  TEST_ASSERT_EQUAL(0, s.evs.size());
}

void test_held_reports_duration() {
  Sim s;
  uint32_t heldMs = 0;
  TEST_ASSERT_FALSE(s.btn.held(s.t, heldMs));
  uint32_t start = s.t;
  s.press(1500);
  TEST_ASSERT_TRUE(s.btn.held(s.t, heldMs));
  TEST_ASSERT_UINT32_WITHIN(20, s.t - start, heldMs);
}

void setup() {
  // Wait for the test runner to open the USB serial port, then give it a moment.
  for (uint32_t t = millis(); !Serial && millis() - t < 15000;) delay(10);
  delay(500);
  UNITY_BEGIN();
  RUN_TEST(test_single_click_nothing);
  RUN_TEST(test_double_click);
  RUN_TEST(test_gap_too_long);
  RUN_TEST(test_second_press_too_long);
  RUN_TEST(test_triple_click_one_switch);
  RUN_TEST(test_hold_rotates);
  RUN_TEST(test_hold_too_long_no_rotate);
  RUN_TEST(test_reset_at_5s);
  RUN_TEST(test_double_then_hold_rotates_only);
  RUN_TEST(test_bounce_ignored);
  RUN_TEST(test_held_reports_duration);
  UNITY_END();
}

void loop() {}
