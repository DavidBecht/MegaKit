// beispiel: display_draw_sprite_collides
// titel: Zwei Sprites treffen sich
// bild: 2.0s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"
#include "megalib/display/display_draw_sprite.h"

enum { BREITE = 128, HOEHE = 32, X_POS = 0, Y_POS = 16 };

//  ####
// ######
// ######
// ######
// ######
//  ####
static const uint8_t Ball_daten[] PROGMEM = {
	0x1E, 0x3F, 0x3F, 0x3F, 0x3F, 0x1E
};
BITMAP_VERTIKAL(Ball, Ball_daten, 6, 6);

static const BITMAP_T * const Ball_Bilder[] PROGMEM = { &Ball };

static SPRITE_T links  = { .x = 10,  .y = 4, .vx =  2,
                           .frames = Ball_Bilder, .frame_count = 1 };
static SPRITE_T rechts = { .x = 110, .y = 4, .vx = -2,
                           .frames = Ball_Bilder, .frame_count = 1 };

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);
	display_draw_sprite_init(FPS_30);
	display_draw_sprite_register(&links);
	display_draw_sprite_register(&rechts);

	while (1)
	{
		display_draw_sprite_update_all(true);

		// Verglichen werden die Rechtecke der beiden Sprites.
		if (display_draw_sprite_collides(&links, &rechts))
		{
			links.vx = 0;
			rechts.vx = 0;
			display_draw_string_P(36, 18, PSTR("TREFFER"), FONT_SIZE_2X);
			display_draw_show();
		}
	}
}
