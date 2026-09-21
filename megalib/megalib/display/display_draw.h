/*-------------------------------------------------------------------------*\
| Datei:        display_draw.h
| Version:      1.1
| Projekt:      Zeichenbibliothek fuer die MEGACARD
| Beschreibung: Pixelgenaues Zeichnen in einem frei waehlbaren Fenster des
|               128x64-Displays. Setzt auf display.c auf.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   19.12.2025
|
| Aenderung:    Doku vereinheitlicht, Kommentare ohne Umlaute
\*-------------------------------------------------------------------------*/

#ifndef DISPLAY_DRAW_H_
#define DISPLAY_DRAW_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>

/* ===========================================================================
   Einstellungen und Makros

   Alles, was der Praeprozessor braucht, steht hier am Anfang beisammen.
   Weiter unten folgen nur noch Typen und Funktionen.
   =========================================================================== */

/**
 * @brief Obergrenze fuer die Groesse des Videopuffers in Byte.
 *
 * Der ATmega16 verfuegt ueber 1024 Byte SRAM, die sich Videopuffer, Variablen
 * und Stack teilen. Der Vorgabewert ist eine grobe obere Schranke, da die
 * uebrige Belegung des Programms zum Zeitpunkt der Uebersetzung nicht bekannt
 * ist. Projektspezifisch vor dem Einbinden ueberschreibbar.
 */
#ifndef DISPLAY_DRAW_MAX_BYTES
#define DISPLAY_DRAW_MAX_BYTES 900
#endif

/**
 * @brief Laenge des Zwischenpuffers von display_draw_printf().
 *
 * Der Puffer liegt waehrend des Aufrufs auf dem Stack und belegt danach
 * keinen Speicher mehr. Laengere Ausgaben werden abgeschnitten. Bei 1x
 * entsprechen 24 Zeichen 96 Pixel, also der vollen Breite des Displays.
 * Projektspezifisch vor dem Einbinden ueberschreibbar.
 */
#ifndef DISPLAY_DRAW_TEXT_MAX
#define DISPLAY_DRAW_TEXT_MAX 24
#endif

/**
 * @brief Initialisiert das Zeichenmodul und definiert ein Zeichenfenster.
 *
 * Alle Koordinaten (x, y) in den Zeichenfunktionen sind IMMER Pixelkoordinaten
 * relativ zu diesem Fenster, nicht zum physischen Display.
 *
 * Schliesst display_init() und damit twi_s_init() ein; ein vorheriger Aufruf
 * ist nicht erforderlich.
 *
 * Das Makro legt den Videopuffer als statisches Array an. Sein Bedarf betraegt
 * ((hoehe + 7) / 8) * breite Byte SRAM, bei 125x35 also 625 Byte. Da der Puffer
 * fest im Programm steht und nicht zur Laufzeit angefordert wird, weist ihn
 * Microchip Studio in der Zeile "Data" aus.
 *
 * @param breite Breite des Zeichenbereichs in Pixel (max. 128).
 * @param hoehe  Hoehe des Zeichenbereichs in Pixel (max. 64).
 * @param xpos   X-Pixeloffset des Fensters auf dem physischen Display.
 * @param ypos   Y-Pixeloffset des Fensters auf dem physischen Display.
 *
 * @warning Alle vier Argumente muessen konstante Ausdruecke sein, also
 *          Literale, #define oder enum-Konstanten. Variablen sind auch mit
 *          const unzulaessig, da breite und hoehe die Groesse des Puffers
 *          bestimmen.
 *
 * @warning Genau ein Aufruf je Programm. Jeder weitere Aufruf in einer anderen
 *          Funktion legt einen zusaetzlichen Puffer an, der Speicher belegt,
 *          ohne verwendet zu werden.
 *
 * @warning Das Fenster muss vollstaendig auf dem Display liegen, es gilt
 *          xpos + breite <= 128 und ypos + hoehe <= 64. Andernfalls gibt die
 *          Bibliothek beim Start eine Meldung aus und haelt an.
 */
#define display_draw_init(breite, hoehe, xpos, ypos)                           \
	do {                                                                        \
		static uint8_t _dd_puffer[((hoehe) + 7) / 8][breite];                    \
		_Static_assert(sizeof(_dd_puffer) <= DISPLAY_DRAW_MAX_BYTES,             \
		    "Zeichenflaeche zu gross: kein Platz mehr fuer Variablen und Stack." \
		    " Breite oder Hoehe verkleinern.");                                  \
		_display_draw_init(&_dd_puffer[0][0], (breite), (hoehe),                 \
		                   (xpos), (ypos));                                      \
	} while (0)

/* --- Bilder anlegen -------------------------------------------------------
 *
 * Ein Bild ist durch Breite, Hoehe und Ablageart vollstaendig beschrieben.
 * Diese Angaben sind Eigenschaften des Bildes und nicht des Aufrufers; sie
 * stehen deshalb in einem BITMAP_T unmittelbar neben den Daten.
 *
 * Bilder werden ausschliesslich ueber die folgenden Makros angelegt. Beide
 * pruefen zur Uebersetzungszeit, ob die Groesse der Datentabelle zu den
 * angegebenen Massen passt, und brechen den Bau andernfalls mit einer
 * erlaeuternden Meldung ab.
 */

// Wieviele Byte eine Tabelle dieser Masse haben muss. Nur fuer die Makros.
#define _BM_BYTES_VERTIKAL(breite, hoehe) \
	((unsigned)(breite) * (((unsigned)(hoehe) + 7u) / 8u))
#define _BM_BYTES_HORIZONTAL(breite, hoehe) \
	((((unsigned)(breite) + 7u) / 8u) * (unsigned)(hoehe))

/**
 * @brief Legt ein vertikal abgelegtes Bild an.
 *
 * Vertikale Ablage: ein Byte umfasst acht uebereinanderliegende Pixel, die
 * Bytes verlaufen zeilenweise von links nach rechts. Dies entspricht dem
 * Speicherformat des Displays und ist beim Zeichnen deutlich schneller.
 * Werkzeuge wie Piskel geben Daten fuer OLED-Displays in dieser Form aus.
 *
 *     static const uint8_t Mond_daten[] PROGMEM = { ... };
 *     BITMAP_VERTIKAL(Mond, Mond_daten, 16, 16);
 *     ...
 *     display_draw_bitmap(108, 2, &Mond);
 *
 * @param name    Bezeichner des Bildes, unter dem es gezeichnet wird.
 * @param tabelle Bezeichner der Datentabelle. Muss in derselben Uebersetzungs-
 *                einheit liegen, da ihre Groesse sonst nicht bekannt ist.
 * @param breite  Breite in Pixel.
 * @param hoehe   Hoehe in Pixel.
 */
#define BITMAP_VERTIKAL(name, tabelle, breite, hoehe)                          \
	_Static_assert(sizeof(tabelle) == _BM_BYTES_VERTIKAL(breite, hoehe),        \
	    "Bild " #name ": Groesse der Tabelle passt nicht zu Breite und Hoehe."  \
	    " Vertikal gilt Breite mal aufgerundete Hoehe durch acht.");            \
	const BITMAP_T name PROGMEM = { (tabelle), (breite), (hoehe), false }

/**
 * @brief Legt ein horizontal abgelegtes Bild an.
 *
 * Horizontale Ablage: ein Byte umfasst acht nebeneinanderliegende Pixel,
 * hoechstwertiges Bit links, jede Bildzeile beginnt an einer Bytegrenze. Diese
 * Ablage erfordert beim Zeichnen eine pixelweise Transposition und ist
 * entsprechend langsamer. Im Uebrigen wie BITMAP_VERTIKAL.
 */
#define BITMAP_HORIZONTAL(name, tabelle, breite, hoehe)                        \
	_Static_assert(sizeof(tabelle) == _BM_BYTES_HORIZONTAL(breite, hoehe),      \
	    "Bild " #name ": Groesse der Tabelle passt nicht zu Breite und Hoehe."  \
	    " Horizontal gilt aufgerundete Breite durch acht mal Hoehe.");          \
	const BITMAP_T name PROGMEM = { (tabelle), (breite), (hoehe), true }


/**
 * @brief Ein Bild samt seiner Beschreibung.
 *
 * Instanzen werden ausschliesslich ueber BITMAP_VERTIKAL oder
 * BITMAP_HORIZONTAL angelegt. Die Makros legen den Beschreiber im
 * Programmspeicher ab.
 *
 * @warning Beschreiber und Bilddaten muessen im Programmspeicher liegen. Die
 *          Zeichenfunktionen greifen ueber pgm_read darauf zu; ein im SRAM
 *          angelegter Beschreiber liefert undefinierte Werte.
 */
typedef struct {
	const uint8_t *daten;   ///< Zeiger auf die Pixeldaten im Flash
	uint8_t breite;         ///< Breite in Pixel
	uint8_t hoehe;          ///< Hoehe in Pixel
	bool    horizontal;     ///< true = horizontal abgelegt, false = vertikal
} BITMAP_T;


/* [intern] Die eigentliche Initialisierung. Nicht direkt aufrufen, dafuer gibt
   es das Makro display_draw_init() weiter oben. Den Puffer legt das Makro an. */
void _display_draw_init(uint8_t *puffer, uint8_t display_width, uint8_t display_height,
                        uint8_t display_x_pos, uint8_t display_y_pos);

/**
 * @brief Liefert die bei display_draw_init() gesetzte Fensterbreite in Pixel.
 * @return Breite des Zeichenfensters, 0 vor der Initialisierung.
 */
uint8_t display_draw_get_width(void);

/**
 * @brief Liefert die bei display_draw_init() gesetzte Fensterhoehe in Pixel.
 * @return Hoehe des Zeichenfensters, 0 vor der Initialisierung.
 */
uint8_t display_draw_get_height(void);

/**
 * @brief Liefert den X-Offset des Fensters auf dem physischen Display.
 * @return X-Position der linken oberen Fensterecke in Pixel.
 */
uint8_t display_draw_get_x_pos(void);

/**
 * @brief Liefert den Y-Offset des Fensters auf dem physischen Display.
 * @return Y-Position der linken oberen Fensterecke in Pixel.
 */
uint8_t display_draw_get_y_pos(void);


/**
 * @brief Loescht den gesamten Zeichenbereich (alle Pixel).
 *
 * Loescht Anzeige und Videopuffer gemeinsam und wirkt sofort. Ein Aufruf von
 * display_draw_show() ist danach nicht noetig.
 */
void display_draw_clear(void);


/**
 * @brief Zeigt das Gezeichnete auf dem Display an.
 *
 * Die Zeichenfunktionen schreiben nicht unmittelbar auf das Display, sondern
 * vermerken die geaenderten Spalten im Videopuffer. Erst dieser Aufruf
 * uebertraegt sie. Unterbleibt er, bleibt die Anzeige unveraendert; eine
 * Fehlermeldung erfolgt nicht.
 *
 * Der Aufruf gehoert an das Ende eines vollstaendig gezeichneten Bildes, nicht
 * hinter jede einzelne Zeichenoperation. Bei Verwendung des Sprite-Systems
 * entfaellt er, da display_draw_sprite_update_all() ihn am Bildende ausfuehrt.
 *
 * Ohne ausstehende Aenderungen ist der Aufruf nahezu kostenfrei.
 *
 * @note Die Sammelbetriebsart setzt voraus, dass display_y_pos ein Vielfaches
 *       von 8 ist, das Fenster also an einer Page-Grenze beginnt. In diesem
 *       Fall wird je Page eine Burst-Transaktion von der ersten bis zur
 *       letzten geaenderten Spalte gesendet. Bei versetztem Fenster
 *       (display_y_pos % 8 != 0) verteilt sich jede Pufferzeile auf zwei
 *       Display-Pages; dort schreibt jede Zeichenoperation unmittelbar, und
 *       dieser Aufruf bleibt ohne Wirkung. Er ist dennoch erforderlich, damit
 *       derselbe Programmcode mit beiden Fensterlagen arbeitet.
 */
void display_draw_show(void);

/**
 * @brief Zweitname von display_draw_show().
 *
 * Leitet unveraendert an display_draw_show() weiter. Fuer neuen Code ist
 * display_draw_show() vorgesehen.
 */
void display_draw_flush(void);


/**
 * @brief Setzt ein einzelnes Pixel.
 *
 * Koordinaten ausserhalb des Zeichenfensters werden verworfen.
 *
 * @param x X-Pixelkoordinate relativ zum Zeichenfenster.
 * @param y Y-Pixelkoordinate relativ zum Zeichenfenster.
 */
void display_draw_pixel(uint8_t x, uint8_t y);


/**
 * @brief Loescht ein einzelnes Pixel.
 *
 * Gegenstueck zu display_draw_pixel(). Alle Funktionen mit display_clear_
 * gehoeren zu diesem Modul und arbeiten im selben Zeichenfenster. Nicht zu
 * verwechseln mit display_clear() aus display.h, das die ganze Anzeige leert.
 *
 * @param x X-Pixelkoordinate relativ zum Zeichenfenster.
 * @param y Y-Pixelkoordinate relativ zum Zeichenfenster.
 */
void display_clear_pixel(uint8_t x, uint8_t y);


/**
 * @brief Zeichnet bis zu 8 vertikale Pixel an einer X-Position.
 *
 * x und y sind Pixelkoordinaten. Intern wird y auf die entsprechende
 * Display-Page abgebildet (y bestimmt die 8-Pixel-Gruppe).
 *
 * Jedes Bit im Parameter byte entspricht einem Pixel:
 * Bit 0 = Pixel bei y, Bit 7 = Pixel bei y+7.
 *
 * @param x    X-Pixelkoordinate relativ zum Zeichenfenster.
 * @param y    Y-Pixelkoordinate (oberstes Pixel des Bytes).
 * @param byte Bitmaske der zu zeichnenden Pixel.
 */
void display_draw_byte(uint8_t x, uint8_t y, uint8_t byte);


/**
 * @brief Loescht bis zu 8 vertikale Pixel an einer X-Position.
 *
 * @param x    X-Pixelkoordinate relativ zum Zeichenfenster.
 * @param y    Y-Pixelkoordinate (oberstes Pixel des Bytes).
 * @param byte Bitmaske der zu loeschenden Pixel.
 */
void display_clear_byte(uint8_t x, uint8_t y, uint8_t byte);


/**
 * @brief Zeichnet eine Linie zwischen zwei Pixelpunkten.
 *
 * Verwendet den Bresenham-Algorithmus.
 *
 * @param x1 Start-X (Pixel)
 * @param y1 Start-Y (Pixel)
 * @param x2 End-X (Pixel)
 * @param y2 End-Y (Pixel)
 */
void display_draw_line(uint8_t x1, uint8_t y1,
                       uint8_t x2, uint8_t y2);


/**
 * @brief Loescht eine Linie zwischen zwei Pixelpunkten.
 *
 * @param x1 Start-X (Pixel)
 * @param y1 Start-Y (Pixel)
 * @param x2 End-X (Pixel)
 * @param y2 End-Y (Pixel)
 */
void display_clear_line(uint8_t x1, uint8_t y1,
                        uint8_t x2, uint8_t y2);


/**
 * @brief Zeichnet eine Linie mit definierter Schrittweite.
 *
 * Alle Koordinaten sind Pixelkoordinaten.
 * Bei step > 1 entsteht eine gestrichelte oder punktierte Linie.
 *
 * @param x1   Start-X (Pixel)
 * @param y1   Start-Y (Pixel)
 * @param x2   End-X (Pixel)
 * @param y2   End-Y (Pixel)
 * @param step Schrittweite (1 = durchgehend)
 */
void display_draw_line_with_step(uint8_t x1, uint8_t y1,
                                 uint8_t x2, uint8_t y2,
                                 uint8_t step);


/**
 * @brief Loescht eine Linie mit definierter Schrittweite.
 *
 * @param x1   Start-X (Pixel)
 * @param y1   Start-Y (Pixel)
 * @param x2   End-X (Pixel)
 * @param y2   End-Y (Pixel)
 * @param step Schrittweite (1 = durchgehend)
 */
void display_clear_line_with_step(uint8_t x1, uint8_t y1,
                                  uint8_t x2, uint8_t y2,
                                  uint8_t step);


/**
 * @brief Zeichnet ein Rechteck.
 *
 * x und y sind Pixelkoordinaten der linken oberen Ecke.
 *
 * @param x      Linke obere X-Pixelposition.
 * @param y      Linke obere Y-Pixelposition.
 * @param width  Breite in Pixel.
 * @param height Hoehe in Pixel.
 * @param filled true = gefuellt, false = nur Rahmen.
 */
void display_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled);


/**
 * @brief Loescht ein Rechteck.
 *
 * @param x      Linke obere X-Pixelposition.
 * @param y      Linke obere Y-Pixelposition.
 * @param width  Breite in Pixel.
 * @param height Hoehe in Pixel.
 * @param filled true = gesamte Flaeche loeschen, false = nur Rahmen.
 */
void display_clear_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled);



/* --- Bilder zeichnen ------------------------------------------------------
 *
 * Breite, Hoehe und Ablageart stecken im Bild, beim Zeichnen genuegen deshalb
 * Ort und Bild.
 *
 * @note Ragt ein Bild rechts oder unten aus dem Zeichenfenster heraus, wird es
 *       sauber abgeschnitten. Der sichtbare Rest bleibt an der richtigen Stelle.
 */

/**
 * @brief Zeichnet ein Bild und ueberschreibt dabei den Hintergrund.
 *
 * Das ganze Rechteck wird ersetzt, die Nullpixel des Bildes loeschen also, was
 * darunter liegt.
 *
 *     display_draw_bitmap(0, 0, &Startbild);
 *
 * @param x    Start-X in Pixel (linke obere Ecke).
 * @param y    Start-Y in Pixel (linke obere Ecke).
 * @param bild Zeiger auf das Bild. NULL zeichnet nichts.
 */
void display_draw_bitmap(uint8_t x, uint8_t y, const BITMAP_T *bild);

/**
 * @brief Zeichnet ein Bild transparent.
 *
 * Nur die gesetzten Pixel werden gezeichnet, der Untergrund bleibt sichtbar.
 * Fuer alles, was ueber etwas anderem liegen soll.
 *
 * @param x    Start-X in Pixel (linke obere Ecke).
 * @param y    Start-Y in Pixel (linke obere Ecke).
 * @param bild Zeiger auf das Bild. NULL zeichnet nichts.
 */
void display_draw_bitmap_transparent(uint8_t x, uint8_t y, const BITMAP_T *bild);

/**
 * @brief Loescht ein Bild wieder.
 *
 * Setzt genau die Pixel zurueck, die das Bild gesetzt haette. Der uebrige
 * Inhalt des Rechtecks bleibt stehen.
 *
 * @param x    Start-X in Pixel (linke obere Ecke).
 * @param y    Start-Y in Pixel (linke obere Ecke).
 * @param bild Zeiger auf das Bild. NULL loescht nichts.
 *
 * @warning Geloescht werden die Pixel des Bildes, ganz gleich was darunter lag.
 *          Was das Bild verdeckt hat, muss das Programm selbst nachziehen.
 */
void display_clear_bitmap(uint8_t x, uint8_t y, const BITMAP_T *bild);


/**
 * @brief Schrift-Skalierungsfaktoren im Q8.8-Festkommaformat.
 *
 * Die Werte sind als Faktor mal 256 gespeichert, 256 ist also die
 * Originalgroesse. Skaliert wird in X- und Y-Richtung gleich.
 *
 * Bei Verkleinerung wird ein Zielpixel gesetzt, sobald mindestens eines der
 * von ihm abgedeckten Quellpixel gesetzt ist. Die Schrift bleibt dadurch
 * lesbar, erscheint jedoch fetter. Fuer dauerhaft kleine Schrift ist die
 * Auswahl von FONT_3x5 in font/FontData.c der Herunterskalierung vorzuziehen.
 */
typedef enum
{
	FONT_SIZE_0_5X  = 128,   ///< 0.5x
	FONT_SIZE_0_75X = 192,   ///< 0.75x
	FONT_SIZE_1X    = 256,   ///< 1.0x (Originalgroesse)
	FONT_SIZE_1_25X = 320,   ///< 1.25x
	FONT_SIZE_1_5X  = 384,   ///< 1.5x
	FONT_SIZE_2X    = 512,   ///< 2.0x
	FONT_SIZE_3X    = 768,   ///< 3.0x
	FONT_SIZE_4X    = 1024   ///< 4.0x
} FontSize_t;


/**
 * @brief Zeichnet ein einzelnes ASCII-Zeichen.
 *
 * Das Zeichen wird aus dem in font/FontData.c gewaehlten Zeichensatz gelesen
 * und mit dem angegebenen Faktor vergroessert oder verkleinert. Zeichen
 * ausserhalb des Satzes werden als Fragezeichen gezeichnet.
 *
 * @param x        Linke obere X-Pixelposition relativ zum Zeichenfenster.
 * @param y        Linke obere Y-Pixelposition relativ zum Zeichenfenster.
 * @param c        ASCII-Zeichen.
 * @param size     Skalierungsfaktor (FontSize_t, Q8.8). 0 gilt als 1x.
 * @param clear_bg true  = Hintergrundrechteck des Zeichens vorher loeschen,
 *                 false = nur gesetzte Pixel zeichnen, der Untergrund bleibt
 *                         sichtbar.
 */
void display_draw_char(uint8_t x, uint8_t y, char c, FontSize_t size, bool clear_bg);


/**
 * @brief Zeichnet einen nullterminierten String aus dem RAM.
 *
 * Der String wird Zeichen fuer Zeichen gezeichnet, bis die abschliessende Null
 * erreicht ist. Zwischen den Zeichen wird der im Zeichensatz festgelegte
 * Abstand eingehalten, ebenfalls skaliert.
 *
 * @param x     Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y     Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param s     Zeiger auf nullterminierten String im RAM. NULL zeichnet nichts.
 * @param size  Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 *
 * @warning Der Hintergrund wird stets geloescht, einschliesslich des
 *          Zeichenabstands. Fuer Text ueber vorhandenem Bildinhalt ist
 *          display_draw_char() je Zeichen mit clear_bg = false zu verwenden.
 */
void display_draw_string(uint8_t x, uint8_t y, const char *s, FontSize_t size);

/**
 * @brief Zeichnet einen nullterminierten String aus dem Programmspeicher.
 *
 * Verhaelt sich wie display_draw_string(), der String liegt aber im Flash und
 * belegt keinen SRAM. Aufruf mit PSTR: display_draw_string_P(0, 0,
 * PSTR("Hallo"), FONT_SIZE_1X);
 *
 * @param x     Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y     Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param s     Zeiger auf nullterminierten String im PROGMEM. NULL zeichnet nichts.
 * @param size  Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 *
 * @warning Der Hintergrund wird immer geloescht, samt Zeichenabstand. Siehe
 *          display_draw_string().
 */
void display_draw_string_P(uint8_t x, uint8_t y, PGM_P s, FontSize_t size);


/**
 * @brief Zeichnet eine Zahl.
 *
 * Wandelt den Wert ohne printf in Text um und zeichnet ihn. Kostet daher
 * nur wenige hundert Byte Flash, kann aber auch nichts anderes als eine
 * vorzeichenlose Dezimalzahl.
 *
 * @param x     Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y     Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param wert  Auszugebende Zahl, 0 bis 65535.
 * @param size  Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 */
void display_draw_zahl(uint8_t x, uint8_t y, uint16_t wert, FontSize_t size);


/**
 * @brief Zeichnet ein Byte als acht Nullen und Einsen.
 *
 * Links steht Bit 7, rechts Bit 0, wie in der ueblichen Schreibweise
 * 0b10010110. Nuetzlich, um Bitoperationen sichtbar zu machen.
 *
 * @param x     Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y     Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param wert  Auszugebendes Byte.
 * @param size  Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 */
void display_draw_binaer(uint8_t x, uint8_t y, uint8_t wert, FontSize_t size);


/**
 * @brief Zeichnet formatierten Text, Format aus dem RAM.
 *
 * Setzt den Text wie printf aus dem Format und den weiteren Argumenten
 * zusammen und zeichnet ihn:
 *
 *     display_draw_printf(4, 12, FONT_SIZE_1X, "PUNKTE %u", punkte);
 *
 * Der Zwischenpuffer liegt auf dem Stack und ist DISPLAY_DRAW_TEXT_MAX
 * Zeichen lang; laengere Ausgaben werden abgeschnitten.
 *
 * @note Zieht vsnprintf aus der avr-libc nach sich und kostet damit rund
 *       1,5 KByte Flash. Wer nur eine Zahl ausgibt, nimmt besser
 *       display_draw_zahl(). Fliesskommaformate wie %f sind in der
 *       Standardeinstellung von Microchip Studio nicht enthalten.
 *
 * @param x       Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y       Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param size    Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 * @param format  Formatstring im RAM, danach die einzusetzenden Werte.
 */
void display_draw_printf(uint8_t x, uint8_t y, FontSize_t size, const char *format, ...)
	__attribute__((format(printf, 4, 5)));


/**
 * @brief Zeichnet formatierten Text, Format aus dem Programmspeicher.
 *
 * Wie display_draw_printf(), der Formatstring liegt aber im Flash und
 * belegt keinen SRAM:
 *
 *     display_draw_printf_P(4, 12, FONT_SIZE_1X, PSTR("PUNKTE %u"), punkte);
 *
 * @param x       Start-X-Pixelposition relativ zum Zeichenfenster.
 * @param y       Start-Y-Pixelposition relativ zum Zeichenfenster.
 * @param size    Skalierungsfaktor (FontSize_t). 0 gilt als 1x.
 * @param format  Formatstring im PROGMEM, danach die einzusetzenden Werte.
 */
void display_draw_printf_P(uint8_t x, uint8_t y, FontSize_t size, PGM_P format, ...);

#endif /* DISPLAY_DRAW_H_ */
