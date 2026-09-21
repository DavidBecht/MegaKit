// beispiel: sound_ton
// titel: Piepser auf Tastendruck
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"
#include "megalib/sound/sound.h"

enum { BREITE = 96, HOEHE = 24, X_POS = 16, Y_POS = 20 };

int main(void)
{
	DDRA  &= (uint8_t)~0x0F;           // S0 bis S3 als Eingang
	PORTA |= 0x0F;                     // Pull-ups ein

	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);
	sound_init();

	display_draw_string_P(4, 8, PSTR("TASTE DRUECKEN"), FONT_SIZE_1X);
	display_draw_show();

	while (1)
	{
		if (!(PINA & (1 << PA0)))      // S0 gedrueckt
		{
			// Tonhoehe, Vorteiler, Dauer in Ticks zu je 10 ms
			sound_ton(106, 3, 8);
			_delay_ms(120);
		}
	}
}
