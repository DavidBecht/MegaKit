// beispiel: display_draw_show
// titel: Die Schleife eines Spiels
// bild: 1.0s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	uint8_t x = 0;

	while (1)
	{
		// 1. altes Bild wegraeumen, 2. neues zeichnen, 3. anzeigen.
		// Nur einmal display_draw_show() je Bild, sonst flackert es.
		// Geloescht wird alles, was sich aendert -- auch die Textzeile,
		// sonst bleiben die Ziffern des letzten Bildes stehen.
		display_clear_rect(1, 14, BREITE - 2, HOEHE - 15, true);

		display_draw_rect(x, 16, 16, 16, true);
		display_draw_rect(0, 0, BREITE, HOEHE, false);
		display_draw_printf_P(4, 36, FONT_SIZE_1X, PSTR("X = %u"), x);

		display_draw_show();

		x = (uint8_t)((x + 4) % (BREITE - 16));
		_delay_ms(40);
	}
}
