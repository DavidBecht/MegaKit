// beispiel: display_draw_rect
// titel: Rahmen, Flaeche und ein Loch
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 96, HOEHE = 48, X_POS = 16, Y_POS = 8 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	display_draw_rect(0, 0, BREITE, HOEHE, false);      // nur der Rahmen
	display_draw_rect(8, 8, 32, 32, true);              // gefuellte Flaeche
	display_clear_rect(16, 16, 16, 16, true);           // ein Loch hinein

	// Ein Balken, wie er fuer Energie- oder Ladeanzeigen gebraucht wird
	display_draw_rect(52, 12, 36, 10, false);
	display_draw_rect(54, 14, 22, 6, true);
	display_draw_string_P(52, 28, PSTR("60%"), FONT_SIZE_1X);

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
