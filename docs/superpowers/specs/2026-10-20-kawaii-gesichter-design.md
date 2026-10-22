# Kawaii-Gesichter zur Auswahl – Konzept

Stand 2026-10-20. Projekt: Kulleraugen (ESP32-C3 + rundes GC9A01, 240×240).

## Warum

Der Gesichts-Screen zeigt bisher nur zwei Augen auf Schwarz und wirkt dem Nutzer zu leer.
Er wünscht sich ein richtiges, **süßes Kawaii-Anime-Gesicht**, sehr hübsch und detailliert,
und möchte zwischen mehreren Gesichtern wechseln können.

## Ziele

1. Vier neue Gesichter nach den Entwürfen in
   `.superpowers/brainstorm/1803-1792527787/content/kawaii-gesichter.html`
   (im Browser öffnen, dort sind sie animiert):
   1. **Anime-Glanzaugen**: hohe Augen mit Wimpernschwung und Wimpernspitzen außen,
      Iris oben dunkel und unten hell leuchtend, dunkler Rand, Pupille, heller Bogen
      unten in der Iris, drei Glanzpunkte (großes Oval oben links, Kreis unten rechts,
      kleiner Punkt), Schatten unter dem Oberlid, feine Augenbrauen, schraffiertes
      Erröten (rosa Verlauf plus drei schräge Striche), kleiner offener Mund mit Zunge.
      Hintergrund: dunkler Nachtblau-Verlauf.
   2. **Chibi-Kätzchen**: runde Augen mit großem schrägem Glanzoval, Kreis und Punkt,
      Lidstrich, Katzenohren oben am Rand (außen hell, innen rosa), die ab und zu
      zucken, Schnurrhaar-Striche, Erröten, rosa Dreiecksnase, ω-Mund mit kleinem Eckzahn.
      Hintergrund: fast schwarz. Blinzelt gern doppelt.
   3. **Sternenglitzer**: Augen wie 1, aber mit vierzackigem Stern als großem Glanzpunkt,
      Herz als zweitem Glanzpunkt, innerem hellem Ring in der Iris; drumherum funkelnde
      Sterne und schwebende Herzchen; breites offenes Lachen mit Zähnen und Zunge.
      Hintergrund: dunkler Lila-Verlauf.
   4. **Pastell-Träumerchen**: heller Pastellverlauf (rosa → flieder → hellblau),
      verschlafen halb geschlossene Augen mit schwerem Lid, weiches verschwommenes
      Erröten, Wellenmund, Schweißtropfen, schwebende „z“.
2. Die **bisherigen Kulleraugen bleiben als fünfte Wahl** (Gesicht 0) unverändert erhalten.
3. **BOOT doppelklicken** wechselt zum nächsten Gesicht (0 → 1 → … → 4 → 0).
4. Das gewählte Gesicht wird gespeichert und bleibt nach einem Neustart erhalten.
5. Nach einem Doppelklick ist **sofort** das neue Gesicht zu sehen, auch wenn gerade
   Uhrzeit oder Vorhersage lief; danach geht der normale Wechsel von dort weiter.
6. Jedes Gesicht zeigt die **fünf Launen** (siehe unten) auf seine eigene Art.
7. Die **Augenfarben folgen dem Wetter** (siehe Paletten), aber im Stil des Gesichts:
   gleicher Aufbau, gleiche Detailtiefe, nur andere Farbtöne.
8. Alle Gesichter leben: blinzeln, schauen umher, kleine Bewegungen wie in den Entwürfen.

## Nicht-Ziele

- Uhrzeit- und Vorhersage-Screen, Einrichtung, Wetterabruf: bleiben, wie sie sind.
- Keine Auswahl über die Einrichtungsseite; nur per Knopf.
- Kein neues Gesicht im Sci-Fi-Stil (das ist für ein anderes Display vorgesehen).

## Anti-Ziele (wäre ein Fehlschlag, auch wenn es technisch läuft)

- Die Gesichter wirken grob, flach oder billig im Vergleich zu den Entwürfen.
- Die Animation ruckelt sichtbar (Richtwert: mindestens so flüssig wie die heutigen Kulleraugen).
- Wetterfarben, die den Stil zerstören (z. B. flache Einfarb-Iris ohne Verlauf).
- Bestehende Knopf-Funktionen gehen kaputt oder lösen versehentlich aus.

## Knopf (BOOT, GPIO 9)

| Bedienung | Wirkung |
|---|---|
| Einzelner kurzer Druck | nichts (wie bisher) |
| **Doppelklick**: zwei Drücke von je < 400 ms, zweiter Druck beginnt < 400 ms nach dem Loslassen des ersten | nächstes Gesicht |
| 1,5–3 s halten, loslassen | Anzeige um 45° drehen (wie bisher) |
| ab 3 s roter Ring, 5 s | Reset (wie bisher) |

Der Doppelklick darf weder Drehen noch Reset auslösen und umgekehrt.

## Speichern

Preferences-Namespace `kuller`, Schlüssel `face` (0..4), wie `rot` für die Drehung.
Unbekannte Werte → 0.

## Launen je Gesicht

Die Laune kommt wie bisher aus `moodForSky()` bzw. „Confused“ ohne WLAN, und
Wechsel sollen weich überblenden (wie heute bei den Kulleraugen).

| Laune | Grundidee für alle | Kätzchen extra | Glitzer extra | Pastell extra |
|---|---|---|---|---|
| Happy (Sonne) | Augen weit offen, untere Lider heben sich leicht zum Lächeln, Mund offen lachend | Ohren aufgestellt | viele Sterne, Herzchen | Augen etwas offener, Schweißtropfen weg |
| Meh (Wolken) | Oberlider halb, Mund kleiner Strich/klein | Ohren leicht seitlich | wenige Sterne | Grundzustand des Entwurfs |
| Grumpy (Regen) | Augenbrauen innen nach unten, Lider schräg, Mund schmollend (umgedrehter Bogen) | Ohren nach hinten/hängend | Sterne in den Augen werden zu normalen Glanzpunkten, kein Glitzer drumherum | Mund schmollend, kein „z“ |
| Cold (Schnee) | Zittern (wie heute), Mund klappert (Zickzack), Erröten kräftiger | Ohren zittern mit | Glitzer wird zu Schneeflocken | zusätzlich leichtes Zittern |
| Confused (kein WLAN) | ein Auge etwas kleiner, unruhiger Blick, „?“ oben rechts (wie heute), Wellenmund | Kopf-/Ohr-Zucken | „?“ statt Herz | Spirale oder „?“ |

Das ist die Richtung; Details darf der Bauende sinnvoll gestalten, solange jede Laune
auf einen Blick erkennbar ist.

## Wetter-Paletten für die Iris

Jede Iris ist ein senkrechter Verlauf mit vier Stufen: oben dunkel → Mitte → hell → unten fast weiß,
plus dunkler Rand und dunkle Pupille im Ton der obersten Stufe.

| Wetter | oben | Mitte | hell | unten |
|---|---|---|---|---|
| Sonne | #3a1a05 | #b8560a | #ffb300 | #ffe9a6 |
| Wolken | #1b2630 | #4f6b7a | #9fb8c6 | #eef3f6 |
| Regen | #06213a | #0b5fa8 | #4fc3f7 | #e1f5fe |
| Schnee | #0d2f3a | #2a8fa8 | #80deea | #ffffff |

Erröten, Hintergründe und sonstige Akzentfarben bleiben je Gesicht wie im Entwurf.

## Rahmenbedingungen

- ESP32-C3, kein FPU: pro Pixel nur Ganzzahl-Rechnung (wie in `src/eyes.cpp`).
- Ein 240×240-Sprite mit 16 Bit (≈115 KB); RAM reicht nicht für einen zweiten.
- Drehung in 45°-Schritten und Überblendung (`fadeSprite`) müssen weiter funktionieren.
  `fadeSprite` überspringt reine Schwarz-Pixel; bei hellem Pastellhintergrund ist das ok,
  weil es ganze Pixel abdunkelt.
- Bibliothek: LovyanGFX (vorhanden). Keine Bilddateien/Bitmaps nötig; alles gezeichnet.

## Entscheidungen von Claude (nicht vom Nutzer)

- Jedes Gesicht in einer eigenen Datei unter `src/faces/`, gemeinsame Schnittstelle
  (z. B. `void draw(LGFX_Sprite&, uint32_t ms, const FaceState&)`), damit weitere
  Gesichter leicht dazukommen. Gemeinsame Teile (Blinzeln, Umherschauen, Launen-Überblendung,
  Iris-Verlauf aus der Wetter-Palette, Glanzpunkte, Erröten) in einem gemeinsamen Modul.
- Die heutigen Kulleraugen bleiben in `src/eyes.cpp` und werden als Gesicht 0 eingehängt.
- Doppelklick-Zeiten wie in der Knopf-Tabelle.
- Der Wechsel auf ein anderes Gesicht passiert ohne Überblendung; das neue Gesicht ist die Rückmeldung.
- Ohne Wetterdaten nutzen alle Gesichter die Wolken-Palette (wie heute die Kulleraugen).
- Gebaut wird Gesicht für Gesicht; nach jedem kann der Nutzer auf dem Gerät schauen.
- `AENDERUNGEN.md` wird fortgeschrieben.

## Dem Bauenden überlassen

- Wie die Formen gezeichnet werden (eigene Pixel-Schleifen wie in `eyes.cpp` oder
  LovyanGFX-Formen wie `fillSmoothCircle`), solange es weich (kantengeglättet) und flüssig aussieht.
- Genaue Animationszeiten, solange sie sich wie in den Entwürfen anfühlen.
- Kleine Abweichungen vom Entwurf, wenn sie auf dem echten Display besser aussehen.
