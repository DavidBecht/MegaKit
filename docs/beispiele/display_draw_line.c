// beispiel: display_draw_line
// titel: Faecher und gestrichelte Linie
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	// Ein Faecher: alle Linien beginnen links unten.
	for (uint8_t x = 0; x < BREITE; x += 12)
	{
		display_draw_line(0, HOEHE - 1, x, 0);
	}

	// Mit Schrittweite wird die Linie gestrichelt.
	display_draw_line_with_step(0, HOEHE / 2, BREITE - 1, HOEHE / 2, 3);

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
