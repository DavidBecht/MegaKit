// beispiel: sprites_takt
// titel: Ein Sprite faehrt von selbst
// bild: 1.6s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"
#include "megalib/display/display_draw_sprite.h"

enum { BREITE = 128, HOEHE = 32, X_POS = 0, Y_POS = 16 };

// Zwei Bilder fuer die Animation: die Raeder wechseln.
//   ######
//  ########
// ##########
// ##########
//  ##    ##
static const uint8_t Auto1_daten[] PROGMEM = {
	0x0C, 0x1E, 0x1F, 0x0F, 0x0F, 0x0F, 0x0F, 0x1F, 0x1E, 0x0C
};
//   ######
//  ########
// ##########
// ##########
//   ##  ##
static const uint8_t Auto2_daten[] PROGMEM = {
	0x0C, 0x0E, 0x1F, 0x1F, 0x0F, 0x0F, 0x1F, 0x1F, 0x0E, 0x0C
};

BITMAP_VERTIKAL(Auto1, Auto1_daten, 10, 5);
BITMAP_VERTIKAL(Auto2, Auto2_daten, 10, 5);

// Die Liste der Einzelbilder liegt ebenfalls im Flash.
static const BITMAP_T * const Auto_Bilder[] PROGMEM = { &Auto1, &Auto2 };

// Ein Sprite ist eine gewoehnliche Variable. Nicht genannte Felder sind 0.
static SPRITE_T auto_sprite = {
	.x = -10, .y = 14,
	.vx = 2,                       // zwei Pixel je Bild nach rechts
	.frames = Auto_Bilder,
	.frame_count = 2,
	.animation_ms = 200,           // beide Bilder zusammen in 200 ms
	.respawn_x = -10, .respawn_y = 14,
	.spawn_delay_ms = 400,         // nach dem Hinauslaufen neu starten
};

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);
	display_draw_string_P(4, 4, PSTR("STRASSE"), FONT_SIZE_1X);
	display_draw_line_with_step(0, 26, BREITE - 1, 26, 4);

	display_draw_sprite_init(FPS_30);          // Timer1 gibt den Takt
	display_draw_sprite_register(&auto_sprite);

	while (1)
	{
		// Loeschen, weiterruecken, neu zeichnen und anzeigen:
		// alles in diesem einen Aufruf. true wartet auf das naechste Bild.
		display_draw_sprite_update_all(true);
	}
}
