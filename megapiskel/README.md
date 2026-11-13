# MegaPiskel

Browser-Erweiterung für [Piskel](https://www.piskelapp.com), die ein gezeichnetes Sprite direkt als
C-Code für die MEGACARD ausgibt und umgekehrt bestehende Bitmaps aus C-Code in Piskel lädt.
Der Umweg über PNG und image2cpp entfällt.

Auf piskelapp.com erscheinen oben rechts zwei Buttons:

- **⬇ MegaCard C Export**: aktuellen Frame als `BITMAP_HORIZONTAL`- oder `BITMAP_VERTIKAL`-Tabelle ausgeben
- **⬆ Import**: eine Bitmap aus C-Code in Piskel laden, um sie weiterzubearbeiten

**Farben wie auf dem Display:** Helle Pixel sind an, dunkle und transparente aus. Am einfachsten
mit Weiß auf schwarzem oder transparentem Hintergrund zeichnen. Importierte Bitmaps erscheinen
genauso: weiße Pixel auf schwarzem Hintergrund.

## Installation

MegaPiskel ist nicht im Chrome Web Store, sondern wird als entpackte Erweiterung geladen.
Das funktioniert in Chrome, Edge und anderen Chromium-Browsern.

1. Aus dem neuesten [Release](https://github.com/DavidBecht/MegaKit/releases/latest) die Datei
   `MegaPiskel_V….zip` herunterladen und an einem festen Ort entpacken, z. B. nach `C:\Tools`.
   Dabei entsteht der Ordner `MegaPiskel`. Der Browser lädt die Dateien bei jedem Start von dort,
   der Ordner darf danach nicht verschoben oder gelöscht werden.
2. Die Erweiterungsseite öffnen:
   - Chrome: `chrome://extensions`
   - Edge: `edge://extensions`
3. **Entwicklermodus** einschalten (Chrome: Schalter oben rechts, Edge: links in der Seitenleiste).
4. **Entpackte Erweiterung laden** anklicken und den entpackten Ordner `MegaPiskel` auswählen
   (den Ordner, in dem `manifest.json` liegt).
5. Die Erweiterung erscheint in der Liste. Ist piskelapp.com schon offen, die Seite neu laden.

**Neue Version:** Das neue ZIP am selben Ort entpacken und die alten Dateien überschreiben.
Dann auf der Erweiterungsseite beim Eintrag den Pfeil ↻ anklicken und piskelapp.com neu laden.
Die Versionsnummer dort zeigt, welcher Stand geladen ist.

Wer im MegaKit-Repository arbeitet, kann statt des ZIP auch direkt den Ordner `megapiskel/`
laden. Änderungen an den Dateien wirken dann nach ↻ sofort.

## Export: Piskel → C-Code

1. Auf <https://www.piskelapp.com> **Create Sprite** wählen.
2. Die Größe einstellen (rechts **Resize**), z. B. 8×8 oder 16×16 Pixel.
3. Als Zeichenfarbe **Weiß** wählen (Piskel startet mit Schwarz) und zeichnen. Pixel mit mehr
   als halber Helligkeit werden auf dem Display eingeschaltet, dunkle und transparente bleiben aus.
4. Den Frame auswählen, der exportiert werden soll.
5. Oben rechts **⬇ MegaCard C Export** klicken.
6. Im Feld **Name der Bitmap** den Namen eintragen, z. B. `Bombe`.
7. Das **Format** wählen, siehe [Horizontal oder vertikal](#horizontal-oder-vertikal).
   Der Code darunter ändert sich bei jeder Eingabe mit.
8. Kopieren und den Code in die eigene `.c`-Datei einfügen, **außerhalb jeder Funktion**.

Zum Kopieren gibt es drei Wege:

| | kopiert |
|---|---|
| **Kopieren** (oder Enter im Namensfeld) | den ganzen Code: Kommentar, Tabelle und Makro |
| **Nur Bytes kopieren** | nur die Hex-Zeilen, z. B. um die Zahlen einer bestehenden Tabelle zu ersetzen |
| Markieren mit der Maus, **Strg+C** | genau den markierten Teil |

Esc schließt das Fenster.

Beispiel für ein 8×8-Sprite mit dem Namen `Bombe`:

```c
// 'Bombe', 8x8px
static const uint8_t Bombe_daten[] PROGMEM = {
	0x3c, 0x7e, 0xdb, 0xff, 0xff, 0xdb, 0x66, 0x3c
};
BITMAP_HORIZONTAL(Bombe, Bombe_daten, 8, 8);
```

### Name der Bitmap

- **Vorschlag:** Hat das Sprite in Piskel einen Namen (beim Speichern vergeben), wird dieser
  vorgeschlagen. Sonst der zuletzt kopierte Name, beim allerersten Mal `Mein_Bild`.
- **Gültiger C-Name:** Leerzeichen und Sonderzeichen werden zu `_`, Umlaute zu `ae`, `oe`, `ue`,
  `ss`. Eine Ziffer am Anfang bekommt ein `_` davor. Weicht der Name im Code von der Eingabe ab,
  zeigt das Fenster daneben `→` und den tatsächlich verwendeten Namen.
- Jede Bitmap im Programm braucht einen eigenen Namen.

### Horizontal oder vertikal

Die megalib kennt zwei Ablagen. Beide zeichnen dasselbe Bild, das Makro im Code muss nur zur
Tabelle passen. MegaPiskel schreibt immer das passende Makro dazu und merkt sich die Wahl.

| | `BITMAP_HORIZONTAL` | `BITMAP_VERTIKAL` |
|---|---|---|
| ein Byte sind | 8 Pixel nebeneinander, höchstes Bit links | 8 Pixel übereinander, niedrigstes Bit oben |
| Größe der Tabelle | aufgerundete Breite / 8 × Höhe | Breite × aufgerundete Höhe / 8 |
| Zeichnen | langsamer, jeder Pixel wird umgerechnet | schneller, entspricht dem Speicher des Displays |
| passt zu | Bildern mit einer Breite in 8er-Schritten | Bildern mit einer Höhe in 8er-Schritten |

Für Sprites, die in jedem Bild neu gezeichnet werden, ist **vertikal** die bessere Wahl.

### Im Programm verwenden

```c
#include "../megalib/display/display_draw.h"   // relativer Pfad je nach Ort der Datei

// ... hier der eingefügte Code ...

display_draw_bitmap(20, 10, &Bombe);            // immer die Adresse übergeben
display_draw_show();
```

Als Frame eines Sprites:

```c
const BITMAP_T * const Bombe_frames[] PROGMEM = { &Bombe };
SPRITE_T bombe_sprite = { .frames = Bombe_frames, .frame_count = 1, .animation_ms = 0 };
```

### Animationen

MegaPiskel exportiert immer nur den **gerade ausgewählten Frame**. Für eine Animation jeden
Frame einzeln auswählen und exportieren, im Namensfeld durchnummeriert (`Auto_1`, `Auto_2`, …),
dann alle in die Frame-Liste des Sprites eintragen:

```c
const BITMAP_T * const Auto_frames[] PROGMEM = { &Auto_1, &Auto_2 };
SPRITE_T auto_sprite = { .frames = Auto_frames, .frame_count = 2, .animation_ms = 400 };
```

## Import: C-Code → Piskel

Damit lassen sich vorhandene Bitmaps, z. B. aus einer Demo oder von image2cpp, in Piskel
weiterbearbeiten.

1. In Piskel ein Sprite öffnen (**Create Sprite**) und den Frame auswählen, in den die Bitmap soll.
2. Oben rechts **⬆ Import** klicken.
3. Den C-Code in das Textfeld einfügen. Erkannt werden:
   - `BITMAP_VERTIKAL(Name, Tabelle, Breite, Höhe)` bzw. `BITMAP_HORIZONTAL(…)`: Name, Größe und Format
   - der Kommentar `// 'Name', 16x16px` von MegaPiskel und image2cpp: Name und Größe
   - die Zahlen der ersten Tabelle in `{ … }`, hexadezimal (`0x3c`), binär (`0b0011`) oder dezimal
4. Fehlt etwas, z. B. bei reinen Bytes ohne Makro, **Größe** und **Format** von Hand eintragen.
   Die Vorschau zeigt sofort, ob das Bild stimmt. Ein verzerrtes Bild heißt fast immer:
   falsches Format oder falsche Breite.
5. **Importieren** klicken.

Was beim Import passiert:

- **Gleiche Größe wie das Sprite in Piskel:** Der aktuelle Frame der aktuellen Ebene wird
  überschrieben.
- **Andere Größe:** MegaPiskel fragt nach und legt dann ein neues Sprite in der Größe der Bitmap an.
  Das bisherige Sprite wird dabei ersetzt.
- Gesetzte Pixel werden weiß, alle übrigen schwarz, wie auf dem Display. Ein erneuter Export
  ergibt wieder genau dieselben Bytes.
- **Strg+Z** in Piskel macht den Import rückgängig.

Für Animationen jedes Bild einzeln importieren: in Piskel einen neuen Frame anlegen, auswählen,
importieren.

## Hinweise

- **Ebenen:** Beim Export zählt an jeder Stelle die oberste Ebene, die dort nicht transparent ist.
  Ein schwarzer Pixel in einer oberen Ebene verdeckt also einen weißen darunter.
- **Auffüllen:** Horizontal wird jede Zeile, vertikal jede 8er-Gruppe von Zeilen auf ganze Bytes
  aufgefüllt. Ein 10 Pixel breites Bild braucht horizontal 2 Byte pro Zeile, 6 Bit bleiben ungenutzt.
- **Größe:** Das Display hat 128×64 Pixel. Die Bitmap muss in das Zeichenfenster aus
  `display_draw_init()` passen.
- Die Tabelle landet mit `PROGMEM` im Flash und kostet kein RAM.
- MegaPiskel arbeitet nur im Browser und sendet nichts ins Internet.

## Fehlersuche

| Problem | Lösung |
|---|---|
| Keine Buttons auf piskelapp.com | Seite neu laden. Prüfen, ob die Erweiterung auf der Erweiterungsseite eingeschaltet ist. |
| Meldung „Kein Response“ | Seite neu laden. Hilft das nicht: mit F12 die Konsole öffnen und die Zeilen mit `[MegaCard]` lesen. |
| Export ergibt nur `0x00` | Mit Schwarz gezeichnet. Dunkle Pixel sind aus, mit Weiß zeichnen. |
| Meldung „Controller nicht gefunden“ | Es ist noch kein Sprite offen. Zuerst **Create Sprite** wählen. |
| Bild auf dem Display ist Pixelsalat | Das Makro passt nicht zur Tabelle (`VERTIKAL` statt `HORIZONTAL` oder umgekehrt), oder `&` vor dem Namen fehlt. Den Code immer komplett mit Makro kopieren. |
| `Groesse der Tabelle passt nicht zu Breite und Hoehe` | Tabelle und Makro stammen aus verschiedenen Exporten. Neu exportieren und komplett kopieren. |
| `redefinition of 'Bombe'` oder `multiple definition of 'Bombe'` | Zwei Bitmaps heißen gleich. Jede braucht einen eigenen Namen. |
| Import: „… Byte nötig“ | Anzahl der Bytes passt nicht zu Größe und Format. Größe und Format prüfen, oder es wurde nicht die ganze Tabelle eingefügt. |

## Dateien

| Datei | Aufgabe |
|---|---|
| `manifest.json` | Beschreibung der Erweiterung (Manifest V3), gilt nur für piskelapp.com |
| `content-main.js` | Läuft im Seitenkontext von Piskel: liest die Pixel des aktuellen Frames und schreibt importierte Bitmaps |
| `content-ui.js` | Buttons, Export- und Import-Fenster, Umwandlung zwischen Pixeln und Bytes, Lesen von C-Code |
| `build_zip.py` | Packt das Release-ZIP `dist/MegaPiskel_V<version>.zip`, nicht Teil der Erweiterung |

Das ZIP baut der Workflow bei jedem Release. Lokal: `python megapiskel/build_zip.py`. Die Version
im Dateinamen kommt aus `manifest.json`, sie wird bei jeder Änderung der Erweiterung erhöht.
