// beispiel: display_string_pos_P
// titel: Text im Zeichenraster
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display.h"

int main(void)
{
	display_init();                       // ohne display_draw: display_init() noetig

	// Spalte und Zeile, nicht Pixel. Gezaehlt wird ab 0.
	display_string_pos_P(0, 0, PSTR("SPALTE 0, ZEILE 0"));
	display_string_pos_P(4, 2, PSTR("SPALTE 4, ZEILE 2"));
	display_string_pos_P(0, 4, PSTR("PLATZ:"));

	// Wie gross das Raster ist, haengt vom Zeichensatz ab.
	display_printf_pos_P(7, 4, PSTR("%u x %u"), display_chars(), display_lines());

	while (1)
	{
		_delay_ms(100);
	}
}
