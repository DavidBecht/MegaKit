// beispiel: start_mischen
// titel: Zeichenflaeche links, Text rechts daneben
// bild: 1.0s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display.h"
#include "megalib/display/display_draw.h"

// Das Fenster belegt die linke Haelfte: Pixel 0 bis 63.
// Rechts davon, ab Pixel 64, bleibt Platz fuer Text aus display.h.
enum { BREITE = 64, HOEHE = 64 };

int main(void)
{
	uint8_t punkte = 42;

	display_draw_init(BREITE, HOEHE, 0, 0);
	display_draw_rect(0, 0, BREITE, HOEHE, false);
	display_draw_string_P(10, 29, PSTR("SPIELFELD"), FONT_SIZE_1X);
	display_draw_show();

	// Hier wird in Zeichen gerechnet, nicht in Pixel. Der 3x5-Satz braucht
	// 4 Pixel je Zeichen, Spalte 16 liegt also bei Pixel 64 und damit knapp
	// rechts neben dem Fenster. Eine Zeile ist eine Page, also 8 Pixel hoch.
	display_string_pos_P(16, 0, PSTR("PUNKTE"));
	display_printf_pos_P(16, 1, PSTR("%u"), punkte);
	display_string_pos_P(16, 3, PSTR("S0 = START"));

	while (1)
	{
		_delay_ms(100);
	}
}
