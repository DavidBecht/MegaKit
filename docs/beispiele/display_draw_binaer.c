// beispiel: display_draw_binaer
// titel: Eine Bitoperation sichtbar machen
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	uint8_t wert = 0b00000101;

	display_draw_string_P(4,  2, PSTR("VORHER"), FONT_SIZE_1X);
	display_draw_binaer(4, 12, wert, FONT_SIZE_2X);

	wert |= (1 << 3);                      // Bit 3 setzen

	display_draw_string_P(4, 28, PSTR("NACH |= (1 << 3)"), FONT_SIZE_1X);
	display_draw_binaer(4, 36, wert, FONT_SIZE_2X);

	display_draw_show();

	DDRC  = 0xFF;                          // dieselben Bits auf den LEDs
	PORTC = wert;

	while (1)
	{
		_delay_ms(100);
	}
}
