#include "screens.h"

#include <math.h>

namespace {

constexpr int CX = 120, CY = 120;

const char* const WEEKDAYS[] = {"Sonntag", "Montag",  "Dienstag", "Mittwoch",
                                "Donnerstag", "Freitag", "Samstag"};

struct Particle {
  uint8_t x, y, speed;
};
Particle parts[40];
bool partsReady = false;

void initParticles() {
  for (auto& p : parts) {
    p.x = random(240);
    p.y = random(240);
    p.speed = random(1, 4);
  }
  partsReady = true;
}

void cloudShape(LGFX_Sprite& s, int x, int y, int sz, uint16_t col) {
  s.fillSmoothCircle(x - sz * 30 / 100, y + sz * 8 / 100, sz * 24 / 100, col);
  s.fillSmoothCircle(x + sz * 2 / 100, y - sz * 10 / 100, sz * 32 / 100, col);
  s.fillSmoothCircle(x + sz * 32 / 100, y + sz * 8 / 100, sz * 22 / 100, col);
  s.fillRoundRect(x - sz * 30 / 100, y + sz * 5 / 100, sz * 62 / 100, sz * 27 / 100, sz * 12 / 100, col);
}

void drawSkyIcon(LGFX_Sprite& s, int cx, int cy, int sz, Sky sky) {
  switch (sky) {
    case Sky::Sun: {
      uint16_t col = lgfx::color565(255, 179, 0);
      s.fillSmoothCircle(cx, cy, sz * 26 / 100, col);
      for (int i = 0; i < 8; ++i) {
        float a = i * PI / 4;
        s.drawWideLine(cx + sinf(a) * sz * 0.36f, cy - cosf(a) * sz * 0.36f,
                       cx + sinf(a) * sz * 0.48f, cy - cosf(a) * sz * 0.48f, sz / 28.f + 0.8f, col);
      }
      break;
    }
    case Sky::Cloud:
      cloudShape(s, cx, cy, sz, lgfx::color565(207, 216, 220));
      break;
    case Sky::Rain: {
      cloudShape(s, cx, cy - sz * 14 / 100, sz, lgfx::color565(176, 190, 197));
      uint16_t col = lgfx::color565(79, 195, 247);
      for (int i = -1; i <= 1; ++i) {
        int x = cx + i * sz * 22 / 100;
        s.drawWideLine(x, cy + sz * 26 / 100, x - sz * 6 / 100, cy + sz * 44 / 100, sz / 30.f + 0.6f, col);
      }
      break;
    }
    case Sky::Snow:
      cloudShape(s, cx, cy - sz * 14 / 100, sz, lgfx::color565(176, 190, 197));
      for (int i = -1; i <= 1; ++i) {
        s.fillSmoothCircle(cx + i * sz * 22 / 100, cy + sz * 36 / 100 + (i & 1) * sz / 12, sz / 14 + 1,
                           TFT_WHITE);
      }
      break;
  }
}

// Draws "23°" centered; the GFX fonts have no degree sign, so it's a small ring.
void drawTemp(LGFX_Sprite& s, int cx, int cy, int t, const lgfx::IFont* font, uint16_t col) {
  s.setFont(font);
  s.setTextColor(col);
  s.setTextDatum(textdatum_t::middle_left);
  String str(t);
  int fh = s.fontHeight();
  int r = fh / 9 + 1;
  int w = s.textWidth(str);
  int left = cx - (w + 3 + 2 * r) / 2;
  s.drawString(str, left, cy);
  int ox = left + w + 3 + r, oy = cy - fh / 4;
  s.drawCircle(ox, oy, r, col);
  s.drawCircle(ox, oy, r - 1, col);
}

void drawWeatherBg(LGFX_Sprite& s, uint32_t ms, Sky sky) {
  if (!partsReady) initParticles();
  switch (sky) {
    case Sky::Sun: {
      uint16_t col = lgfx::color565(77, 54, 0);
      float rot = ms / 4000.f;
      for (int i = 0; i < 16; ++i) {
        float a = rot + i * PI / 8;
        s.drawWideLine(CX + sinf(a) * 70, CY - cosf(a) * 70, CX + sinf(a) * 118, CY - cosf(a) * 118, 1.5f, col);
      }
      break;
    }
    case Sky::Cloud: {
      uint16_t col = lgfx::color565(34, 39, 45);
      for (int i = 0; i < 4; ++i) {
        int x = int((ms / 40 + i * 80) % 340) - 50, y = 40 + i * 45;
        s.fillCircle(x, y, 22, col);
        s.fillCircle(x + 25, y - 8, 26, col);
        s.fillCircle(x + 50, y, 20, col);
      }
      break;
    }
    case Sky::Rain: {
      uint16_t col = lgfx::color565(28, 68, 87);
      for (auto& p : parts) {
        int y = int((p.y + ms * p.speed / 3) % 244);
        s.drawLine(p.x, y, p.x - 3, y + 10, col);
      }
      break;
    }
    case Sky::Snow: {
      uint16_t col = lgfx::color565(90, 90, 90);
      for (auto& p : parts) {
        int y = int((p.y + ms * p.speed / 30) % 244);
        int x = p.x + int(sinf(ms / 600.f + p.y) * 6);
        s.fillCircle(x, y, p.speed, col);
      }
      break;
    }
  }
}

}  // namespace

void drawTimeScreen(LGFX_Sprite& s, uint32_t ms, const tm& now, const WeatherData& w) {
  if (w.valid) drawWeatherBg(s, ms, w.sky);

  char buf[24];
  snprintf(buf, sizeof buf, "%02d:%02d", now.tm_hour, now.tm_min);
  s.setFont(&fonts::FreeSansBold24pt7b);
  s.setTextDatum(textdatum_t::middle_center);
  s.setTextColor(TFT_WHITE);
  s.drawString(buf, CX, CY - 6);

  snprintf(buf, sizeof buf, "%s, %d.%d.", WEEKDAYS[now.tm_wday], now.tm_mday, now.tm_mon + 1);
  s.setFont(&fonts::FreeSans9pt7b);
  s.setTextColor(lgfx::color565(170, 170, 170));
  s.drawString(buf, CX, CY + 36);
}

void drawForecastScreen(LGFX_Sprite& s, const WeatherData& w, const String& place) {
  s.setFont(&fonts::FreeSans9pt7b);
  s.setTextDatum(textdatum_t::middle_center);
  s.setTextColor(lgfx::color565(136, 136, 136));
  s.drawString(place, CX, 30);

  drawSkyIcon(s, 84, 70, 52, w.sky);
  drawTemp(s, 152, 72, w.temp, &fonts::FreeSansBold18pt7b, TFT_WHITE);

  char buf[8];
  for (int i = 0; i < 4; ++i) {
    int x = 51 + i * 46;
    snprintf(buf, sizeof buf, "%02dh", w.next[i].hour);
    s.setFont(&fonts::FreeSans9pt7b);
    s.setTextDatum(textdatum_t::middle_center);
    s.setTextColor(lgfx::color565(136, 136, 136));
    s.drawString(buf, x, 122);
    drawSkyIcon(s, x, 152, 30, w.next[i].sky);
    drawTemp(s, x, 184, w.next[i].temp, &fonts::FreeSans9pt7b, TFT_WHITE);
  }
}

void drawSetupScreen(LGFX_Sprite& s, const char* apName) {
  s.fillScreen(TFT_BLACK);
  s.drawCircle(CX, CY, 117, lgfx::color565(2, 136, 209));
  s.drawCircle(CX, CY, 116, lgfx::color565(2, 136, 209));
  s.setTextDatum(textdatum_t::middle_center);

  s.setFont(&fonts::FreeSansBold18pt7b);
  s.setTextColor(lgfx::color565(255, 179, 0));
  s.drawString("Hallo!", CX, 58);

  s.setFont(&fonts::FreeSans9pt7b);
  s.setTextColor(TFT_WHITE);
  s.drawString("Verbinde dich mit", CX, 96);
  s.drawString("dem WLAN", CX, 116);

  s.setFont(&fonts::FreeSansBold12pt7b);
  s.setTextColor(lgfx::color565(79, 195, 247));
  s.drawString(apName, CX, 144);

  s.setFont(&fonts::FreeSans9pt7b);
  s.setTextColor(lgfx::color565(170, 170, 170));
  s.drawString("und gib WLAN und", CX, 174);
  s.drawString("Wohnort ein.", CX, 194);
}

void drawMessage(LGFX_Sprite& s, const char* title, const String& detail) {
  s.fillScreen(TFT_BLACK);
  s.setTextDatum(textdatum_t::middle_center);
  s.setFont(&fonts::FreeSansBold12pt7b);
  s.setTextColor(TFT_WHITE);
  s.drawString(title, CX, 104);
  s.setFont(&fonts::FreeSans9pt7b);
  s.setTextColor(lgfx::color565(170, 170, 170));
  s.drawString(detail, CX, 140);
}

void drawResetRing(LGFX_Sprite& s, float progress) {
  s.fillArc(CX, CY, 119, 112, -90, -90 + 360 * progress, lgfx::color565(255, 82, 82));
}

void fadeSprite(LGFX_Sprite& s, int alpha) {
  if (alpha >= 256) return;
  auto* buf = static_cast<uint16_t*>(s.getBuffer());
  size_t n = size_t(s.width()) * s.height();
  for (size_t i = 0; i < n; ++i) {
    uint16_t v = buf[i];
    if (!v) continue;
    v = uint16_t((v >> 8) | (v << 8));  // sprite stores RGB565 big-endian
    uint32_t r = (v >> 11) * alpha >> 8;
    uint32_t g = ((v >> 5) & 63) * alpha >> 8;
    uint32_t b = (v & 31) * alpha >> 8;
    v = uint16_t((r << 11) | (g << 5) | b);
    buf[i] = uint16_t((v >> 8) | (v << 8));
  }
}
