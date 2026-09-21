// beispiel: BITMAP_VERTIKAL
// titel: Ein eigenes Bild anlegen und zeichnen
// bild: 0.8s
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"

enum { BREITE = 128, HOEHE = 48, X_POS = 0, Y_POS = 8 };

// So sieht das Bild aus. Ein Byte sind acht uebereinanderliegende Pixel,
// Bit 0 oben; die Bytes laufen zeilenweise von links nach rechts.
//  ####     ####
// ###############
// ################
// ################
// ################
// ################
// ################
//  ##############
//   ############
//    ##########
//     ########
//      ######
//       ####
//        ##
static const uint8_t Herz_daten[] PROGMEM = {
	0x7E, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0xFE, 0xFE, 0xFE, 0xFE, 0xFF, 0xFF, 0xFF, 0xFF, 0xFE, 0x7C,
	0x00, 0x00, 0x01, 0x03, 0x07, 0x0F, 0x1F, 0x3F, 0x3F, 0x1F, 0x0F, 0x07, 0x03, 0x01, 0x00, 0x00
};

// Das Makro prueft beim Uebersetzen, ob die Tabelle zu 16 x 14 passt.
BITMAP_VERTIKAL(Herz, Herz_daten, 16, 14);

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);

	display_draw_bitmap(8, 8, &Herz);                  // deckend

	display_draw_rect(48, 4, 72, 40, true);            // heller Untergrund
	display_clear_bitmap(56, 12, &Herz);               // Loch in die Flaeche

	display_draw_string_P(84, 20, PSTR("LEBEN"), FONT_SIZE_1X);
	display_draw_show();

	while (1)
	{
		_delay_ms(100);
	}
}
