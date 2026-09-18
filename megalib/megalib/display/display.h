/*-------------------------------------------------------------------------*\
| Datei:        display.h
| Version:      1.1
| Projekt:      Display-Bibliothek fuer die MEGACARD
| Beschreibung: Treiber fuer das SSD1306-OLED. Zeichenweise Textausgabe in
|               einem Raster aus Zeilen und Spalten, dazu ein Zugang zum
|               Grafikspeicher fuer display_draw.c.
| Schaltung:    MEGACARD V6.11, OLED an PB0 (SCL) und PB1 (SDA)
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   3.3.2022
|
| Aenderung:    Doku vereinheitlicht, const bei display_string_pos
\*-------------------------------------------------------------------------*/
#ifndef DISPLAY_H_
#define DISPLAY_H_

#include <stdbool.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#define OLED_PAGES 8       ///< Zahl der 8 Pixel hohen Speicherstreifen
#define OLED_PIXEL_X 128   ///< Breite des Displays in Pixel
#define OLED_PIXEL_Y 64    ///< Hoehe des Displays in Pixel

/**
 * @brief Initialisiert die Schnittstelle und schaltet das Display ein.
 *
 * Ruft twi_s_init() selbst auf, sendet die Initialisierungssequenz aus dem
 * Datenblatt, loescht die Anzeige und setzt den Cursor nach oben links.
 * Muss vor jeder anderen Funktion dieser Datei aufgerufen werden.
 *
 * @note Bei Verwendung von display_draw_init() entfaellt dieser Aufruf; er ist
 *       dort bereits enthalten.
 */
void display_init(void);

/**
 * @brief Zahl der Textzeilen, die auf das Display passen.
 *
 * Der Wert haengt vom gewaehlten Zeichensatz ab und aendert sich mit der
 * Auswahl in font/FontData.c: 8 Zeilen beim 5x8-Satz, 12 beim 3x5-Satz.
 *
 * @return Zahl der Zeilen.
 */
uint8_t display_lines (void);

/**
 * @brief Zahl der Zeichen, die in eine Textzeile passen.
 *
 * Ergibt sich aus Zeichenbreite und Zeichenabstand des gewaehlten Satzes.
 *
 * @return Zahl der Zeichen je Zeile.
 */
uint8_t display_chars (void);

/**
 * @brief Erstes im Zeichensatz enthaltenes Zeichen.
 * @return ASCII-Code des ersten Zeichens, ueblicherweise 32 (Leerzeichen).
 */
uint8_t display_char_first (void);

/**
 * @brief Letztes im Zeichensatz enthaltenes Zeichen.
 * @return ASCII-Code des letzten Zeichens.
 */
uint8_t display_char_last (void);

/**
 * @brief Loescht die gesamte Anzeige und setzt den Cursor auf (0, 0).
 *
 * Wirkt auf das gesamte Display, nicht auf ein einzelnes Zeichenfenster. In
 * Verbindung mit display_draw ist display_draw_clear() zu verwenden, da
 * Anzeige und Videopuffer sonst auseinanderlaufen.
 */
void display_clear(void);

/**
 * @brief Verschiebt den Bildschirminhalt um eine Zeile nach oben.
 *
 * Die unterste Zeile wird dabei leer. Verschoben wird ueber den Zeilenversatz
 * des Displays, der Inhalt wird also nicht umkopiert.
 */
void display_scroll_up (void);

/**
 * @brief Verschiebt den Bildschirminhalt um eine Zeile nach unten.
 *
 * Die oberste Zeile wird dabei leer.
 */
void display_scroll_down (void);

/**
 * @brief Setzt den Cursor auf eine Position im Zeichenraster.
 *
 * Die Zaehlung beginnt bei 0, posx von links nach rechts, posy von oben nach
 * unten. Es handelt sich um Zeichen-, nicht um Pixelkoordinaten: posx wird mit
 * Zeichenbreite zuzueglich Abstand multipliziert, posy bezeichnet die Textzeile.
 *
 * @param posx Spalte, 0 bis display_chars() - 1.
 * @param posy Zeile, 0 bis display_lines() - 1.
 *
 * @note Liegt eine der Angaben ausserhalb des gueltigen Bereichs, bleibt die
 *       Cursorposition unveraendert; eine Fehlermeldung erfolgt nicht.
 */
void display_pos(uint8_t posx, uint8_t posy);

/**
 * @brief Gibt ein Zeichen an der aktuellen Cursorposition aus.
 *
 * Rueckt den Cursor anschliessend um eine Stelle weiter.
 *
 * @param c Auszugebendes Zeichen. Zeichen ausserhalb des Satzes werden durch
 *          das erste Zeichen des Satzes ersetzt.
 */
void display_char(uint8_t c);

/**
 * @brief Schreibt eine Zeichenkette an die aktuelle Cursorposition.
 *
 * @param line         Nullterminierte Zeichenkette.
 * @param from_progmem true = line liegt im Flash (mit PSTR erzeugt),
 *                     false = line liegt im RAM.
 */
void display_string(const char *line, bool from_progmem);

/**
 * @brief Schreibt eine Zeichenkette aus dem RAM an eine Rasterposition.
 *
 * @param posx Spalte im Zeichenraster.
 * @param posy Zeile im Zeichenraster.
 * @param line Nullterminierte Zeichenkette im RAM.
 */
void display_string_pos (uint8_t posx, uint8_t posy, const char *line);

/**
 * @brief Schreibt eine Zeichenkette aus dem Flash an eine Rasterposition.
 *
 * Vermeidet SRAM-Bedarf, da gewoehnliche Zeichenketten beim Start ins SRAM
 * kopiert werden. Aufruf: display_string_pos_P(0, 0, PSTR("Hallo"));
 *
 * @param posx Spalte im Zeichenraster.
 * @param posy Zeile im Zeichenraster.
 * @param line Nullterminierte Zeichenkette im PROGMEM.
 */
void display_string_pos_P(uint8_t posx, uint8_t posy, PGM_P line);

/**
 * @brief Schreibt formatierten Text an die aktuelle Cursorposition.
 *
 * @param fmt Formatstring im RAM, danach die einzusetzenden Werte.
 * @return Zahl der Zeichen, die der Text haette haben sollen, oder ein
 *         negativer Wert bei einem Formatfehler.
 *
 * @warning Der Text wird in einem 25 Byte grossen Zwischenspeicher aufgebaut;
 *          Zeichen ab Position 25 entfallen. Der Rueckgabewert kann daher die
 *          Zahl der tatsaechlich ausgegebenen Zeichen uebersteigen.
 * @warning Die Formatierung beansprucht rund 1,5 kB Flash und mehrere Dutzend
 *          Byte Stack. Fuer unformatierten Text ist display_string_pos_P()
 *          vorzuziehen.
 */
int8_t  display_printf(const char *fmt, ...);

/**
 * @brief Schreibt formatierten Text an eine Rasterposition.
 *
 * @param posx Spalte im Zeichenraster.
 * @param posy Zeile im Zeichenraster.
 * @param fmt  Formatstring im RAM, danach die einzusetzenden Werte.
 * @return Wie bei display_printf().
 *
 * @warning Ebenfalls auf 24 Zeichen begrenzt, siehe display_printf().
 */
int8_t  display_printf_pos(uint8_t posx, uint8_t posy, const char *fmt, ...);

/**
 * @brief Wie display_printf_pos(), der Formatstring liegt aber im Flash.
 *
 * Spart SRAM, denn ein gewoehnlicher Formatstring wird beim Start ins SRAM
 * kopiert. Aufruf: display_printf_pos_P(0, 5, PSTR("Wert: %u"), wert);
 *
 * @param posx Spalte im Zeichenraster.
 * @param posy Zeile im Zeichenraster.
 * @param fmt  Formatstring im PROGMEM, danach die einzusetzenden Werte.
 * @return Wie bei display_printf().
 *
 * @warning Ebenfalls auf 24 Zeichen begrenzt, siehe display_printf().
 */
int8_t  display_printf_pos_P(uint8_t posx, uint8_t posy, PGM_P fmt, ...);

/**
 * @brief Schreibt ein Byte direkt in den Grafikspeicher.
 *
 * Jedes Bit entspricht einem Pixel, Bit 0 liegt oben. Das Byte fuellt stets
 * einen vollstaendigen, acht Pixel hohen Streifen; prow wird daher auf die
 * naechstniedrigere Page abgerundet. Einzige Grafikfunktion dieser Schicht,
 * auf der display_draw.c aufsetzt.
 *
 * @param pcol  Spalte in Pixel, 0 bis 127.
 * @param prow  Zeile in Pixel, 0 bis 63. Wird auf ein Vielfaches von 8 gerundet.
 * @param pbyte Die acht zu setzenden Pixel.
 */
void display_pixel_byte (uint8_t pcol, uint8_t prow, uint8_t pbyte);

/**
 * @brief Setzt den Schreibzeiger und beginnt eine fortlaufende Uebertragung.
 *
 * Fuer zusammenhaengende Byte-Folgen deutlich schneller als einzelne Aufrufe
 * von display_pixel_byte(), da Adressierung und Busprotokoll nur einmal
 * anfallen. Auf jeden Aufruf muessen display_burst_write() und
 * display_burst_end() folgen.
 *
 * @param pcol Startspalte in Pixel.
 * @param prow Startzeile in Pixel, wird auf ein Vielfaches von 8 gerundet.
 *
 * @warning Zwischen Start und Ende darf keine weitere Displayfunktion
 *          aufgerufen werden; andernfalls bricht die Uebertragung ab.
 */
void display_burst_start(uint8_t pcol, uint8_t prow);

/**
 * @brief Schreibt ein Byte innerhalb einer laufenden Uebertragung.
 *
 * Der Schreibzeiger rueckt danach automatisch eine Spalte weiter.
 *
 * @param byte Die acht zu setzenden Pixel.
 */
void display_burst_write(uint8_t byte);

/**
 * @brief Beendet eine laufende Uebertragung und gibt den Bus frei.
 */
void display_burst_end(void);

#endif /* DISPLAY_H_ */
