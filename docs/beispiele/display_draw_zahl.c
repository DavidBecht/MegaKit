// beispiel: display_draw_zahl
// titel: Punktestand ohne printf
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	uint16_t punkte = 1234;

	display_draw_string_P(4, 4, PSTR("PUNKTE"), FONT_SIZE_1X);
	display_draw_zahl(4, 14, punkte, FONT_SIZE_3X);

	// display_draw_zahl kostet nur ein paar hundert Byte Flash,
	// display_draw_printf dagegen rund 1,5 KByte.
	display_draw_string_P(4, 40, PSTR("BESTWERT"), FONT_SIZE_1X);
	display_draw_zahl(56, 40, 9999, FONT_SIZE_1X);

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
