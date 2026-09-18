/*-------------------------------------------------------------------------*\
| Datei:        main.c
| Version:      1.0
| Projekt:      $projectname$
| Beschreibung: Ausgangspunkt fuer ein Programm mit der megalib.
| Schaltung:    MEGACARD V6.11
| Autor:        $username$
| Erstellung:   $time$
|
| Aenderung:
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

// Zeichenflaeche 96 x 48 Pixel, mittig auf der 128 x 64 grossen Anzeige.
// Die Werte muessen beim Uebersetzen feststehen, display_draw_init() legt
// daraus den Videopuffer an: Breite * Hoehe / 8 = 576 Byte SRAM. Die ganze
// Anzeige (1024 Byte) passt nicht in den Speicher des ATmega16.
enum {
	BREITE = 96,
	HOEHE  = 48,
	X_POS  = 16,
	Y_POS  = 8,
};

// Taster S0..S3 an PA0..PA3, gedrueckt = 0
#define TASTER_MASKE 0x0F

int main(void)
{
	DDRA  &= (uint8_t)~TASTER_MASKE;   // Taster als Eingang
	PORTA |= TASTER_MASKE;             // Pull-ups ein

	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	// Koordinaten ab hier relativ zur Zeichenflaeche
	display_draw_rect(0, 0, BREITE, HOEHE, false);
	display_draw_string_P(6, 10, PSTR("Hallo MEGACARD"), FONT_SIZE_1X);
	display_draw_string_P(6, 22, PSTR("Taste druecken"), FONT_SIZE_1X);
	display_draw_show();

	while (true)
	{
		// Solange ein Taster gedrueckt ist, erscheint unten ein
		// ausgefuelltes Rechteck.
		if ((PINA & TASTER_MASKE) != TASTER_MASKE)
		{
			display_draw_rect(24, 36, 48, 6, true);
		}
		else
		{
			display_clear_rect(24, 36, 48, 6, true);
		}
		display_draw_show();
		_delay_ms(20);
	}
}
