#include <Arduino.h>
#include <iris_palette.h>
#include <unity.h>

namespace {

void assertRgb(uint32_t hex, Rgb8 c) {
  TEST_ASSERT_EQUAL_HEX8(hex >> 16, c.r);
  TEST_ASSERT_EQUAL_HEX8((hex >> 8) & 0xff, c.g);
  TEST_ASSERT_EQUAL_HEX8(hex & 0xff, c.b);
}

void assertPalette(Sky sky, uint32_t top, uint32_t mid, uint32_t light, uint32_t bottom) {
  IrisPalette p = irisPalette(sky);
  assertRgb(top, p.top);
  assertRgb(mid, p.mid);
  assertRgb(light, p.light);
  assertRgb(bottom, p.bottom);
}

}  // namespace

void test_sun_palette() { assertPalette(Sky::Sun, 0x3a1a05, 0xb8560a, 0xffb300, 0xffe9a6); }
void test_cloud_palette() { assertPalette(Sky::Cloud, 0x1b2630, 0x4f6b7a, 0x9fb8c6, 0xeef3f6); }
void test_rain_palette() { assertPalette(Sky::Rain, 0x06213a, 0x0b5fa8, 0x4fc3f7, 0xe1f5fe); }
void test_snow_palette() { assertPalette(Sky::Snow, 0x0d2f3a, 0x2a8fa8, 0x80deea, 0xffffff); }

void test_lut_ends() {
  Rgb8 lut[64];
  buildIrisLut(Sky::Rain, lut, 64);
  assertRgb(0x06213a, lut[0]);
  assertRgb(0xe1f5fe, lut[63]);
}

void test_lut_mid_stop() {
  Rgb8 lut[64];
  buildIrisLut(Sky::Sun, lut, 64);
  Rgb8 m = lut[28];  // round(0.45 * 63)
  TEST_ASSERT_INT_WITHIN(3, 0xb8, m.r);
  TEST_ASSERT_INT_WITHIN(3, 0x56, m.g);
  TEST_ASSERT_INT_WITHIN(3, 0x0a, m.b);
}

void test_clamp_face() {
  TEST_ASSERT_EQUAL(3, clampFace(3, 5));
  TEST_ASSERT_EQUAL(0, clampFace(5, 5));
  TEST_ASSERT_EQUAL(0, clampFace(-1, 5));
  TEST_ASSERT_EQUAL(0, clampFace(255, 5));
}

void setup() {
  // Wait for the test runner to open the USB serial port, then give it a moment.
  for (uint32_t t = millis(); !Serial && millis() - t < 15000;) delay(10);
  delay(500);
  UNITY_BEGIN();
  RUN_TEST(test_sun_palette);
  RUN_TEST(test_cloud_palette);
  RUN_TEST(test_rain_palette);
  RUN_TEST(test_snow_palette);
  RUN_TEST(test_lut_ends);
  RUN_TEST(test_lut_mid_stop);
  RUN_TEST(test_clamp_face);
  UNITY_END();
}

void loop() {}
