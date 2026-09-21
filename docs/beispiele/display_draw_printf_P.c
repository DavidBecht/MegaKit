// beispiel: display_draw_printf_P
// titel: Zeit und Punkte in einer Zeile
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	uint16_t punkte = 42;
	uint16_t zehntel = 1057;               // 105,7 Sekunden

	// Der Formatstring liegt im Flash und kostet kein SRAM.
	display_draw_printf_P(4, 4, FONT_SIZE_1X, PSTR("PUNKTE %u"), punkte);
	display_draw_printf_P(4, 14, FONT_SIZE_1X, PSTR("ZEIT %u.%u S"),
	                      zehntel / 10, zehntel % 10);
	display_draw_printf_P(4, 24, FONT_SIZE_2X, PSTR("%u/%u"), punkte, 50);

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
