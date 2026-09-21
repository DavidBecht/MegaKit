// beispiel: display_printf_pos_P
// titel: Zahlen in eine Tabelle schreiben
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display.h"

int main(void)
{
	display_init();

	display_string_pos_P(0, 0, PSTR("KANAL  WERT   VOLT"));

	// %u fuer uint, %3u rueckt auf drei Stellen ein, %% ergibt ein Prozentzeichen.
	for (uint8_t kanal = 0; kanal < 4; kanal++)
	{
		uint16_t wert = (uint16_t)(kanal * 256 + 12);
		uint16_t mv   = (uint16_t)((uint32_t)wert * 5000 / 1023);

		display_printf_pos_P(0, (uint8_t)(kanal + 2), PSTR("ADC%u  %4u  %u.%03u"),
		                     kanal, wert, mv / 1000, mv % 1000);
	}

	while (1)
	{
		_delay_ms(100);
	}
}
