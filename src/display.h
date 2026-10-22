#pragma once

#define LGFX_USE_V1
#include <LovyanGFX.hpp>

// GC9A01 240x240 round display, wired as:
// SCL=GPIO5, SDA=GPIO6, DC=GPIO2, CS=GPIO7, RST=GPIO3
class LGFX : public lgfx::LGFX_Device {
  lgfx::Panel_GC9A01 _panel;
  lgfx::Bus_SPI _bus;

public:
  LGFX() {
    {
      auto cfg = _bus.config();
      cfg.spi_host = SPI2_HOST;
      cfg.spi_mode = 0;
      cfg.freq_write = 40000000;  // 80 MHz leaves the panel blank on these pins
      cfg.freq_read = 16000000;
      cfg.spi_3wire = true;
      cfg.use_lock = true;
      cfg.dma_channel = SPI_DMA_CH_AUTO;
      cfg.pin_sclk = 5;
      cfg.pin_mosi = 6;
      cfg.pin_miso = -1;
      cfg.pin_dc = 2;
      _bus.config(cfg);
      _panel.setBus(&_bus);
    }
    {
      auto cfg = _panel.config();
      cfg.pin_cs = 7;
      cfg.pin_rst = 3;
      cfg.pin_busy = -1;
      cfg.panel_width = 240;
      cfg.panel_height = 240;
      cfg.offset_x = 0;
      cfg.offset_y = 0;
      cfg.offset_rotation = 0;
      cfg.readable = false;
      cfg.invert = true;      // flip if colors look inverted
      cfg.rgb_order = false;  // flip if red and blue are swapped
      cfg.dlen_16bit = false;
      cfg.bus_shared = false;
      _panel.config(cfg);
    }
    setPanel(&_panel);
  }
};

extern LGFX tft;
extern LGFX_Sprite spr;
