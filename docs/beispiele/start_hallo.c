// beispiel: start_beispiel
// titel: Hallo MEGACARD
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

// Die Zeichenflaeche: 96 x 48 Pixel, mittig auf der 128 x 64 grossen Anzeige.
// Sie kostet (48 / 8) * 96 = 576 Byte SRAM.
enum { BREITE = 96, HOEHE = 48, X_POS = 16, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	display_draw_rect(0, 0, BREITE, HOEHE, false);          // Rahmen
	display_draw_string_P(14, 14, PSTR("HALLO"), FONT_SIZE_2X);
	display_draw_string_P(14, 30, PSTR("MEGACARD"), FONT_SIZE_1X);
	display_draw_show();                                    // erst jetzt sichtbar

	while (1)
	{
		_delay_ms(100);
	}
}
