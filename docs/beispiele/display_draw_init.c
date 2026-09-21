// beispiel: display_draw_init
// titel: Ein kleines Fenster anlegen
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

// Alle vier Werte muessen beim Uebersetzen feststehen. Mit einem enum stehen
// sie an einer Stelle und lassen sich im ganzen Programm verwenden.
enum { BREITE = 64, HOEHE = 24, X_POS = 32, Y_POS = 20 };

int main(void)
{
	// Videopuffer: ((24 + 7) / 8) * 64 = 192 Byte SRAM
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	// (0, 0) ist ab jetzt die linke obere Ecke DIESES Fensters,
	// nicht die des Displays.
	display_draw_rect(0, 0, BREITE, HOEHE, false);
	display_draw_pixel(0, 0);
	display_draw_string_P(6, 6, PSTR("FENSTER"), FONT_SIZE_1X);
	display_draw_printf_P(6, 14, FONT_SIZE_1X, PSTR("%u X %u"),
	                      display_draw_get_width(), display_draw_get_height());
	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
