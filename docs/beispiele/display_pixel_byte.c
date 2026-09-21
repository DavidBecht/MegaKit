// beispiel: display_pixel_byte
// titel: Balken ohne Zeichenflaeche
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display.h"

int main(void)
{
	display_init();
	display_string_pos_P(0, 0, PSTR("PEGEL"));

	// Ein Byte sind acht uebereinanderliegende Pixel, Bit 0 oben.
	// 0xFF ist ein voller Streifen, 0x81 nur oberstes und unterstes Pixel.
	for (uint8_t x = 0; x < 120; x++)
	{
		uint8_t hoehe = (uint8_t)(x / 15 + 1);          // 1 bis 8 Pixel
		uint8_t muster = (uint8_t)(0xFF << (8 - hoehe)); // von unten auffuellen

		display_pixel_byte(x, 40, muster);
	}

	// Fuer viele Byte nacheinander ist die Sammeluebertragung schneller:
	display_burst_start(0, 56);
	for (uint8_t x = 0; x < 128; x++)
	{
		display_burst_write((x % 8 == 0) ? 0xFF : 0x18);
	}
	display_burst_end();

	while (1)
	{
		_delay_ms(100);
	}
}
