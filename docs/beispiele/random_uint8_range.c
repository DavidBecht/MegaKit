// beispiel: random_uint8_range
// titel: Fuenf Wuerfe
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"
#include "megalib/display/random.h"

enum { BREITE = 128, HOEHE = 40, X_POS = 0, Y_POS = 12 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	// Ohne random_init() wuerfelt der Controller nach jedem Einschalten
	// dieselbe Folge.
	random_init();

	display_draw_string_P(2, 2, PSTR("WUERFEL"), FONT_SIZE_1X);

	for (uint8_t wurf = 0; wurf < 5; wurf++)
	{
		// 1 bis 6: die obere Grenze ist ausgeschlossen.
		uint8_t augen = random_uint8_range(1, 7);

		display_draw_rect((uint8_t)(2 + wurf * 26), 14, 22, 22, false);
		display_draw_zahl((uint8_t)(9 + wurf * 26), 20, augen, FONT_SIZE_2X);
	}

	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
