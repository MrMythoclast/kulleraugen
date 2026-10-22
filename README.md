<p align="center">
  <img src="docs/img/banner.png" alt="Kulleraugen – a weather face with big round eyes" width="900">
</p>

<p align="center"><b>English</b> · <a href="README.de.md">Deutsch</a></p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--C3-Arduino-blue?logo=espressif&logoColor=white" alt="ESP32-C3">
  <img src="https://img.shields.io/badge/PlatformIO-project-orange?logo=platformio&logoColor=white" alt="PlatformIO">
  <img src="https://img.shields.io/badge/Display-GC9A01%20240%C3%97240-ff69b4" alt="GC9A01">
  <img src="https://img.shields.io/badge/License-MIT-green" alt="MIT">
</p>

**Kulleraugen** (German for "big round goggle eyes") turns a round GC9A01 display on an ESP32-C3
into a face with big, expressive eyes. **Its mood shows the weather outside.** In between, it shows
the time and the forecast.

Every image on this page is a real capture taken straight from the device's frame buffer.

## Moods

The weather sets the mood, and the iris colour follows the weather. Shown here on the Anime Sparkle Eyes:

<table>
  <tr>
    <td align="center"><img src="docs/img/mood0.png" width="150"><br><b>☀️ Sun</b><br>happy</td>
    <td align="center"><img src="docs/img/mood1.png" width="150"><br><b>☁️ Clouds</b><br>bored</td>
    <td align="center"><img src="docs/img/mood2.png" width="150"><br><b>🌧️ Rain</b><br>grumpy</td>
    <td align="center"><img src="docs/img/mood3.png" width="150"><br><b>❄️ Snow</b><br>shivering</td>
    <td align="center"><img src="docs/img/mood4.png" width="150"><br><b>📡 No Wi-Fi</b><br>confused</td>
  </tr>
</table>

## Faces

Double-click the BOOT button to switch between five faces. All of them show the weather mood.

<table>
  <tr>
    <td align="center"><img src="docs/img/face0.png" width="150"><br><b>0 · Kulleraugen</b><br>the original<br>face</td>
    <td align="center"><img src="docs/img/face1.png" width="150"><br><b>1 · Anime Sparkle Eyes</b><br>big highlights<br>and lashes</td>
    <td align="center"><img src="docs/img/face2.png" width="150"><br><b>2 · Chibi Kitten</b><br>ears, whiskers<br>and a cat mouth</td>
    <td align="center"><img src="docs/img/face3.png" width="150"><br><b>3 · Starry Glitter</b><br>star eyes, sparkles<br>and little hearts</td>
    <td align="center"><img src="docs/img/face4.png" width="150"><br><b>4 · Pastel Dreamer</b><br>soft pastel<br>gradient</td>
  </tr>
</table>

## Hardware

- ESP32-C3 (e.g. ESP32-C3 DevKitM-1 or SuperMini)
- Round GC9A01 display, 240 × 240 pixels, SPI

| Display | ESP32-C3 |
|---|---|
| SCL | GPIO 4 |
| SDA | GPIO 10 |
| DC | GPIO 2 |
| CS | GPIO 7 |
| RST | GPIO 3 |
| VCC / GND | 3.3 V / GND |

## Flashing

With [PlatformIO](https://platformio.org/):

```
pio run -t upload
```

Libraries used: [LovyanGFX](https://github.com/lovyan03/LovyanGFX),
[WiFiManager](https://github.com/tzapu/WiFiManager), [ArduinoJson](https://arduinojson.org/).
PlatformIO downloads them automatically.

## First start

The on-device texts and the setup page are in German.

1. The display shows "Hallo!" and opens the Wi-Fi network **Kulleraugen**.
2. Connect with your phone. The setup page opens by itself (otherwise go to `192.168.4.1`).
3. Pick your Wi-Fi, enter the password and your location ("Wohnort": postcode, city or full address), then save.
4. If the location can't be found, setup starts again.

Weather data comes from [Open-Meteo](https://open-meteo.com/), the location is looked up with
[OpenStreetMap Nominatim](https://nominatim.org/). No API key needed.

## BOOT button

| Action | Effect |
|---|---|
| Double-click | next face |
| Hold 1.5–3 s, release | rotate the display 45° clockwise (eight steps) |
| Hold 5 s (red ring from 3 s) | erase settings and restart setup |

Face and rotation are kept across restarts.

## For developers

### Screenshots and frame rate

`python tools/snapshot.py COM23 image.png [commands]` grabs an image from the device
(needs `pyserial` and `Pillow`).

| Command | Effect |
|---|---|
| `f` | next face |
| `m0` … `m4` | force a mood (happy, bored, grumpy, cold, confused) |
| `w0` … `w3` | force the weather (sun, clouds, rain, snow) |
| `x` | back to the real weather |
| `p` | hold the face (no time/forecast pages) |

Every 5 s the device prints `FPS n` on the serial port.

### Layout

```
src/            main program, display, weather, screens
src/faces/      the faces and their shared eye renderer
lib/logic/      hardware-independent logic (button, colour palettes)
test/           Unity tests for lib/logic
tools/          snapshot.py
docs/           design notes, implementation plan (German) and images
```

## Troubleshooting

- **Display stays black after flashing and no `FPS` lines appear:**
  The chip is stuck in download mode. Unplug and plug it back in without pressing BOOT.
- **Colours look wrong:** flip `cfg.invert` or `cfg.rgb_order` in `src/display.h`.

## License

[MIT](LICENSE)
