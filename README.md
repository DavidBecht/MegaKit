# MegaKit

Software für die **MEGACARD V6.11** der HTL Rankweil (ATmega16, 12 MHz, OLED-Anzeige 128 × 64, Piezo, 4 Taster, 8 LEDs).

| Teil | Wozu |
|---|---|
| **megalib** | C-Bibliothek für die MEGACARD: Anzeige, Zeichnen, Bitmaps, animierte Sprites, Ton, Zufall |
| **megasim** | Simulator für den PC. Startet ein Microchip-Studio-Projekt ohne Hardware, mit Anzeige, LEDs, Tastern und Ton |
| **megasound** | Wandelt MIDI-Dateien in Tontabellen für die megalib um |
| **Template** | Projektvorlage für Microchip Studio mit eingebauter megalib |

Die fertigen Programme gibt es unter **[Releases](../../releases)**:
- `megasim.exe`
- `megasound.exe`
- `Template_HTL_Rankweil_MegaLib_V<version>.zip`

Wer nur damit arbeiten will, braucht nichts selbst zu bauen.

**Alle Funktionen der megalib mit Beispielen und Bildern:
[davidbecht.github.io/MegaKit](https://davidbecht.github.io/MegaKit/)**

---

## Inhalt

1. [Schnellstart für Schülerinnen und Schüler](#1-schnellstart)
2. [Das Template in Microchip Studio](#2-das-template-in-microchip-studio)
3. [megasim: der Simulator](#3-megasim-der-simulator)
4. [megalib: die Bibliothek](#4-megalib-die-bibliothek)
5. [megasound: Melodien aus MIDI-Dateien](#5-megasound-melodien-aus-midi-dateien)
6. [Selbst bauen](#6-selbst-bauen)
7. [Ein Release erstellen](#7-ein-release-erstellen)
8. [Aufbau des Repositorys](#8-aufbau-des-repositorys)

---

## 1. Schnellstart

1. Aus dem neuesten Release `megasim.exe` und das Template-ZIP herunterladen.
2. Das Template installieren, siehe [Abschnitt 2](#2-das-template-in-microchip-studio).
3. Den Simulator in Microchip Studio einrichten, siehe [Abschnitt 3](#3-megasim-der-simulator).
4. In Microchip Studio **Datei → Neu → Projekt → „MegaLib … (HTL Rankweil)“** wählen.
5. Das Projekt bauen (F7), dann **Tools → megasim** starten. Die Anzeige zeigt „Hallo MEGACARD“.
6. Auf die echte Hardware kommt das Programm wie gewohnt über den Programmer.

---

## 2. Das Template in Microchip Studio

### Installieren

Die ZIP-Datei **nicht entpacken**, sondern so, wie sie ist, in diesen Ordner kopieren:

```
Dokumente\Atmel Studio\7.0\Templates\ProjectTemplates\
```

Microchip Studio danach neu starten.

### Neues Projekt anlegen

**Datei → Neu → Projekt**, dann in der Liste **„MegaLib … (HTL Rankweil)“** wählen, einen Namen vergeben und OK klicken.

Das neue Projekt enthält:

```
MeinProjekt/
  main.c              Startprogramm: Zeichenfläche, Text, Taster abfragen
  megalib/
    display/          Anzeige, Zeichnen, Sprites, Schrift, Zufall
    sound/            Töne und Melodien
```

Controller (ATmega16), Taktfrequenz (`F_CPU=12000000UL`) und Optimierung sind bereits eingestellt. Die Startdatei `main.c` ist als Ausgangspunkt gedacht und darf frei geändert werden.

### Dateien einbinden

Includes werden **relativ** zur eigenen Datei geschrieben. Aus `main.c` heraus:

```c
#include "megalib/display/display_draw.h"
#include "megalib/sound/sound.h"
```

Aus einer Datei in einem Unterordner, etwa `spiel/spiel.c`:

```c
#include "../megalib/display/display_draw.h"
```

---

## 3. megasim: der Simulator

megasim übersetzt das Projekt für den PC und zeigt die MEGACARD in einem Fenster. Die Hardware wird nicht nachgebildet: Register wie `PORTC` oder `TCCR0` sind Variablen, die der Simulator ausliest. Deshalb ist megasim schnell, bildet aber nicht jedes Zeitverhalten des echten Controllers nach.

### Einrichten in Microchip Studio

1. `megasim.exe` an einen festen Ort legen, zum Beispiel `C:\Tools\megasim\megasim.exe`.
2. **Tools → External Tools… → Add**:

   | Feld | Wert |
   |---|---|
   | Title | `megasim` |
   | Command | `C:\Tools\megasim\megasim.exe` |
   | Arguments | `"$(ProjectDir)."` |
   | Initial directory | leer lassen |
   | **Use Output window** | anhaken |

3. Mit OK bestätigen. Der Simulator steht jetzt im Menü **Tools**.

Beim Start schreibt megasim zuerst seine Version ins Ausgabefenster (`[sim] megasim 1.0.0`), danach die Meldungen beim Übersetzen. Fehler im C-Code stehen ebenfalls dort, mit Datei und Zeile.

### Erster Start

Beim allerersten Start richtet megasim seinen eingebauten C-Compiler ein. Das dauert einige Sekunden, ein Ladebalken zeigt den Fortschritt. Der Compiler landet in `%LOCALAPPDATA%\megasim`. Jeder weitere Start dauert etwa zwei Sekunden, ohne Änderungen am Code noch weniger.

### Bedienung

| Taste | Wirkung |
|---|---|
| `1` `2` `3` `4` | Taster S0 S1 S2 S3 |
| Pfeil rechts, links, unten, oben | S0, S1, S2, S3 (wie auf der Platine angeordnet) |
| `R` oder Knopf RESET | Neustart, wie der Reset-Taster |
| `Q` oder `Esc` | Beenden |
| Mausrad, `+` / `-`, Klick auf den Poti-Balken | Poti an ADC5 verstellen |

Unter der Anzeige zeigt das Fenster die LEDs an `PORTC`, die Taster, die Bildrate und darunter die Stellung des Potis.

### Was der Simulator anzeigt

- **Zeichenfläche:** Die mit `display_draw_init` gewählte Fläche ist leicht heller hinterlegt.
- **Orange Pixel** liegen außerhalb der Zeichenfläche. Das deutet auf einen Fehler im Programm hin. Die Position steht zusätzlich im Ausgabefenster.
- **LEDs:** Hellrot heißt eingeschaltet und als Ausgang konfiguriert. Dunkelrot heißt, der Pin ist Eingang mit Pull-up; auf der Platine würde die LED dann nur schwach leuchten.
- **Ton:** Der Piezo ist über die Lautsprecher des PCs zu hören.
- **EEPROM:** Der Inhalt bleibt erhalten und liegt im Projektordner in `megacard.eep`.
- **ADC:** Kanal 5 liefert die Stellung des Potis, die übrigen Kanäle 0. Einzelwandlung, Interrupt `ADC_vect` und Freilauf werden nachgebildet.
- **Timer:** Compare- und Überlauf-Interrupts von Timer0 und Timer2 sowie `TIMER1_COMPA_vect` und `TIMER1_COMPB_vect` kommen im Mittel im richtigen Takt, auch bei 1 ms.

### Grenzen

- Es ist nur das erlaubt, was es auch auf dem AVR gibt. `#include <windows.h>` oder `<conio.h>` meldet einen Fehler, genau wie beim Bauen für die Platine. Ebenso verlangt `_delay_ms()` wie beim AVR einen konstanten Wert.
- Zeitverhalten wie Pausen und Timerfrequenzen ist nachgebildet, aber nicht taktgenau. Windows weckt den Simulator nur etwa alle 15 ms; schnelle Timer-Interrupts kommen deshalb gebündelt, im Mittel aber richtig.
- Nicht nachgebildet sind externe Interrupts (INT0 bis INT2), UART und SPI. Programme damit bauen im Simulator nicht.
- PWM auf den LEDs ist nur als Flackern zu sehen, die Helligkeit erst auf der Platine.
- Klangeigenheiten des echten Piezos sind nicht zu hören, etwa ein Pfeifen bei bestimmten Frequenzen.

### Aufruf ohne Microchip Studio

```
megasim.exe "C:\Pfad\zum\Projekt"
megasim.exe "C:\Pfad\zum\Projekt" --scale 6 --fps 30
megasim.exe --version
```

---

## 4. megalib: die Bibliothek

Die Bibliothek besteht aus Schichten, die jeweils für sich nutzbar sind:

| Datei | Aufgabe |
|---|---|
| `display/twi/twi_soft.c` | I2C-Verbindung zur Anzeige (PB0 = SCL, PB1 = SDA) |
| `display/display.c` | Text in Zeilen und Spalten, direkter Zugriff auf die Anzeige |
| `display/display_draw.c` | Pixel, Linien, Rechtecke, Text in beliebiger Größe, Bitmaps |
| `display/display_draw_sprite.c` | Animierte Figuren mit Bewegung, Kollision und Neustart |
| `sound/sound.c` | Töne und Melodien am Piezo (PB3), läuft im Hintergrund |
| `display/random.c` | Zufallszahlen |

Die genaue Beschreibung jeder Funktion steht in der jeweiligen `.h`-Datei.

### Die Funktionsübersicht im Netz

Dieselben Beschreibungen gibt es als Webseite, nach Themen sortiert und mit
Beispielen:

**[davidbecht.github.io/MegaKit](https://davidbecht.github.io/MegaKit/)**

| Reiter | Inhalt |
|---|---|
| **Start** | Wann `display.h`, wann `display_draw.h`, und woran man denken muss |
| **Außerhalb** | `display.h`: Text in Zeilen und Spalten auf dem ganzen Display |
| **Innerhalb** | `display_draw.h`: Pixel, Linien, Bilder und Schrift im Zeichenfenster |
| **Sprites** | `display_draw_sprite.h`: bewegte Bilder |
| **Ton** | `sound.h`: Töne und Melodien |
| **Zufall** | `random.h` |

Neben jedem Beispiel steht das Bild, das es auf dem Display erzeugt. Diese
Bilder sind nicht gezeichnet, sondern aufgenommen: Das Beispiel wird
übersetzt, im Simulator ausgeführt und sein Bildspeicher gespeichert. Die
Texte selbst stammen aus den Kommentaren in den `.h`-Dateien, die Seite kann
also nicht veralten. Die Suche oben (Taste `/`) findet jede Funktion.

### Zeichnen

```c
#include "megalib/display/display_draw.h"

enum { BREITE = 96, HOEHE = 48 };           // Werte muessen beim Uebersetzen feststehen

int main(void)
{
    display_draw_init(BREITE, HOEHE, 16, 8); // Flaeche 96x48, ab Pixel (16, 8)

    display_draw_rect(0, 0, BREITE, HOEHE, false);
    display_draw_line(0, 0, BREITE - 1, HOEHE - 1);
    display_draw_string_P(6, 10, PSTR("Hallo"), FONT_SIZE_2X);
    display_draw_show();                     // erst jetzt erscheint alles

    while (1) { }
}
```

Wichtig:
- **`display_draw_init` einmal aufrufen**, mit festen Zahlen. Die Bibliothek legt daraus den Bildspeicher an: Breite × Höhe / 8 Byte. Mehr als 900 Byte sind nicht erlaubt, weil der ATmega16 nur 1024 Byte RAM hat. Die ganze Anzeige mit 128 × 64 Pixeln passt deshalb nicht. Ein zu großer Wert wird schon beim Übersetzen gemeldet.
- **Koordinaten gelten relativ zur Zeichenfläche**, nicht zur ganzen Anzeige.
- **Zeichnen allein ändert die Anzeige nicht.** Erst `display_draw_show()` überträgt die Änderungen.
- Zu jeder Zeichenfunktion gibt es ein Gegenstück zum Löschen, zum Beispiel `display_clear_rect`.
- Texte aus dem Flash stehen in `PSTR("...")` und werden mit den Funktionen mit `_P` am Ende ausgegeben.

### Text und Zahlen

| Funktion | wofür | Flash |
|---|---|---|
| `display_draw_string_P(x, y, PSTR("TEXT"), groesse)` | fester Text aus dem Flash | – |
| `display_draw_string(x, y, text, groesse)` | Text aus dem RAM | – |
| `display_draw_zahl(x, y, wert, groesse)` | eine Zahl von 0 bis 65535 | + 0,15 KB |
| `display_draw_binaer(x, y, wert, groesse)` | ein Byte als acht Nullen und Einsen | + 0,2 KB |
| `display_draw_printf_P(x, y, groesse, PSTR("PUNKTE %u"), punkte)` | Text und Werte gemischt | + 1,6 KB |
| `display_draw_printf(x, y, groesse, format, ...)` | dasselbe, Format aus dem RAM | + 1,6 KB |

Die Angaben gelten nur für Programme, die die Funktion auch verwenden; ungenutzte wirft der Linker hinaus. `display_draw_printf` legt seinen Zwischenpuffer auf dem Stack an (`DISPLAY_DRAW_TEXT_MAX`, Vorgabe 24 Zeichen) und schneidet längere Ausgaben ab. Fließkommaformate wie `%f` sind in der Standardeinstellung von Microchip Studio nicht enthalten.

### Bitmaps

Ein Bild ist eine Tabelle im Flash und wird mit einem Makro beschrieben. Das Makro prüft beim Übersetzen, ob die Tabelle zur angegebenen Größe passt. Es muss außerhalb von Funktionen stehen.

```c
static const uint8_t Herz_daten[] PROGMEM = { 0x0C, 0x1E, 0x3E, 0x7C, 0x3E, 0x1E, 0x0C, 0x00 };
BITMAP_VERTIKAL(Herz, Herz_daten, 8, 8);

display_draw_bitmap(10, 10, &Herz);           // immer mit &
display_draw_bitmap_transparent(20, 10, &Herz);
display_clear_bitmap(10, 10, &Herz);
```

### Sprites

Ein Sprite ist eine Figur aus mehreren Bildern, die sich selbst animiert und bewegt.

```c
#include "megalib/display/display_draw_sprite.h"

const BITMAP_T * const Vogel_bilder[] PROGMEM = { &Vogel_1, &Vogel_2 };

static SPRITE_T vogel = {
    .x = 10, .y = 10,
    .vx = 1,                      // Pixel je Bild nach rechts
    .frames = Vogel_bilder,
    .frame_count = 2,
    .animation_ms = 400,          // Dauer aller Bilder zusammen
};

display_draw_sprite_init(FPS_30);
display_draw_sprite_register(&vogel);
while (1)
{
    display_draw_sprite_update_all(true);   // wartet auf das naechste Bild und zeichnet
}
```

Bis zu 8 Sprites gleichzeitig. `width` und `height` füllt die Bibliothek selbst aus. Kollisionen prüft `display_draw_sprite_collides(&a, &b)`.

### Ton

```c
#include "megalib/sound/sound.h"

sound_init();
sound_ton(141, 3, 50);                        // ein Ton, 50 Ticks zu 10 ms
sound_melodie(&KnightRider, SOUND_ENDLOS);    // Melodie im Hintergrund
sound_stumm(true);                            // leise schalten, Melodie laeuft weiter
```

Melodien erzeugt man mit megasound, siehe [Abschnitt 5](#5-megasound-melodien-aus-midi-dateien).

### Belegung der Hardware

| Hardware | Pin / Einheit | Verwendet von |
|---|---|---|
| Anzeige (I2C) | PB0, PB1 | `twi_soft.c` |
| Piezo | PB3 (OC0), Timer0 | `sound.c` |
| Ablaufsteuerung Ton | Timer2, 100 Hz | `sound.c` |
| Bildtakt Sprites | Timer1 | `display_draw_sprite.c` |
| Taster S0–S3 | PA0–PA3, gedrückt = 0 | eigenes Programm |
| LEDs | PORTC | eigenes Programm |
| Poti | PA5 / ADC5, Jumper X14 „VANA_on“ | eigenes Programm |

Wer Ton oder Sprites nutzt, darf die jeweiligen Timer nicht selbst verwenden. `sound_init()` und `display_draw_sprite_init()` schalten die Interrupts ein.

**Interrupt-Vektoren:** Die Interrupt-Routinen `TIMER1_COMPA_vect` (Sprites) und `TIMER2_COMP_vect` (Ton) stehen in **jedem** Projekt aus dem Template, auch wenn Ton und Sprites gar nicht verwendet werden: Jede `.c`-Datei des Projekts wird mitgelinkt. Eine eigene `ISR` mit demselben Namen ergibt den Linkerfehler `multiple definition of '__vector_…'`. Frei sind zum Beispiel `TIMER1_COMPB_vect` (Timer1 im CTC-Modus bis `OCR1A`, Interrupt über `OCR1B`, wenn keine Sprites laufen) und `TIMER0_COMP_vect` (wenn kein Ton läuft).

Nach `random_init()` ist der ADC für das Rauschen umgestellt. Wer das Poti liest, initialisiert den ADC erst **danach**.

---

## 5. megasound: Melodien aus MIDI-Dateien

megasound wandelt eine Stimme aus einer MIDI-Datei in eine C-Datei um, die die megalib abspielt. Der Piezo kann nur einen Ton gleichzeitig spielen. megasound wählt deshalb eine Spur aus und nimmt bei Akkorden den höchsten Ton.

### Verwenden

1. `megasound.exe` starten. Liegt eine `.mid`-Datei im selben Ordner, wird sie gleich geladen.
2. **Datei wählen …** und eine MIDI-Datei öffnen. Die Tabelle zeigt alle Spuren mit Instrument, Tonumfang und Länge.
3. Eine **Spur anklicken**. Die Einstellung „ab Takt“ springt automatisch an den Anfang der Spur, und unten erscheint die Vorschau des C-Codes.
4. Den Ausschnitt festlegen:
   - **ab Takt:** erster Takt
   - **Anzahl Takte:** Länge; 0 bedeutet bis zum Ende
   - **Arrayname:** Name der Melodie im Programm, zum Beispiel `Titelmelodie`
   - **Tickrate Hz** und **F_CPU** passen zur megalib und bleiben normalerweise, wie sie sind.
5. Mit **Abspielen** klingt der Ausschnitt so, wie er später am Piezo klingt. **Stopp** beendet die Wiedergabe.
6. **C-Datei speichern** und die Datei ins Projekt legen.

### Im Programm verwenden

1. Die gespeicherte Datei, etwa `titel_melodie.c`, in Microchip Studio zum Projekt hinzufügen: **Rechtsklick aufs Projekt → Hinzufügen → Vorhandenes Element**.
2. In der Datei die Include-Zeile an den Ort anpassen. Liegt die Datei im Projektordner, lautet sie:
   ```c
   #include "megalib/sound/sound.h"
   ```
3. Im Programm die Melodie bekannt machen und abspielen:
   ```c
   #include "megalib/sound/sound.h"

   extern const MELODIE_T Titelmelodie PROGMEM;

   int main(void)
   {
       sound_init();
       sound_melodie(&Titelmelodie, SOUND_ENDLOS);   // oder eine Anzahl, z.B. 1
       while (1) { }
   }
   ```

Die Länge der Melodie zählt der Compiler selbst. Die Tabelle landet im Flash und belegt kein RAM.

### Kommandozeile

```
megasound.exe --info  lied.mid
megasound.exe --gen   lied.mid --spur 5 --takt 9 --takte 8 --name Titelmelodie --aus titel_melodie.c
megasound.exe --version
```

---

## 6. Selbst bauen

Die fertigen Dateien baut normalerweise GitHub bei jedem Release, siehe [Abschnitt 7](#7-ein-release-erstellen). Lokal geht es so:

### Voraussetzungen

- Windows
- Python 3.12 mit `pip install pygame pyinstaller`
- Für die megalib-Tests: eine MinGW-GCC; alternativ reicht die mitgelieferte in `megasim/gcc_minimal`
- Für das Template mit Probebau: `avr-gcc` im PATH, etwa aus Microchip Studio unter `C:\Program Files (x86)\Atmel\Studio\7.0\toolchain\avr8\avr8-gnu-toolchain\bin`

### megasound.exe

```
cd megasound
python build.py                 # Ergebnis: megasound/dist/megasound.exe
python build.py --clean         # vorher Zwischenstand loeschen
python build.py --bump patch    # Version erhoehen und bauen
```

Die Version steht als `VERSION = '…'` am Anfang von `megasound.py`. Sie erscheint im Fenster, in den erzeugten C-Dateien und in den Dateieigenschaften der exe. Ohne Build lässt sich megasound direkt mit `python megasound.py` starten.

### megasim.exe

```
cd megasim
python build.py                 # Ergebnis: megasim/dist_onefile/megasim.exe
python build.py --clean
python build.py --bump patch
```

Die Version steht in `sim.py`. Während der Entwicklung lässt sich der Simulator direkt starten: `python sim.py ..\megalib`. Dafür wird eine MinGW-GCC unter `C:\msys64\mingw64\bin` erwartet, oder man setzt die Umgebungsvariable `MEGACARD_GCC`.

Der mitgelieferte Compiler liegt in `megasim/gcc_minimal`. Er ist ein ausgedünnter Auszug aus WinLibs-GCC 16.1 und enthält nur die C-Header, die es auch auf dem AVR gibt. Neu zusammenstellen, zum Beispiel für eine neuere GCC:

```
cd megasim
powershell -ExecutionPolicy Bypass -File install_gcc.ps1   # laedt WinLibs nach megasim/winlibs
python build.py --gcc-neu
```

`winlibs/` kann danach wieder gelöscht werden.

### Template

```
python template/build_template.py --version 1.2.0 --pruefen
# Ergebnis: template/dist/Template_HTL_Rankweil_MegaLib_V1.2.0.zip
```

Das Skript nimmt die Bibliothek aus `megalib/megalib/`, die Startdatei `template/main.c` und die Compiler-Einstellungen aus `megalib/MegaLib.cproj`. Mit `--pruefen` wird das fertige Template zusätzlich mit `avr-gcc` für den ATmega16 übersetzt.

### Doku-Seite

```
python docs/build_docs.py                    # Seite nach docs/site/ bauen
python docs/build_docs.py --pruefen          # jedes Beispiel mit avr-gcc uebersetzen
python docs/build_docs.py --bilder           # Bilder der Beispiele neu aufnehmen
python docs/build_docs.py --bilder display_draw_rect   # nur eines davon
```

`docs/site/index.html` lässt sich direkt im Browser öffnen, auch ohne Server.
Auf GitHub baut der Workflow `pages.yml` die Seite bei jeder Änderung an der
Bibliothek neu und veröffentlicht sie.

**Neue Funktion in der Bibliothek?** Der Kommentarblock im Header genügt, sie
erscheint dann von selbst. Nur eintragen, in welchen Abschnitt sie gehört:
`docs/inhalt.py`. Fehlt der Eintrag, bricht der Bau mit einer Meldung ab —
so bleibt keine Funktion unerwähnt.

**Neues Beispiel?** Eine `.c`-Datei nach `docs/beispiele/` legen, mit drei
Zeilen im Kopf:

```c
// beispiel: display_draw_rect      Eintrag oder Abschnitt, wo es hingehoert
// titel: Rahmen und Fuellung
// bild: 1.2s                       Zeitpunkt der Aufnahme, weglassen = kein Bild
```

Danach `--bilder` laufen lassen (braucht Windows und den Simulator) und die
neue PNG-Datei mit einchecken. `--pruefen` übersetzt jedes Beispiel für den
ATmega16, ein Beispiel mit Fehler fällt also auf.

### megalib und Demos

`megalib/MegaLib.atsln` in Microchip Studio öffnen. In `megalib/main.c` wählt die Zeile `#define PROGRAMM` eine der Demos:

| Demo | Inhalt |
|---|---|
| `LAMA_DEMO` | Sprite-Animation |
| `TRAFFIC_RACER` | Spiel mit Sprites, Kollision, Musik und Highscore im EEPROM |
| `SELFTEST` | Selbsttest aller Zeichenfunktionen, weiter mit S0 |
| `GRAFIK_DEMO` | Nachthimmel mit Gondel, alle Zeichenfunktionen |
| `TONTEST` | Töne einzeln zum Anhören am Piezo |

Die Tests der Bibliothek laufen ohne Hardware:

```
python megasim/tests/run_tests.py
```

---

## 7. Ein Release erstellen

Ein Release besteht aus `megasim.exe`, `megasound.exe` und dem Template-ZIP. Gebaut wird es von GitHub Actions (`.github/workflows/release.yml`), sobald ein Tag `vX.Y.Z` gepusht wird. Das erledigt das Skript `release.py`:

```
python release.py            # naechste Patch-Version, z.B. 1.0.0 -> 1.0.1
python release.py minor      # 1.0.1 -> 1.1.0
python release.py major      # 1.1.0 -> 2.0.0
python release.py 1.5.0      # feste Version
python release.py --probe    # nur pruefen und anzeigen
```

Das Skript
1. bricht ab, wenn das Repository nicht sauber ist, also Dateien geändert oder neu sind, wenn man nicht auf `main` ist oder wenn `main` nicht auf dem Stand von GitHub ist;
2. berechnet die neue Version aus dem letzten Tag; beim ersten Release gilt die Version aus den Programmen;
3. setzt `VERSION` in `megasim/sim.py` und `megasound/megasound.py` und committet das;
4. pusht Commit und Tag gemeinsam.

Danach im Tab **Actions** zusehen. Nach etwa 10 Minuten steht das Release unter **Releases**.

**Was der Workflow sonst noch tut:** Bei jedem Push auf `main` und bei Pull Requests laufen die Tests und alle Builds ohne Release. Die Ergebnisse liegen als Artefakte am jeweiligen Lauf. Erst ein Tag erzeugt ein Release.

**Wenn ein Job fehlschlägt,** entsteht kein Release. Nach der Korrektur den Tag löschen und neu setzen:

```
git tag -d v1.1.0
git push origin :refs/tags/v1.1.0
git tag -a v1.1.0 -m "MegaKit v1.1.0"
git push origin v1.1.0
```

Meldet der Release-Schritt „Resource not accessible by integration“, unter **Settings → Actions → General → Workflow permissions** „Read and write permissions“ einstellen.

---

## 8. Aufbau des Repositorys

```
megalib/                   Microchip-Studio-Projekt: Bibliothek und Demos
  megalib/                 die Bibliothek (kommt ins Template)
    display/
    sound/
  demos/
  main.c                   Auswahl der Demo
  MegaLib.atsln / .cproj
megasim/                   Simulator
  sim.py                   Programm
  build.py                 baut megasim.exe
  fake_avr/, fake_src/     AVR-Header und Register fuer den PC
  gcc_minimal/             mitgelieferter C-Compiler
  install_gcc.ps1          laedt die volle WinLibs-GCC (nur fuer --gcc-neu)
  tests/                   Tests der megalib
megasound/
  megasound.py             Programm
  build.py                 baut megasound.exe
template/
  main.c                   Startdatei des Templates
  build_template.py        baut das Template-ZIP
docs/                      Doku-Seite (GitHub Pages)
  build_docs.py            baut die Seite
  kopf_lesen.py            liest die Kommentarbloecke der Header
  inhalt.py                Reiter, Abschnitte, Einleitungen
  vorlage.py               Geruest, Aussehen, Verhalten der Seite
  beispiele/               Beispielprogramme
  bilder/                  deren Display-Bilder aus dem Simulator
release.py                 Version erhoehen und Release anstossen
.github/workflows/         Build und Release auf GitHub
```
