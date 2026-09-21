// beispiel: display_draw_pixel
// titel: Eine Wurfbahn aus einzelnen Pixeln
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	display_draw_line(0, HOEHE - 1, BREITE - 1, HOEHE - 1);   // Boden

	// Wurfbahn: y waechst mit dem Quadrat des Abstands zum Scheitelpunkt.
	// Ganzzahlig gerechnet, der ATmega16 hat keine Fliesskommaeinheit.
	for (uint8_t x = 0; x < BREITE; x++)
	{
		uint16_t abstand = (x > 64) ? (uint16_t)(x - 64) : (uint16_t)(64 - x);
		uint16_t y = (uint16_t)(6 + (abstand * abstand) / 110);

		if (y < HOEHE - 1)
		{
			display_draw_pixel(x, (uint8_t)y);
		}
	}

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
