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

<h3>Beides zusammen verwenden</h3>

<p>Ein Zeichenfenster, daneben Punktestand oder Menue aus
<code>display.h</code>: Das ist eine gute Aufteilung, denn der Text kostet
kein zusaetzliches SRAM. Drei Dinge sind dabei zu beachten.</p>

<ol class="merkliste">
<li><b>Zeichen, nicht Pixel.</b> <code>display_string_pos()</code> und
<code>display_printf_pos()</code> zaehlen in <b>Zeichen</b>: die Spalte von 0
bis 31 beim 3x5-Satz, die Zeile von 0 bis 7. Eine Zeile ist eine Page und
damit 8 Pixel hoch. Faustregel fuer den 3x5-Satz: <b>Spalte mal 4 ergibt die
Pixelposition, Zeile mal 8 ebenso.</b></li>
<li><b>Neben das Fenster, nicht hinein.</b>
<code>display_draw_show()</code> uebertraegt den gesamten geaenderten Bereich
des Videopuffers und weiss nichts von Text, der am Puffer vorbei geschrieben
wurde. Was im Fenster steht, ist beim naechsten Bild wieder weg.</li>
<li><b>Eine ungueltige Position faellt nicht auf.</b> Eine Spalte oder Zeile
ausserhalb des Rasters wird verworfen, der Text aber trotzdem ausgegeben:
an der aktuellen Cursorposition, meist also bei (0, 0) und damit mitten im
Fenster, wo ihn das naechste <code>display_draw_show()</code> zerschneidet.
Es gibt keine Fehlermeldung.</li>
</ol>

<pre class="code">// Fenster auf der linken Haelfte, Pixel 0 bis 63
display_draw_init(64, 64, 0, 0);

display_printf_pos(70, 1, "Punkte %u", p);   // falsch: 70 waere eine Pixelspalte,
                                             // als Zeichenspalte gibt es sie nicht
display_printf_pos(16, 1, "Punkte %u", p);   // richtig: Spalte 16 = Pixel 64</pre>

<h3>Einbinden: Ordner und Include-Pfade</h3>

<p>Ein frisches Projekt aus dem MegaLib-Template sieht so aus. Die Bibliothek
liegt vollstaendig im Projektordner; es gibt nichts zu installieren und nichts
einzustellen:</p>

<pre class="code">MeinSpiel/
&#9500;&#9472; main.c                        &#8592; hier wird programmiert
&#9500;&#9472; MeinSpiel.cproj
&#9492;&#9472; megalib/
   &#9500;&#9472; display/
   &#9474;  &#9500;&#9472; display.h               Text ausserhalb des Zeichenfensters
   &#9474;  &#9500;&#9472; display_draw.h          Zeichnen innerhalb des Fensters
   &#9474;  &#9500;&#9472; display_draw_sprite.h   bewegte Bilder im Fenster
   &#9474;  &#9500;&#9472; random.h                Zufallszahlen
   &#9474;  &#9500;&#9472; font/                   Zeichensatz
   &#9474;  &#9500;&#9472; twi/                    I2C-Treiber
   &#9474;  &#9492;&#9472; animations/             fertige Bilder
   &#9492;&#9472; sound/
      &#9492;&#9472; sound.h                 Toene und Melodien</pre>

<p>Eingebunden wird nur, was man auch aufruft. Jede Zeile entspricht genau
einem Reiter dieser Seite:</p>

<div class="vergleich">
  <table>
    <tr><th>Was man machen will</th><th>Reiter</th><th>Zeile in der <code>main.c</code></th></tr>
    <tr><td>Text in Zeilen und Spalten, ohne SRAM-Bedarf</td>
        <td>Ausserhalb</td>
        <td><code>#include "megalib/display/display.h"</code></td></tr>
    <tr><td>Pixel, Linien, Bilder und Schrift im Fenster</td>
        <td>Innerhalb</td>
        <td><code>#include "megalib/display/display_draw.h"</code></td></tr>
    <tr><td>Bilder, die sich von selbst bewegen</td>
        <td>Sprites</td>
        <td><code>#include "megalib/display/display_draw_sprite.h"</code></td></tr>
    <tr><td>Toene und Melodien am Piezo</td>
        <td>Ton</td>
        <td><code>#include "megalib/sound/sound.h"</code></td></tr>
    <tr><td>Zufallszahlen fuer Wuerfel, Gegner, Hindernisse</td>
        <td>Zufall</td>
        <td><code>#include "megalib/display/random.h"</code></td></tr>
  </table>
</div>

<p>Eine <code>main.c</code>, die ein Spielfeld zeichnet und den Punktestand
daneben schreibt, beginnt also so:</p>

<pre class="code">#include &lt;avr/io.h&gt;
#include &lt;avr/pgmspace.h&gt;
#include &lt;util/delay.h&gt;

#include "megalib/display/display.h"        // Text neben dem Fenster
#include "megalib/display/display_draw.h"   // Zeichnen im Fenster</pre>

<ul class="merkliste">
<li><b>Spitze Klammern oder Anfuehrungszeichen?</b>
<code>&lt;avr/io.h&gt;</code> steht fuer eine Datei des Compilers,
<code>"megalib/..."</code> fuer eine Datei des eigenen Projekts. Die megalib
gehoert zum Projekt, also immer Anfuehrungszeichen.</li>
<li><b>Der Pfad zaehlt ab der eigenen Datei</b>, nicht ab dem Projektordner.
Aus der <code>main.c</code> geht es direkt nach
<code>megalib/display/...</code>. Wer eigene Module in einem Unterordner
anlegt, etwa <code>spiel/motor.c</code>, muss eine Ebene hinauf:
<code>#include "../megalib/display/display_draw.h"</code>.</li>
<li><b>Nur Header werden eingebunden, nie eine <code>.c</code>-Datei.</b>
Microchip Studio uebersetzt und linkt jede <code>.c</code>-Datei des Projekts
von selbst. <code>font/</code> und <code>twi/</code> braucht deshalb niemand
einzubinden: Sie arbeiten im Hintergrund.</li>
<li><b><code>display_draw_sprite.h</code> holt <code>display_draw.h</code>
selbst</b> &ndash; eine Zeile genuegt fuer beides.</li>
<li><b><code>display_draw.h</code> holt <code>display.h</code> aber nicht.</b>
Wer im Fenster zeichnet <i>und</i> daneben Text schreibt, braucht beide Zeilen.
Fehlt die zweite, meldet der Compiler
<code>implicit declaration of function 'display_string_pos'</code>.
<code>display_draw_init()</code> ruft <code>display_init()</code> trotzdem
selbst auf; eingebunden werden muss der Header dennoch.</li>
</ul>

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
            {"id": "start_mischen", "titel": "Fenster und Text nebeneinander",
             "text": "Die beiden Schichten vertragen sich, solange sie sich "
                     "nicht denselben Platz teilen. Das Fenster rechnet in "
                     "Pixeln, der Text daneben in Zeichen. Der Simulator faerbt "
                     "alles orange ein, was ausserhalb der Zeichenflaeche liegt; "
                     "auf der MEGACARD leuchtet selbstverstaendlich alles gleich.",
             "eintraege": []},
        ],
    },
    {
        "id": "aussen",
        "titel": "Ausserhalb",
        "untertitel": "display.h &ndash; das ganze Display, Text in Zeilen und Spalten",
        "einleitung": """
<p class="einbinden"><b>Einbinden</b> <code>#include "megalib/display/display.h"</code></p>
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
<p class="einbinden"><b>Einbinden</b> <code>#include "megalib/display/display_draw.h"</code></p>
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
<p class="einbinden"><b>Einbinden</b> <code>#include "megalib/display/display_draw_sprite.h"</code></p>
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
<p class="einbinden"><b>Einbinden</b> <code>#include "megalib/sound/sound.h"</code></p>
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
<p class="einbinden"><b>Einbinden</b> <code>#include "megalib/display/random.h"</code></p>
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
