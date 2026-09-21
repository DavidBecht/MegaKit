// beispiel: display_scroll_up
// titel: Eine Zeile nachschieben
// bild: 1.6s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display.h"

int main(void)
{
	display_init();

	// Das Display fuellen: eine Zeile je Durchlauf, ganz unten.
	// Ist es voll, macht display_scroll_up() unten wieder Platz.
	uint8_t letzte = (uint8_t)(display_lines() - 1);

	for (uint8_t nummer = 1; nummer <= 20; nummer++)
	{
		if (nummer > display_lines())
		{
			display_scroll_up();
		}
		uint8_t zeile = (nummer <= display_lines()) ? (uint8_t)(nummer - 1) : letzte;
		display_printf_pos_P(0, zeile, PSTR("MESSUNG %u"), nummer);
		_delay_ms(60);
	}

	while (1)
	{
		_delay_ms(100);
	}
}
