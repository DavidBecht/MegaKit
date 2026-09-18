/*-------------------------------------------------------------------------*\
| Datei:        selftest.c
| Version:      1.0
| Projekt:      Einfuehrung in das Programmieren des Atmega16
| Beschreibung: Selbsttest der Zeichenbibliothek. Zeigt nacheinander alle
|               Zeichenfunktionen. Weiter jeweils mit Taster S0 (PA0).
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   11.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <util/delay.h>
#include <stdbool.h>

#include "../megalib/display/display_draw.h"
#include "../megalib/display/animations/lama.h"

// --- Taster -----------------------------------------------------------------
// true, solange S0 gedrueckt ist. Die Taster sind active low, ein
// gedrueckter Taster zieht sein Bit auf 0.
static bool _taster_down(void)
{
	return (PINA & (1 << PA0)) == 0;   // active low
}

// Wartet auf einen sauberen Tastendruck: druecken, entprellen, loslassen.
// Erst danach geht es weiter, sonst wuerde ein einziger Druck durch mehrere
// Testbilder springen.
static void _wait_taster_pressed(void)
{
	for (;;)
	{
		// warten bis gedrueckt
		while (!_taster_down()) { _delay_ms(10); }

		// entprellen: muss nach kurzer Zeit noch gedrueckt sein
		_delay_ms(20);
		if (_taster_down()) break;
	}

	// warten bis losgelassen
	while (_taster_down()) { _delay_ms(10); }

	// entprellen release
	_delay_ms(20);
}

// --- Bilder fuer Test 7 -----------------------------------------------------
// Stehen auf Dateiebene, nicht im Funktionsrumpf: ein Bild im Flash braucht
// einen festen Platz, eine Variable innerhalb einer Funktion hat keinen.
static const uint8_t smiley32x32_daten[] PROGMEM =
{
	0x00, 0x00, 0x00, 0x00, 0x3f, 0xff, 0xfe, 0x00, 0x40, 0x00, 0x01, 0xe0, 0xb0, 0x00, 0x00, 0x30,
	0x20, 0x00, 0x00, 0x18, 0x60, 0x00, 0x00, 0x08, 0x40, 0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x04,
	0x40, 0x00, 0x00, 0x02, 0x41, 0xe0, 0x1f, 0x02, 0x46, 0x20, 0x1c, 0xc2, 0x46, 0x20, 0x10, 0x62,
	0x41, 0xe0, 0x30, 0x22, 0x40, 0x00, 0x1f, 0xe2, 0x40, 0x00, 0x00, 0x04, 0x40, 0x00, 0x00, 0x04,
	0x40, 0x02, 0x00, 0x04, 0x60, 0x02, 0x00, 0x08, 0x20, 0x02, 0x00, 0x08, 0x21, 0x00, 0x00, 0x10,
	0x31, 0x80, 0x01, 0x30, 0x10, 0x80, 0x06, 0x20, 0x0c, 0x60, 0x0c, 0x40, 0x06, 0x1c, 0x30, 0xc0,
	0x03, 0x03, 0xc0, 0x80, 0x01, 0x80, 0x03, 0x00, 0x00, 0x70, 0x1e, 0x00, 0x00, 0x0f, 0xf0, 0x00,
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00
};
BITMAP_HORIZONTAL(smiley32x32, smiley32x32_daten, 32, 32);
static const uint8_t t_block_16x8_daten[] PROGMEM =
{
	0b00001111, 0b11111111,
	0b00001001, 0b10011001,
	0b00001001, 0b10011001,
	0b00001111, 0b11111111,
	0b00000000, 0b11110000,
	0b00000000, 0b10010000,
	0b00000000, 0b10010000,
	0b00000000, 0b11110000
};
BITMAP_HORIZONTAL(t_block_16x8, t_block_16x8_daten, 16, 8);

static const uint8_t flower_32x32_daten[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xf0, 0x00, 0x00, 0x03, 0x9c, 0x00, 0x00, 0x02, 0x06, 0x00, 0x00, 0x06, 0x02, 0x00,
	0x03, 0xfc, 0x02, 0x00, 0x06, 0x06, 0x02, 0x00, 0x04, 0x00, 0x06, 0x00, 0x04, 0x00, 0x04, 0x00,
	0x04, 0x00, 0x0f, 0x00, 0x06, 0x03, 0x81, 0x00, 0x03, 0x03, 0x81, 0x80, 0x01, 0x83, 0x81, 0x00,
	0x03, 0xe0, 0x03, 0x00, 0x04, 0x00, 0x0e, 0x00, 0x04, 0x00, 0x0f, 0x00, 0x04, 0x00, 0x05, 0x00,
	0x06, 0x0c, 0x04, 0x80, 0x03, 0x1c, 0x04, 0xc0, 0x01, 0xb4, 0x04, 0x40, 0x00, 0xe4, 0x04, 0x60,
	0x00, 0x04, 0x08, 0x20, 0x00, 0x03, 0x08, 0x30, 0x00, 0x01, 0xf0, 0x10, 0x00, 0x00, 0x00, 0x10,
	0x00, 0x00, 0x00, 0x10, 0x00, 0x00, 0x00, 0x18, 0x00, 0x00, 0x00, 0x08, 0x00, 0x00, 0x00, 0x08
};
BITMAP_HORIZONTAL(flower_32x32, flower_32x32_daten, 32, 32);


// --- Programm ---------------------------------------------------------------
// Zeigt nacheinander alle Zeichenfunktionen der Bibliothek, je ein Bild pro
// Tastendruck auf S0. Laeuft endlos und kehrt nicht zurueck.
void selftest_run(void)
{
	DDRA  &= ~(1<<PA0);
	PORTA |=  (1<<PA0);
	display_draw_init(120, 56, 2, 2);

	// Groesse der Zeichenflaeche ueber die oeffentliche Schnittstelle holen
	const uint8_t disp_w = display_draw_get_width();
	const uint8_t disp_h = display_draw_get_height();

	// wir zeichnen unterhalb der Statuszeile
	const uint8_t y0 = 10; // Abstand nach oben (Font 5x8 + spacing)
	const uint8_t y_mid  = (uint8_t)(y0 + (disp_h - y0) / 2);
	const uint8_t y_last = (uint8_t)(disp_h - 1);

	for (;;)
	{
		// -------------------------------------------------
		// 1) Rect Rahmen + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("1) rect frame"), FONT_SIZE_1X);
		display_draw_rect(0, y0, disp_w, disp_h - y0, false);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("1) clear rect frame"), FONT_SIZE_1X);
		display_clear_rect(0, y0, disp_w, disp_h - y0, false);
		_wait_taster_pressed();

		// -------------------------------------------------
		// 2) Pixel + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("2) pixels"), FONT_SIZE_1X);
		display_draw_pixel(0, y0);
		display_draw_pixel((uint8_t)(disp_w - 1), y0);
		display_draw_pixel(0, (uint8_t)(disp_h - 1));
		display_draw_pixel((uint8_t)(disp_w - 1), (uint8_t)(disp_h - 1));

		uint8_t d = (disp_w < (disp_h - y0)) ? disp_w : (uint8_t)(disp_h - y0);
		for (uint8_t i = 0; i < d; i++)
		display_draw_pixel(i, (uint8_t)(y0 + i));
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("2) clear pixels"), FONT_SIZE_1X);
		display_clear_pixel(0, y0);
		display_clear_pixel((uint8_t)(disp_w - 1), y0);
		display_clear_pixel(0, (uint8_t)(disp_h - 1));
		display_clear_pixel((uint8_t)(disp_w - 1), (uint8_t)(disp_h - 1));
		for (uint8_t i = 0; i < d; i++)
		display_clear_pixel(i, (uint8_t)(y0 + i));
		_wait_taster_pressed();

		// -------------------------------------------------
		// 3) Byte-Muster + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("3) bytes AA/55"), FONT_SIZE_1X);
		for (uint8_t x = 0; x < disp_w; x += 2)
		{
			display_draw_byte(x, y0, 0xAA);
			if ((uint8_t)(x + 1) < disp_w)
			display_draw_byte((uint8_t)(x + 1), y0, 0x55);
		}
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("3) clear bytes"), FONT_SIZE_1X);
		for (uint8_t x = 0; x < disp_w; x += 2)
		{
			display_clear_byte(x, y0, 0xFF);
			if ((uint8_t)(x + 1) < disp_w)
			display_clear_byte((uint8_t)(x + 1), y0, 0xFF);
		}
		_wait_taster_pressed();

		// -------------------------------------------------
		// 4) hline / vline + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("4) hline/vline"), FONT_SIZE_1X);
		// waagrecht: oben, Mitte, unten
		display_draw_line(0, y0,     (uint8_t)(disp_w - 1), y0);
		display_draw_line(0, y_mid,  (uint8_t)(disp_w - 1), y_mid);
		display_draw_line(0, y_last, (uint8_t)(disp_w - 1), y_last);

		// senkrecht: links, Mitte, rechts
		display_draw_line(0,                     y0, 0,                     y_last);
		display_draw_line((uint8_t)(disp_w / 2), y0, (uint8_t)(disp_w / 2), y_last);
		display_draw_line((uint8_t)(disp_w - 1), y0, (uint8_t)(disp_w - 1), y_last);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("4) clear hline/vline"), FONT_SIZE_1X);
		display_clear_line(0, y0,     (uint8_t)(disp_w - 1), y0);
		display_clear_line(0, y_mid,  (uint8_t)(disp_w - 1), y_mid);
		display_clear_line(0, y_last, (uint8_t)(disp_w - 1), y_last);

		display_clear_line(0,                     y0, 0,                     y_last);
		display_clear_line((uint8_t)(disp_w / 2), y0, (uint8_t)(disp_w / 2), y_last);
		display_clear_line((uint8_t)(disp_w - 1), y0, (uint8_t)(disp_w - 1), y_last);
		_wait_taster_pressed();

		// -------------------------------------------------
		// 5) Linien + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("5) lines"), FONT_SIZE_1X);
		display_draw_line(0, y0, (uint8_t)(disp_w - 1), (uint8_t)(disp_h - 1));
		display_draw_line(0, (uint8_t)(disp_h - 1), (uint8_t)(disp_w - 1), y0);
		display_draw_line_with_step(0, (uint8_t)(y0 + (disp_h - y0)/2),
		(uint8_t)(disp_w - 1),
		(uint8_t)(y0 + (disp_h - y0)/2), 2);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("5) clear lines"), FONT_SIZE_1X);
		display_clear_line(0, y0, (uint8_t)(disp_w - 1), (uint8_t)(disp_h - 1));
		display_clear_line(0, (uint8_t)(disp_h - 1), (uint8_t)(disp_w - 1), y0);
		display_clear_line_with_step(0, (uint8_t)(y0 + (disp_h - y0)/2),
		(uint8_t)(disp_w - 1),
		(uint8_t)(y0 + (disp_h - y0)/2), 2);
		_wait_taster_pressed();

		// -------------------------------------------------
		// 6) Rechtecke (frame + filled) + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("6) rect frame"), FONT_SIZE_1X);
		display_draw_rect(4, y0 + 2, (uint8_t)(disp_w - 8), (uint8_t)(disp_h - y0 - 4), false);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("6) clear rect frame"), FONT_SIZE_1X);
		display_clear_rect(4, y0 + 2, (uint8_t)(disp_w - 8), (uint8_t)(disp_h - y0 - 4), false);
		_wait_taster_pressed();

		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("6) rect filled"), FONT_SIZE_1X);
		display_draw_rect(10, y0 + 6, (uint8_t)(disp_w - 20), (uint8_t)(disp_h - y0 - 12), true);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("6) clear rect filled"), FONT_SIZE_1X);
		display_clear_rect(10, y0 + 6, (uint8_t)(disp_w - 20), (uint8_t)(disp_h - y0 - 12), true);
		_wait_taster_pressed();

		// -------------------------------------------------
		// 7) Bitmaps + Clear
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("7) bitmaps"), FONT_SIZE_1X);

		display_draw_bitmap(20, y0, &smiley32x32);
		display_draw_bitmap(60, y0, &t_block_16x8);
		display_draw_bitmap(80, 20, &flower_32x32);
		_wait_taster_pressed();

		display_draw_string_P(0, 0, PSTR("7) clear bitmaps"), FONT_SIZE_1X);
		display_clear_bitmap(20, y0, &smiley32x32);
		display_clear_bitmap(60, y0, &t_block_16x8);
		display_clear_bitmap(80, 20, &flower_32x32);
		_wait_taster_pressed();

		// -------------------------------------------------
		// 8) Text + Clear (clear_bg test)
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("8) text"), FONT_SIZE_1X);
		display_draw_string_P(0, y0, PSTR("Font 0.75x"), FONT_SIZE_0_75X);
		display_draw_string_P(0, y0 + 6, PSTR("Font 1x"), FONT_SIZE_1X);
		display_draw_string_P(0, y0 + 14, PSTR("1.5x"), FONT_SIZE_1_5X);
		display_draw_string_P(0, y0 + 26, PSTR("2x"), FONT_SIZE_2X);
		_wait_taster_pressed();
		
		// -------------------------------------------------
		// 9) Animationen
		// -------------------------------------------------
		display_draw_clear();
		display_draw_string_P(0, 0, PSTR("9) animations"), FONT_SIZE_1X);
		uint8_t prev = 0;
		while ((PINA & (1<<PA0)) == 1)
		{
			for(uint8_t i = 0; i < LAMA_BILDER; i++)
			{
				display_clear_bitmap(0, y0, (const BITMAP_T *)pgm_read_ptr(&lama_bilder[prev]));
				display_draw_bitmap(0, y0, (const BITMAP_T *)pgm_read_ptr(&lama_bilder[i]));
				prev = i;
				_delay_ms(80);
			}
		}
	}
}
