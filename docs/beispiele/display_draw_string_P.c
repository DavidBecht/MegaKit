// beispiel: display_draw_string_P
// titel: Drei Schriftgroessen
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	// PSTR legt den Text im Flash ab: er kostet dann kein SRAM.
	display_draw_string_P(2,  2, PSTR("KLEIN 1X"), FONT_SIZE_1X);
	display_draw_string_P(2, 12, PSTR("MITTEL"), FONT_SIZE_2X);
	display_draw_string_P(2, 30, PSTR("GROSS"), FONT_SIZE_3X);

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
