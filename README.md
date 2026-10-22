<p align="center">
  <img src="docs/banner.svg" alt="Kulleraugen" width="760">
</p>

<p align="center">
  <img src="https://img.shields.io/badge/ESP32--C3-Arduino-blue?logo=espressif&logoColor=white" alt="ESP32-C3">
  <img src="https://img.shields.io/badge/PlatformIO-Projekt-orange?logo=platformio&logoColor=white" alt="PlatformIO">
  <img src="https://img.shields.io/badge/Display-GC9A01%20240%C3%97240-ff69b4" alt="GC9A01">
  <img src="https://img.shields.io/badge/Lizenz-MIT-green" alt="MIT">
</p>

Ein rundes GC9A01-Display am ESP32-C3 wird zu einem Gesicht mit großen Kulleraugen.
**Die Laune zeigt das Wetter.** Zwischendurch blendet es Uhrzeit und Vorhersage ein.

| Wetter | Laune |
|---|---|
| ☀️ Sonne | fröhlich |
| ☁️ Wolken | gelangweilt |
| 🌧️ Regen | grimmig |
| ❄️ Schnee | zittert vor Kälte |
| 📡 kein WLAN | verwirrt |

## Gesichter

Per Doppelklick auf den BOOT-Knopf wechselst du zwischen fünf Gesichtern.
Alle zeigen die Wetter-Laune, die Irisfarbe folgt dem Wetter.

| Nr. | Gesicht |
|---|---|
| 0 | **Kulleraugen** – das ursprüngliche Gesicht |
| 1 | **Anime-Glanzaugen** |
| 2 | **Chibi-Kätzchen** |
| 3 | **Sternenglitzer** |
| 4 | **Pastell-Träumerchen** |

## Hardware

- ESP32-C3 (z. B. ESP32-C3 DevKitM-1 oder SuperMini)
- Rundes GC9A01-Display, 240 × 240 Pixel, SPI

| Display | ESP32-C3 |
|---|---|
| SCL | GPIO 4 |
| SDA | GPIO 10 |
| DC | GPIO 2 |
| CS | GPIO 7 |
| RST | GPIO 3 |
| VCC / GND | 3,3 V / GND |

## Aufspielen

Mit [PlatformIO](https://platformio.org/):

```
pio run -t upload
```

Benutzte Bibliotheken: [LovyanGFX](https://github.com/lovyan03/LovyanGFX),
[WiFiManager](https://github.com/tzapu/WiFiManager), [ArduinoJson](https://arduinojson.org/).
PlatformIO lädt sie automatisch.

## Erster Start

1. Das Display zeigt „Hallo!“ und öffnet das WLAN **Kulleraugen**.
2. Mit dem Handy verbinden; die Einrichtungsseite öffnet sich (sonst `192.168.4.1` aufrufen).
3. WLAN auswählen, Passwort und Wohnort (PLZ, Stadt oder ganze Adresse) eintragen, speichern.
4. Wird der Ort nicht gefunden, kommt die Einrichtung noch einmal.

Das Wetter kommt von [Open-Meteo](https://open-meteo.com/), der Ort wird über
[OpenStreetMap Nominatim](https://nominatim.org/) gesucht. Ein API-Schlüssel ist nicht nötig.

## BOOT-Knopf

| Bedienung | Wirkung |
|---|---|
| Doppelklick | nächstes Gesicht |
| 1,5–3 s halten, loslassen | Anzeige um 45° im Uhrzeigersinn drehen (acht Stufen) |
| 5 s halten (roter Ring ab 3 s) | Einstellungen löschen, Einrichtung startet neu |

Gesicht und Drehung bleiben nach einem Neustart erhalten.

## Für Entwickler

### Fotos und Bildrate

`python tools/snapshot.py COM23 bild.png [befehle]` holt ein Bild aus dem Gerät
(braucht `pyserial` und `Pillow`).

| Befehl | Wirkung |
|---|---|
| `f` | nächstes Gesicht |
| `m0` … `m4` | Laune erzwingen (fröhlich, gelangweilt, grimmig, frierend, verwirrt) |
| `w0` … `w3` | Wetter erzwingen (Sonne, Wolken, Regen, Schnee) |
| `x` | zurück zu echtem Wetter |
| `p` | Gesicht festhalten (keine Uhrzeit-/Vorhersage-Seiten) |

Alle 5 s kommt `FPS n` über die serielle Schnittstelle.

### Aufbau

```
src/            Hauptprogramm, Display, Wetter, Bildschirme
src/faces/      die Gesichter und ihr gemeinsamer Augen-Renderer
lib/logic/      hardwareunabhängige Logik (Knopf, Farbpaletten)
test/           Unity-Tests für lib/logic
tools/          snapshot.py
docs/           Konzept und Umsetzungsplan der Kawaii-Gesichter
```

## Fehlersuche

- **Display bleibt nach dem Aufspielen schwarz und es kommen keine `FPS`-Zeilen:**
  Der Chip steht im Download-Modus. Ab- und wieder anstecken, ohne BOOT zu drücken.
- **Farben sehen falsch aus:** In `src/display.h` `cfg.invert` bzw. `cfg.rgb_order` umdrehen.

## Lizenz

[MIT](LICENSE)
