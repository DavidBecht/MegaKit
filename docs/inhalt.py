"""
inhalt.py -- Gliederung der Doku-Seite: welcher Eintrag steht in welchem Reiter.

Die Texte der einzelnen Funktionen stehen in den Headern und werden von
kopf_lesen.py geholt. Hier steht nur, wie die Seite aufgebaut ist: Reiter,
Abschnitte und die einleitenden Saetze dazu.

Jeder Eintrag eines Headers muss genau einmal vorkommen. build_docs.py bricht
ab, wenn eine Funktion fehlt oder doppelt steht; eine neue Funktion in der
Bibliothek faellt dadurch sofort auf.
"""

HEADER = {
    "aussen":  "megalib/display/display.h",
    "innen":   "megalib/display/display_draw.h",
    "sprites": "megalib/display/display_draw_sprite.h",
    "ton":     "megalib/sound/sound.h",
    "zufall":  "megalib/display/random.h",
}

# --- Startseite ------------------------------------------------------------

START = """
<p class="lead">Die <b>megalib</b> ist die C-Bibliothek der MEGACARD V6.11:
OLED-Anzeige, Zeichenfunktionen, bewegte Bilder, Ton und Zufall. Diese Seite
beschreibt jede Funktion und zeigt zu den wichtigsten ein Beispiel samt dem
Bild, das es auf dem Display erzeugt.</p>

<div class="hinweis tipp">
<b>Alle Bilder auf dieser Seite sind echt.</b> Sie entstehen nicht von Hand,
sondern aus dem daneben stehenden Programm: Es wird uebersetzt, im Simulator
ausgefuehrt und der Bildspeicher abfotografiert. Aendert sich die Bibliothek,
aendern sich die Bilder mit.
</div>

<h3>Die zwei Wege auf das Display</h3>

<p>Das ist die Verwechslung, die am meisten Zeit kostet. Es gibt zwei Schichten,
die beide Text anzeigen koennen, und sie haben verschiedene Koordinaten:</p>

<div class="vergleich">
  <table>
    <tr>
      <th></th>
      <th><code>display.h</code><br><span class="th-klein">Reiter „Ausserhalb“</span></th>
      <th><code>display_draw.h</code><br><span class="th-klein">Reiter „Innerhalb“</span></th>
    </tr>
    <tr><td>Arbeitet auf</td>
        <td>dem ganzen Display, 128 &times; 64</td>
        <td>einem selbst gewaehlten Fenster</td></tr>
    <tr><td>Koordinaten</td>
        <td>Zeichen: Spalte und Zeile</td>
        <td>Pixel, relativ zum Fenster</td></tr>
    <tr><td>Braucht SRAM</td>
        <td>nichts</td>
        <td>den Videopuffer, z.&nbsp;B. 619&nbsp;Byte bei 96&times;48</td></tr>
    <tr><td>Kann</td>
        <td>Text, einzelne Byte-Spalten</td>
        <td>Pixel, Linien, Rechtecke, Bilder, Text in acht Groessen</td></tr>
    <tr><td>Anzeigen</td>
        <td>sofort</td>
        <td>erst mit <code>display_draw_show()</code></td></tr>
    <tr><td>Gedacht fuer</td>
        <td>schnelle Textausgaben, Zahlen, Menues</td>
        <td>Spiele und Grafik</td></tr>
  </table>
</div>

<p><b>Mischen ist moeglich, aber nur mit Bedacht:</b> Wer
<code>display_draw_init()</code> verwendet, laesst die Finger von
<code>display_clear()</code> aus <code>display.h</code> — sonst ist die Anzeige
leer, der Videopuffer aber noch voll, und das naechste
<code>display_draw_show()</code> uebertraegt nur die geaenderten Spalten.
Zum Loeschen dient <code>display_draw_clear()</code>.</p>

<h3>Einbinden</h3>

<p>Die Includes werden relativ zur eigenen Datei geschrieben. Aus der
<code>main.c</code> eines Projekts aus dem MegaLib-Template:</p>

<pre class="code">#include "megalib/display/display_draw.h"
#include "megalib/display/display_draw_sprite.h"
#include "megalib/display/random.h"
#include "megalib/sound/sound.h"</pre>

<h3>Woran man denken muss</h3>

<ul class="merkliste">
<li><b>Die Zeichenflaeche kostet SRAM.</b> Der ATmega16 hat 1024 Byte fuer
Puffer, Variablen und Stack zusammen. <code>display_draw_init()</code> laesst
hoechstens <code>DISPLAY_DRAW_MAX_BYTES</code> (900) zu und meldet sich sonst
schon beim Uebersetzen.</li>
<li><b>Timer1 und Timer2 sind vergeben.</b> Die Sprites brauchen
<code>TIMER1_COMPA_vect</code>, der Ton <code>TIMER2_COMP_vect</code>. Ein
eigener Interrupt mit demselben Namen bricht mit
<code>multiple definition of '__vector_6'</code> ab. Frei sind
<code>TIMER1_COMPB_vect</code> und <code>TIMER0_COMP_vect</code>.</li>
<li><b>Der Zeichensatz kennt keine Umlaute.</b> „ZURUECK“ statt „ZURÜCK“.</li>
<li><b>Text kostet SRAM, wenn er nicht im Flash liegt.</b> Jede gewoehnliche
Zeichenkette wird beim Start ins SRAM kopiert. Die Varianten mit
<code>_P</code> und <code>PSTR()</code> vermeiden das.</li>
<li><b><code>random_init()</code> stellt den ADC um.</b> Wer das Poti liest,
ruft <code>adc_init()</code> danach auf, nicht davor.</li>
</ul>
"""

# --- Reiter ----------------------------------------------------------------

REITER = [
    {
        "id": "start",
        "titel": "Start",
        "untertitel": "Worum es geht",
        "einleitung": START,
        "abschnitte": [
            {"id": "start_beispiel", "titel": "Das kleinste Programm",
             "text": "Zeichenflaeche anlegen, etwas hineinschreiben, anzeigen. "
                     "Mehr braucht es fuer das erste Bild nicht.",
             "eintraege": []},
        ],
    },
    {
        "id": "aussen",
        "titel": "Ausserhalb",
        "untertitel": "display.h &ndash; das ganze Display, Text in Zeilen und Spalten",
        "einleitung": """
<p>Diese Schicht spricht das OLED unmittelbar an. Sie kennt keine Zeichenflaeche
und keinen Videopuffer: Jeder Aufruf geht sofort ueber den I2C-Bus zum Display.
Das kostet kein SRAM und ist der kuerzeste Weg zu einer Textausgabe.</p>
<p>Gerechnet wird in <b>Zeichen</b>, nicht in Pixel: <code>display_pos(3, 1)</code>
meint die vierte Spalte der zweiten Zeile. Wie viele das sind, haengt vom
Zeichensatz in <code>font/FontData.c</code> ab und steht in
<code>display_lines()</code> und <code>display_chars()</code>.</p>
""",
        "abschnitte": [
            {"id": "aussen_start", "titel": "Einschalten und abfragen",
             "text": "Vor allem anderen steht display_init(). Wer display_draw_init() "
                     "verwendet, braucht es nicht: Das ist dort enthalten.",
             "eintraege": ["display_init", "display_clear", "display_lines",
                           "display_chars", "display_char_first", "display_char_last"]},
            {"id": "aussen_schrift", "titel": "Schrift",
             "text": "Der Cursor merkt sich, wo es weitergeht. Entweder man setzt ihn "
                     "mit display_pos() und schreibt, oder man nimmt gleich eine der "
                     "Funktionen mit _pos im Namen.",
             "eintraege": ["display_pos", "display_char", "display_string",
                           "display_string_pos", "display_string_pos_P",
                           "display_printf", "display_printf_pos",
                           "display_printf_pos_P"]},
            {"id": "aussen_bewegen", "titel": "Bewegen",
             "text": "Das Display kann seinen Inhalt selbst verschieben, ohne dass "
                     "etwas umkopiert wird.",
             "eintraege": ["display_scroll_up", "display_scroll_down"]},
            {"id": "aussen_pixel", "titel": "Pixel direkt",
             "text": "Die unterste Ebene: acht uebereinanderliegende Pixel als ein Byte. "
                     "Darauf setzt display_draw.c auf. Fuer eigene Programme ist der "
                     "Reiter „Innerhalb“ der bequemere Weg.",
             "eintraege": ["display_pixel_byte", "display_burst_start",
                           "display_burst_write", "display_burst_end"]},
        ],
    },
    {
        "id": "innen",
        "titel": "Innerhalb",
        "untertitel": "display_draw.h &ndash; Pixel, Linien, Bilder und Schrift im Zeichenfenster",
        "einleitung": """
<p>Hier wird pixelgenau gezeichnet, und zwar in ein <b>Fenster</b>, das das
Programm zu Beginn festlegt. Alle Koordinaten sind Pixel und zaehlen ab der
linken oberen Ecke dieses Fensters, nicht ab der des Displays.</p>
<p>Gezeichnet wird zunaechst nur in den Videopuffer im SRAM. Sichtbar wird das
Bild erst mit <code>display_draw_show()</code>. Das ist kein Umstand, sondern der
Grund, warum Spiele hier nicht flackern: Ein Bild wird fertig aufgebaut und
dann in einem Zug uebertragen.</p>
""",
        "abschnitte": [
            {"id": "innen_fenster", "titel": "Das Zeichenfenster",
             "text": "Ein Aufruf zu Beginn des Programms legt Groesse und Lage fest. "
                     "Die Groesse bestimmt den SRAM-Bedarf: ((Hoehe + 7) / 8) * Breite Byte.",
             "eintraege": ["display_draw_init", "DISPLAY_DRAW_MAX_BYTES",
                           "display_draw_get_width", "display_draw_get_height",
                           "display_draw_get_x_pos", "display_draw_get_y_pos"]},
            {"id": "innen_zeigen", "titel": "Anzeigen und loeschen",
             "text": "Die haeufigste Fehlersuche im Unterricht endet hier: Es wurde "
                     "gezeichnet, aber nicht angezeigt.",
             "eintraege": ["display_draw_show", "display_draw_flush",
                           "display_draw_clear"]},
            {"id": "innen_zeichnen", "titel": "Zeichnen",
             "text": "Zu jeder Zeichenfunktion gibt es ein Gegenstueck mit "
                     "display_clear_, das genau dieselben Pixel wieder loescht.",
             "eintraege": ["display_draw_pixel", "display_clear_pixel",
                           "display_draw_line", "display_clear_line",
                           "display_draw_line_with_step", "display_clear_line_with_step",
                           "display_draw_rect", "display_clear_rect",
                           "display_draw_byte", "display_clear_byte"]},
            {"id": "innen_schrift", "titel": "Schrift",
             "text": "Derselbe Zeichensatz wie im Reiter „Ausserhalb“, aber frei "
                     "platzierbar und in acht Groessen.",
             "eintraege": ["FontSize_t", "display_draw_string_P", "display_draw_string",
                           "display_draw_char", "display_draw_zahl",
                           "display_draw_binaer", "display_draw_printf_P",
                           "display_draw_printf", "DISPLAY_DRAW_TEXT_MAX"]},
            {"id": "innen_bilder", "titel": "Bilder",
             "text": "Ein Bild liegt im Flash und kennt seine eigenen Masse. Angelegt "
                     "wird es mit einem der beiden Makros, gezeichnet mit Ort und Zeiger.",
             "eintraege": ["BITMAP_T", "BITMAP_VERTIKAL", "BITMAP_HORIZONTAL",
                           "display_draw_bitmap", "display_draw_bitmap_transparent",
                           "display_clear_bitmap"]},
        ],
    },
    {
        "id": "sprites",
        "titel": "Sprites",
        "untertitel": "display_draw_sprite.h &ndash; bewegte Bilder im Zeichenfenster",
        "einleitung": """
<p>Ein Sprite ist ein Bild, das sich von selbst bewegt und animiert. Das
Programm beschreibt es einmal — Bilder, Ort, Geschwindigkeit — und ruft danach
in der Hauptschleife nur noch <code>display_draw_sprite_update_all()</code> auf.
Loeschen, Weiterruecken, Neuzeichnen und der Bildtakt sind Sache der
Bibliothek.</p>
<p>Der Takt kommt von Timer1. Das Sprite-System setzt deshalb
<code>display_draw_init()</code> voraus und belegt
<code>TIMER1_COMPA_vect</code>.</p>
""",
        "abschnitte": [
            {"id": "sprites_takt", "titel": "Bildtakt",
             "text": "Eine Bildrate waehlen, und in der Hauptschleife Bild fuer Bild "
                     "weiterrechnen lassen.",
             "eintraege": ["display_draw_sprite_init", "SPRITE_FPS_T",
                           "display_draw_sprite_update_all"]},
            {"id": "sprites_anlegen", "titel": "Einen Sprite anlegen",
             "text": "Ein Sprite ist eine gewoehnliche Variable vom Typ SPRITE_T. "
                     "Ueblich ist die Initialisierung mit benannten Feldern.",
             "eintraege": ["SPRITE_T", "display_draw_sprite_register", "MAX_SPRITES",
                           "SPRITE_INTERN_T"]},
            {"id": "sprites_steuern", "titel": "Steuern",
             "text": "Sprites lassen sich anhalten, entfernen und an einer neuen "
                     "Stelle wieder erscheinen.",
             "eintraege": ["display_draw_sprite_activate", "display_draw_sprite_kill",
                           "display_draw_sprite_pause", "display_draw_sprite_reset"]},
            {"id": "sprites_kollision", "titel": "Kollision",
             "text": "Verglichen werden die Rechtecke, nicht die einzelnen Pixel. Das "
                     "genuegt fuer ein Spiel und kostet fast nichts.",
             "eintraege": ["display_draw_sprite_collides",
                           "display_draw_sprite_collision_point"]},
        ],
    },
    {
        "id": "ton",
        "titel": "Ton",
        "untertitel": "sound.h &ndash; Toene und Melodien am Piezo",
        "einleitung": """
<p>Der Piepser haengt an PB3. Timer0 erzeugt die Tonhoehe, Timer2 zaehlt die
Dauer ab. Eine Melodie laeuft dadurch im Hintergrund weiter, waehrend das
Programm weiterrechnet: <code>sound_melodie()</code> kehrt sofort zurueck.</p>
<p>Melodien sind Tabellen im Flash. Aus einer MIDI-Datei macht
<b>megasound</b> eine solche Tabelle.</p>
""",
        "abschnitte": [
            {"id": "ton_start", "titel": "Einschalten",
             "text": "Einmal zu Beginn. Danach laeuft die Ablaufsteuerung im "
                     "Hintergrund.",
             "eintraege": ["sound_init", "SOUND_TICK_HZ"]},
            {"id": "ton_melodie", "titel": "Melodien",
             "text": "Eine Melodie ist eine Folge von TON_T-Eintraegen im Flash. Das "
                     "Makro MELODIE() zaehlt die Eintraege selbst.",
             "eintraege": ["TON_T", "MELODIE_T", "MELODIE", "sound_melodie",
                           "SOUND_ENDLOS"]},
            {"id": "ton_einzeln", "titel": "Einzelne Toene",
             "text": "Fuer Piepser bei einem Treffer oder einem Tastendruck.",
             "eintraege": ["sound_ton"]},
            {"id": "ton_steuern", "titel": "Steuern",
             "text": "Abbrechen, stummschalten, nachfragen, ob noch etwas laeuft.",
             "eintraege": ["sound_stop", "sound_laeuft", "sound_stumm",
                           "sound_ist_stumm"]},
        ],
    },
    {
        "id": "zufall",
        "titel": "Zufall",
        "untertitel": "random.h &ndash; Zufallszahlen fuer Spiele",
        "einleitung": """
<p>Ein Mikrocontroller startet jedes Mal gleich. Ohne Zutun wuerde ein Wuerfel
nach dem Einschalten immer dieselbe Folge werfen. <code>random_init()</code>
verhindert das, indem es den Startwert aus dem Rauschen des ADC bildet.</p>
<div class="hinweis warnung"><b>Reihenfolge beachten:</b>
<code>random_init()</code> stellt den ADC fuer seine Messung um. Wer das Poti
liest, ruft seine eigene <code>adc_init()</code> danach auf.</div>
""",
        "abschnitte": [
            {"id": "zufall_alle", "titel": "Zufallszahlen",
             "text": "Einmal initialisieren, dann Zahlen holen. Die Obergrenze ist "
                     "immer ausgeschlossen: random_uint8(6) liefert 0 bis 5.",
             "eintraege": ["random_init", "random_init_fixed_seed", "random_uint8",
                           "random_uint8_range", "random_float"]},
        ],
    },
]
