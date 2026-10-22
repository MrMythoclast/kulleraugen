# Kawaii-Gesichter Implementation Plan

> **For agentic workers:** REQUIRED SUB-SKILL: Use superpowers:subagent-driven-development (recommended) or superpowers:executing-plans to implement this plan task-by-task. Steps use checkbox (`- [ ]`) syntax for tracking.

**Goal:** Vier neue, detaillierte Kawaii-Anime-Gesichter plus die alten Kulleraugen, per BOOT-Doppelklick umschaltbar, mit Wetter-Launen und Wetter-Irisfarben.

**Architecture:** Reine Logik (Knopf-Auswertung, Iris-Paletten) liegt in `lib/logic/` und wird mit Unity-Tests direkt auf dem ESP32-C3 geprüft (`pio test`; auf dem PC gibt es keinen gcc). Gesichter liegen in `src/faces/`, teilen sich Animationszustand und einen Pixel-Renderer für Anime-Augen. Ein serieller Debug-Kanal liefert Bildschirmfotos des Sprites als PNG und Bildraten, damit jedes Gesicht gegen den Browser-Entwurf geprüft werden kann.

**Tech Stack:** PlatformIO, Arduino-ESP32, LovyanGFX, Unity (pio test), Python 3 + Pillow für Fotos.

**Spec:** `docs/superpowers/specs/2026-10-20-kawaii-gesichter-design.md`
Entwürfe: `.superpowers/brainstorm/1803-1792527787/content/kawaii-gesichter.html`

## Global Constraints

- ESP32-C3 ohne FPU: pro Pixel nur Ganzzahl-Rechnung; Floats nur pro Bild.
- Genau ein 240×240-Sprite (16 Bit). Kein zweiter Bildpuffer.
- Drehung (BOOT 1,5–3 s), Reset-Ring ab 3 s, Reset bei 5 s und `fadeSprite` funktionieren unverändert weiter.
- Doppelklick: zwei Drücke von je < 400 ms, zweiter Druck beginnt < 400 ms nach dem Loslassen des ersten.
- Preferences `kuller`/`face` (0..4), unbekannte Werte → 0.
- Iris-Paletten (oben, Mitte, hell, unten), Stufen bei 0 / 0,45 / 0,8 / 1:
  Sonne `#3a1a05 #b8560a #ffb300 #ffe9a6` · Wolken `#1b2630 #4f6b7a #9fb8c6 #eef3f6` ·
  Regen `#06213a #0b5fa8 #4fc3f7 #e1f5fe` · Schnee `#0d2f3a #2a8fa8 #80deea #ffffff`.
  Ohne Wetterdaten: Wolken.
- Gesichtswechsel ohne Überblendung; neues Gesicht sofort sichtbar.
- Bildrate jedes neuen Gesichts ≥ Bildrate der Kulleraugen (in Task 1 gemessen) minus 10 %.
- Kein git im Ordner: statt Commit wird am Ende jeder Task `AENDERUNGEN.md` ergänzt.
- `pio test` überschreibt die Firmware auf dem Gerät; am Ende jeder Task wieder `pio run -t upload --upload-port COM23`.
  Vor jedem Upload prüfen, dass COM23 die Kulleraugen sind (Flash enthält „Kulleraugen“), siehe `AENDERUNGEN.md`.

## Review Focus

1. Doppelklick, bei dem der zweite Druck lang gehalten wird (> 1,5 s): darf weder Gesicht wechseln noch beides auslösen; es gilt nur Drehen bzw. Reset. → Test `test_double_then_hold_rotates_only` (Task 2).
2. Drei schnelle Klicks: genau ein Wechsel, der dritte Klick beginnt eine neue Zählung. → Test `test_triple_click_one_switch` (Task 2).
3. Doppelklick während Uhrzeit/Vorhersage läuft: Gesicht sofort und mit voller Helligkeit (kein Einblenden aus Schwarz). → Prüfschritt in Task 3.
4. Pastell-Gesicht bei 45°-Drehung und beim Überblenden: heller Hintergrund darf keine Ecken/Streifen zeigen. → Prüfschritt in Task 7.
5. Laune wechselt mitten im Blinzeln oder Gesicht wechselt mitten in einer Laune: keine Sprünge, kein Absturz. → Prüfschritt in Task 3 (Debug-Befehle schnell hintereinander).

---

### Task 1: Debug-Kanal (Foto + Bildrate) und Logik-Bibliothek

**Files:**
- Create: `src/debug_tools.h`, `src/debug_tools.cpp`, `tools/snapshot.py`
- Create: `lib/logic/sky.h` (enthält `enum class Sky : uint8_t { Sun, Cloud, Rain, Snow };`)
- Modify: `src/weather.h` (Sky-Definition entfernen, `#include <sky.h>`)
- Modify: `src/main.cpp` (`debugPoll()` und `debugFrameDone()` in `loop()`)

**Interfaces:**
- Produces:
  - `void debugPoll(LGFX_Sprite& s);` liest Serial-Befehle, Zeile für Zeile:
    `s` = Foto, `f` = nächstes Gesicht, `m0`..`m4` = Laune erzwingen (Reihenfolge wie `Mood`), `w0`..`w3` = Wetter erzwingen, `x` = Erzwingen aus, `p` = Bildschirm auf Gesicht festhalten an/aus.
  - `bool debugForcedMood(Mood& out); bool debugForcedSky(Sky& out); bool debugHoldFace(); bool debugTakeNextFace();`
  - `void debugFrameDone(uint32_t ms);` gibt alle 5 s `FPS <zahl>` aus.
  - Fotoformat: Zeile `SNAP 240 240`, dann 115200 Byte Sprite-Puffer roh (RGB565, wie im Sprite gespeichert: big-endian), dann Zeile `END`.
  - `python tools/snapshot.py COM23 out.png` sendet `s`, liest das Foto, schreibt PNG (Pillow).

- [ ] **Step 1:** Implementieren; Foto wird vor `show()` aus dem Sprite gelesen (also ungedreht).
- [ ] **Step 2:** `pio run` → SUCCESS; Upload auf COM23.
- [ ] **Step 3:** `python tools/snapshot.py COM23 %SCRATCH%/kulleraugen.png` → PNG zeigt die Kulleraugen; Bild mit Read ansehen.
- [ ] **Step 4:** Seriellen Monitor 15 s lesen → drei `FPS`-Zeilen. Wert als **Basis-FPS** in `AENDERUNGEN.md` notieren.
- [ ] **Step 5:** `AENDERUNGEN.md` ergänzen.

### Task 2: Knopf-Auswertung mit Tests

**Files:**
- Create: `lib/logic/boot_button.h`, `lib/logic/boot_button.cpp`
- Create: `test/test_button/test_main.cpp`
- Modify: `src/main.cpp` (`checkResetButton` ersetzt durch `BootButton`)

**Interfaces:**
- Produces:
```cpp
enum class ButtonEvent : uint8_t { None, DoubleClick, Rotate, Reset };
class BootButton {
 public:
  ButtonEvent update(bool down, uint32_t ms);  // jedes Bild aufrufen
  bool held(uint32_t ms, uint32_t& heldMs) const;  // für den Reset-Ring
};
constexpr uint32_t CLICK_MAX_MS = 400, CLICK_GAP_MS = 400;
constexpr uint32_t ROTATE_MIN_MS = 1500, ROTATE_MAX_MS = 3000, RESET_HOLD_MS = 5000;
```
  `Reset` kommt, sobald 5 s gehalten (noch während gedrückt). `Rotate` beim Loslassen nach 1,5–3 s.
  Drücke < 30 ms werden als Prellen ignoriert.

- [ ] **Step 1: Tests schreiben** (Hilfsfunktion `press(btn, t, dauer)` simuliert Drücken/Loslassen in 10-ms-Schritten und sammelt Events):
  - `test_single_click_nothing`: Druck 100 ms, dann 1000 ms nichts → keine Events.
  - `test_double_click`: 100 ms drücken, 200 ms Pause, 100 ms drücken → genau ein `DoubleClick`.
  - `test_gap_too_long`: Pause 500 ms → keine Events.
  - `test_second_press_too_long`: zweiter Druck 600 ms → keine Events.
  - `test_triple_click_one_switch`: drei Klicks à 100 ms, Pausen 200 ms → genau ein `DoubleClick`.
  - `test_hold_rotates`: 2000 ms halten → genau ein `Rotate`.
  - `test_hold_too_long_no_rotate`: 3500 ms halten → keine Events.
  - `test_reset_at_5s`: 5000 ms halten → `Reset` bei t ≥ 5000, kein `Rotate`.
  - `test_double_then_hold_rotates_only`: Klick 100 ms, Pause 200 ms, dann 2000 ms halten → genau ein `Rotate`, kein `DoubleClick`.
  - `test_bounce_ignored`: 10 ms Druck, 200 ms Pause, Klick 100 ms → keine Events.
- [ ] **Step 2:** `pio test -e esp32c3 --filter test_button --upload-port COM23 --test-port COM23` → FAIL (Klasse fehlt).
- [ ] **Step 3:** `BootButton` implementieren.
- [ ] **Step 4:** Gleicher Befehl → alle Tests PASS.
- [ ] **Step 5:** In `main.cpp` einbauen: `Rotate` → `rotateDisplay()`, `Reset` → bisheriger Reset-Ablauf, Ring ab 3 s aus `held()`. `DoubleClick` vorerst nur `Serial.println("double")`.
- [ ] **Step 6:** `pio run -t upload`, am Gerät: 2 s halten dreht; Doppelklick schreibt `double`; 5 s Reset **nicht** ausprobieren (würde WLAN löschen), nur Ring ab 3 s sehen.
- [ ] **Step 7:** `AENDERUNGEN.md` ergänzen.

### Task 3: Gesichter-Gerüst, Paletten, Auswahl und Speichern

**Files:**
- Create: `lib/logic/iris_palette.h`, `lib/logic/iris_palette.cpp`, `test/test_palette/test_main.cpp`
- Create: `src/faces/faces.h`, `src/faces/faces.cpp` (Auswahl + Verteilung), `src/faces/anim.h`, `src/faces/anim.cpp` (gemeinsamer Zustand), `src/faces/anime_eye.h`, `src/faces/anime_eye.cpp` (Renderer)
- Modify: `src/main.cpp`, `README.md`

**Interfaces:**
- Consumes: `Sky` (`lib/logic/sky.h`), `Mood` + `eyesSetMood/eyesSetPalette/eyesDraw` (`src/eyes.h`), `ButtonEvent::DoubleClick` (Task 2), `debugForced*/debugTakeNextFace/debugHoldFace` (Task 1).
- Produces:
```cpp
// iris_palette.h
struct Rgb8 { uint8_t r, g, b; };
struct IrisPalette { Rgb8 top, mid, light, bottom; };
IrisPalette irisPalette(Sky sky);                 // Werte aus Global Constraints
void buildIrisLut(Sky sky, Rgb8* lut, int n);     // senkrechter Verlauf, Stufen 0/.45/.8/1; lut[0]=top, lut[n-1]=bottom
uint8_t clampFace(int32_t stored, uint8_t count); // außerhalb 0..count-1 → 0

// anim.h
struct FaceState {
  Mood mood; Sky sky; uint32_t ms; float dt;
  float top, tilt, bottom, shiver, asym;  // weich überblendete Lid-Werte wie LidParams in eyes.cpp
  float lookX, lookY;                     // -1..1
  float blink;                            // 0..1
  float moodMix[5];                       // Anteil jeder Laune 0..1, Summe 1, weich überblendet
};
void animUpdate(uint32_t ms, Mood mood, Sky sky);
const FaceState& animState();

// faces.h
constexpr uint8_t FACE_COUNT = 5;          // 0 Kulleraugen, 1 Glanz, 2 Katze, 3 Glitzer, 4 Pastell
void facesSetMood(Mood m);
void facesSetPalette(Sky sky);
void facesDraw(LGFX_Sprite& s, uint32_t ms, uint8_t face);

// anime_eye.h – Pixel-Renderer, Ganzzahl, kantengeglättet (Vorbild: drawEye in eyes.cpp)
enum class Highlight : uint8_t { Oval, Star, Heart };
struct AnimeEyeSpec {
  int16_t cx, cy;                 // Augenmitte
  int16_t rx, ry;                 // Augenweiß-Halbachsen
  int16_t irisRx, irisRy, pupilRx, pupilRy;
  Highlight main;                 // großer Glanzpunkt oben links
  bool innerRing;                 // heller Ring in der Iris (Glitzer)
  uint8_t lashWidth;              // Oberlid-Strich, 0 = keiner
  uint8_t lashFlicks;             // Wimpernspitzen außen
  Rgb8 lashColor;
  float heavyLid;                 // zusätzliches Schließen (Pastell), 0..1
};
void drawAnimeEye(LGFX_Sprite& s, const AnimeEyeSpec& e, const FaceState& st, int side /* -1 links, +1 rechts */);
void drawBlush(LGFX_Sprite& s, int cx, int cy, int rx, int ry, bool hatching);
void drawSparkle(LGFX_Sprite& s, int cx, int cy, int r, uint16_t col);  // vierzackiger Stern
void drawHeart(LGFX_Sprite& s, int cx, int cy, int size, uint16_t col);
```
  Die Iris im Renderer nutzt `buildIrisLut(st.sky, …)` (Cache, neu nur bei Wetterwechsel).

- [ ] **Step 1: Tests schreiben** `test/test_palette/test_main.cpp`:
  - `test_sun_palette`: `irisPalette(Sky::Sun).top == {0x3a,0x1a,0x05}`, `.bottom == {0xff,0xe9,0xa6}`; ebenso je eine Prüfung für Cloud, Rain, Snow (alle vier Stufen aus den Global Constraints).
  - `test_lut_ends`: `buildIrisLut(Sky::Rain, lut, 64)` → `lut[0]` = top, `lut[63]` = bottom.
  - `test_lut_mid_stop`: Index bei 0,45·63 (gerundet) liegt höchstens ±3 je Kanal neben `mid`.
  - `test_clamp_face`: `clampFace(3,5)==3`, `clampFace(5,5)==0`, `clampFace(-1,5)==0`, `clampFace(255,5)==0`.
- [ ] **Step 2:** `pio test -e esp32c3 --filter test_palette …` → FAIL.
- [ ] **Step 3:** `iris_palette` implementieren → PASS.
- [ ] **Step 4:** `anim`, `faces`, `anime_eye` implementieren. Gesichter 1–4 zeichnen vorerst ein Platzhalter-Paar `drawAnimeEye` mit Standardwerten, damit das Umschalten testbar ist. `facesDraw(…, 0)` ruft die bestehenden `eyes*`-Funktionen.
- [ ] **Step 5:** `main.cpp`: `face` aus Preferences laden (`clampFace`), `DoubleClick` oder `debugTakeNextFace()` → `face = (face+1) % FACE_COUNT`, speichern, `cycleStart = ms - 333` (Gesichts-Segment beginnt, schon voll hell). `pickScreen(ms - cycleStart, …)`. `debugHoldFace()` hält das Gesicht dauerhaft. Erzwungene Laune/Wetter aus Task 1 überschreiben die echten Werte.
- [ ] **Step 6: Prüfen am Gerät:** Upload. Doppelklick während Uhrzeit läuft → sofort Gesicht, nicht schwarz eingeblendet (Review Focus 3). Fünf Doppelklicks → wieder Kulleraugen. Neustart → zuletzt gewähltes Gesicht. Über Serial schnell `m0 m2 f m4 f w3` nacheinander → kein Absturz, keine Sprünge (Review Focus 5).
- [ ] **Step 7:** README um „Gesicht wechseln: BOOT doppelklicken“ ergänzen; `AENDERUNGEN.md` ergänzen.

### Task 4: Gesicht 1 – Anime-Glanzaugen

**Files:** Create `src/faces/face_glanz.cpp`; Modify `src/faces/faces.cpp` (eintragen)

**Interfaces:** Consumes `FaceState`, `AnimeEyeSpec`, `drawAnimeEye`, `drawBlush` (Task 3). Produces `void drawFaceGlanz(LGFX_Sprite&, const FaceState&);`

Vorlage: Karte 1 im Entwurf (Augen bei x 68/172, y 112; Weiß 30×37; Iris 24×31; Pupille 11×15; Wimpernstrich 6 px mit 2 Spitzen; Brauen; Erröten mit Schraffur bei y 160; offener Mund mit Zunge bei y 166–182; Hintergrund Verlauf `#1b1830` → `#05040a`). Launen nach Spec-Tabelle, Spalte „Grundidee“.

- [ ] **Step 1:** Implementieren.
- [ ] **Step 2:** Upload; `p` (festhalten), `f` bis Gesicht 1; für `w0` mit `m0`..`m4` und dann `w1`..`w3` mit `m1` je ein Foto über `tools/snapshot.py` in den Scratchpad.
- [ ] **Step 3:** Fotos mit Read ansehen und mit Karte 1 im Entwurf vergleichen: alle drei Glanzpunkte, Wimpernspitzen, Schraffur, Zunge erkennbar; jede Laune auf einen Blick unterscheidbar; Iris-Verlauf in Wetterfarbe oben dunkel → unten hell. Abweichungen beheben, erneut fotografieren.
- [ ] **Step 4:** FPS ablesen → ≥ 90 % der Basis-FPS. Sonst optimieren (z. B. Bounding-Box je Auge, LUT statt Rechnen).
- [ ] **Step 5:** Fotos dem Nutzer zeigen und fragen, ob es auf dem Display so passt; `AENDERUNGEN.md` ergänzen.

### Task 5: Gesicht 2 – Chibi-Kätzchen

**Files:** Create `src/faces/face_katze.cpp`; Modify `src/faces/faces.cpp`

**Interfaces:** wie Task 4; Produces `void drawFaceKatze(LGFX_Sprite&, const FaceState&);`

Vorlage: Karte 2 (runde Augen r 34 bei x 70/170, y 114; Iris r 28; Pupille r 14; schräges Glanzoval; Ohren als Dreiecke oben am Rand, außen `#f3e3d6`, innen `#ffb3c7`, zucken alle ~3,8 s; Schnurrhaar-Striche; Nase `#ff8fab`; ω-Mund mit Eckzahn; Hintergrund `#07060c`; blinzelt gern doppelt). Launen: Ohren laut Spec-Tabelle (aufgestellt / seitlich / hängend / zitternd / zuckend).

- [ ] **Step 1–5:** wie Task 4 (Fotos aller Launen und Wetter, Vergleich mit Karte 2, FPS ≥ 90 %, Nutzer zeigen, `AENDERUNGEN.md`).

### Task 6: Gesicht 3 – Sternenglitzer

**Files:** Create `src/faces/face_glitzer.cpp`; Modify `src/faces/faces.cpp`

**Interfaces:** wie Task 4; Produces `void drawFaceGlitzer(LGFX_Sprite&, const FaceState&);`; nutzt `drawSparkle`, `drawHeart`, `Highlight::Star`, `innerRing = true`.

Vorlage: Karte 3 (Augen wie Glanz, Stern-Glanzpunkt, Herz als zweiter Glanzpunkt, Iris-Innenring; 4 funkelnde Sterne am Rand, 2 schwebende Herzen; breites Lachen mit Zähnen und Zunge; Hintergrund Verlauf `#2a1338` → `#050208`). Launen: Grumpy ohne Glitzer und mit normalem Glanzpunkt, Cold mit Schneeflocken statt Sternen, Confused mit „?“ statt Herz.

- [ ] **Step 1–5:** wie Task 4 mit Karte 3.

### Task 7: Gesicht 4 – Pastell-Träumerchen

**Files:** Create `src/faces/face_pastell.cpp`; Modify `src/faces/faces.cpp`

**Interfaces:** wie Task 4; Produces `void drawFacePastell(LGFX_Sprite&, const FaceState&);`; nutzt `heavyLid`.

Vorlage: Karte 4 (Hintergrund senkrechter Verlauf `#ffd9ec` → `#e9d8ff` → `#cfe9ff`, ein `fillRect` je Zeile; helle Blasen; schweres Oberlid in Hintergrundfarbe; weiches Erröten; Wellenmund `#6b3a5c`; Schweißtropfen; schwebende „z“). Launen: Happy offener und ohne Tropfen, Grumpy schmollend ohne „z“, Cold zittert, Confused mit „?“.

- [ ] **Step 1–4:** wie Task 4 mit Karte 4.
- [ ] **Step 5: Review Focus 4:** Gerät auf 45° drehen (2 s halten) und Foto des Displays vom Nutzer erbitten bzw. Nutzer schauen lassen: keine hellen Ecken oder Streifen am Rand; Übergang zu Uhrzeit-Screen blendet sauber über. Bei Problemen: Hintergrund nur innerhalb des Kreises r 120 zeichnen.
- [ ] **Step 6:** Nutzer zeigen; `AENDERUNGEN.md` ergänzen.

### Task 8: Abschluss

**Files:** Modify `README.md`, `AENDERUNGEN.md`, Memory `kulleraugen-aenderungen.md`

- [ ] **Step 1:** `pio test -e esp32c3 --upload-port COM23 --test-port COM23` → alle Tests PASS.
- [ ] **Step 2:** Prüfen, dass COM23 die Kulleraugen sind; `pio run -t upload --upload-port COM23` → SUCCESS.
- [ ] **Step 3:** README: Tabelle der Knopf-Bedienung (Doppelklick, 2 s, 5 s) und Liste der Gesichter.
- [ ] **Step 4:** `AENDERUNGEN.md` und Memory auf den Endstand bringen (inkl. gemessener FPS je Gesicht).
