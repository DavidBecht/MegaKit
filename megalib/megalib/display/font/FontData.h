/*-------------------------------------------------------------------------*\
| Datei:        FontData.h
| Version:      1.1
| Projekt:      Display-Bibliothek fuer die MEGACARD
| Beschreibung: Beschreibung des gewaehlten Zeichensatzes. Welcher es ist,
|               wird in FontData.c umgeschaltet.
| Schaltung:    MEGACARD V6.11
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   3.3.2022
|
| Aenderung:    Doku vereinheitlicht, fontParam schreibgeschuetzt
\*-------------------------------------------------------------------------*/
#ifndef FONTDATA_H_
#define FONTDATA_H_

#include <stdint.h>

/**
 * @brief Masse des gewaehlten Zeichensatzes.
 *
 * Aus diesen fuenf Zahlen rechnet display.c das Textraster aus, und
 * display_draw.c bestimmt damit die Groesse eines Zeichens. Ein anderer
 * Zeichensatz verschiebt deshalb jede Textausgabe im ganzen Projekt.
 */
typedef struct
{
   uint8_t char_first;  ///< ASCII-Code des ersten enthaltenen Zeichens
   uint8_t char_last;   ///< ASCII-Code des letzten enthaltenen Zeichens
   uint8_t width;       ///< Breite eines Zeichens in Pixel, zugleich Byte je Zeichen
   uint8_t height;      ///< Hoehe eines Zeichens in Pixel
   uint8_t spacing;     ///< Leerspalten zwischen zwei Zeichen
} FontParam_t;

/**
 * @brief Masse des aktuell gewaehlten Zeichensatzes.
 *
 * Wird in FontData.c gesetzt und ist nur lesbar. Umgeschaltet wird ueber die
 * Auswahl der Zeichensaetze am Anfang von FontData.c, nicht zur Laufzeit.
 */
extern const FontParam_t fontParam;

/**
 * @brief Die Zeichendaten im Flash, spaltenweise, ab dem ersten Zeichen.
 *
 * Jedes Zeichen belegt fontParam.width Byte, je Spalte eines. In jedem Byte
 * ist Bit 0 das oberste Pixel. Nur mit pgm_read_byte() lesen.
 */
extern const uint8_t fontData[];

#endif /* FONTDATA_H_ */
