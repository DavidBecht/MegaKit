/*-------------------------------------------------------------------------*\
| Datei:        megalib_test.c
| Version:      1.0
| Projekt:      Pruefungen fuer megalib
| Beschreibung: Laeuft auf dem PC gegen die nachgebauten AVR-Header des
|               Simulators. Jeder Fall prueft genau eine Zusage der
|               Bibliothek, und zwar am sichtbaren Ergebnis im Bildspeicher.
|               Start ueber: python tests/run_tests.py
| Autor:        David Bechtold
| Erstellung:   14.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/
#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "megalib/display/display_draw.h"
#include "megalib/display/display_draw_sprite.h"

extern volatile uint8_t sim_framebuffer[8][128];

static int fehler = 0;
static int geprueft = 0;

static void behaupte(int bedingung, const char *was)
{
	geprueft++;
	if (!bedingung) { fehler++; printf("  FEHLER: %s\n", was); }
	else            { printf("  ok    : %s\n", was); }
}

static uint8_t pixel(uint8_t x, uint8_t y)
{
	return (sim_framebuffer[y / 8][x] >> (y % 8)) & 1u;
}

static uint16_t gesetzte_pixel(void)
{
	uint16_t n = 0;
	for (uint8_t p = 0; p < 8; p++)
		for (uint8_t x = 0; x < 128; x++)
			for (uint8_t b = 0; b < 8; b++)
				if (sim_framebuffer[p][x] & (1u << b)) n++;
	return n;
}

/* 4x16 Bitmap: erste Spalte voll, Rest leer. Damit laesst sich der
   Zeilenabstand pruefen, denn Zeile 2 beginnt bei Index 4. */
static const uint8_t Streifen[] = { 0xFF, 0x00, 0x00, 0x00,
                                    0x00, 0x00, 0x00, 0xFF };

BITMAP_VERTIKAL(Probebild, Streifen, 4, 16);

static SPRITE_T LeererSprite = { .x = 0, .y = 0 };   /* ohne frames */

int pruefungen(void)
{
	display_draw_init(120, 48, 4, 0);

	/* --- gefuelltes Rechteck: Breite und Hoehe nicht vertauscht --------- */
	display_draw_clear();
	display_draw_rect(0, 0, 20, 4, true);
	display_draw_show();
	behaupte(pixel(4 + 19, 3) == 1, "gefuelltes Rechteck 20x4: rechts unten gesetzt");
	display_draw_show();
	behaupte(pixel(4 + 4, 10) == 0, "gefuelltes Rechteck 20x4: nicht hochkant");

	/* --- Rechteck laeuft nicht ueber 255 hinaus ------------------------- */
	display_draw_clear();
	display_draw_rect(0, 40, 10, 200, false);   /* 40 + 200 - 1 = 239 */
	display_draw_show();
	behaupte(pixel(4 + 0, 3) == 0, "Rahmen mit riesiger Hoehe: keine Kante im Bild");

	/* --- gefuelltes Rechteck ebenso ------------------------------------- */
	display_draw_clear();
	display_draw_rect(0, 44, 10, 250, true);
	display_draw_show();
	behaupte(pixel(4 + 0, 2) == 0, "voll mit riesiger Hoehe: nichts oben");
	display_draw_show();
	behaupte(pixel(4 + 0, 47) == 1, "voll mit riesiger Hoehe: unten gefuellt");

	/* --- Bitmap am rechten Rand: Zeilenabstand bleibt richtig ----------- */
	display_draw_clear();
	display_draw_bitmap(117, 0, &Probebild);   /* 3 von 4 Spalten sichtbar */
	display_draw_show();
	behaupte(pixel(4 + 117, 0) == 1,  "beschnittenes Bitmap: Spalte 0 oben gesetzt");
	display_draw_show();
	behaupte(pixel(4 + 117, 8) == 0,  "beschnittenes Bitmap: Zeile 2 nicht verrutscht");
	display_draw_show();
	behaupte(pixel(4 + 119, 8) == 0,  "beschnittenes Bitmap: keine falsche Spalte");

	/* --- Linie wird beschnitten, nicht geklemmt ------------------------- */
	display_draw_clear();
	display_draw_line(0, 0, 200, 20);      /* Endpunkt weit rechts draussen */
	display_draw_show();
	behaupte(pixel(4 + 0, 0) == 1, "lange Linie: beginnt am Ursprung");
	display_draw_show();
	behaupte(pixel(4 + 119, 20) == 0,
	         "lange Linie: erreicht bei x=119 noch nicht y=20 (Steigung erhalten)");

	/* --- waagrechte Linie ueber den Rand hinaus ------------------------- */
	display_draw_clear();
	display_draw_line(0, 5, 200, 5);
	display_draw_show();
	behaupte(pixel(4 + 119, 5) == 1, "waagrechte Linie: bis zum rechten Rand");
	behaupte(gesetzte_pixel() == 120, "waagrechte Linie: genau 120 Pixel");

	/* --- Sprite ohne Bilder wird abgewiesen ----------------------------- */
	display_draw_sprite_reset();
	display_draw_sprite_register(&LeererSprite);
	display_draw_sprite_register(0);
	behaupte(LeererSprite.active == false, "Sprite ohne frames wird nicht aufgenommen");

	/* --- Bildtakt: FPS_15 muss wirklich 15 sein ------------------------- */
	{
		display_draw_sprite_init(FPS_15);
		const uint8_t cs = TCCR1B_reg & 0x07u;
		const uint16_t teiler = (cs == 2) ? 8 : (cs == 3) ? 64 : (cs == 4) ? 256 : 0;
		const uint32_t rate = teiler ? (12000000UL / ((uint32_t)teiler * (OCR1A_reg + 1UL))) : 0;
		printf("        OCR1A=%u Vorteiler=%u -> %lu Bilder/s\n",
		       (unsigned)OCR1A_reg, (unsigned)teiler, (unsigned long)rate);
		behaupte(rate >= 14 && rate <= 16, "FPS_15 ergibt 15 Bilder je Sekunde");

		display_draw_sprite_init(FPS_60);
		const uint8_t cs2 = TCCR1B_reg & 0x07u;
		const uint16_t t2 = (cs2 == 2) ? 8 : (cs2 == 3) ? 64 : (cs2 == 4) ? 256 : 0;
		const uint32_t r2 = t2 ? (12000000UL / ((uint32_t)t2 * (OCR1A_reg + 1UL))) : 0;
		behaupte(r2 >= 59 && r2 <= 61, "FPS_60 ergibt 60 Bilder je Sekunde");
	}

	/* --- animation_ms gilt fuer den ganzen Durchlauf -------------------- */
	{
		static const BITMAP_T * const Sechs[6] PROGMEM = {
			&Probebild, &Probebild, &Probebild, &Probebild, &Probebild, &Probebild };

		/* 800 ms fuer sechs Bilder sind bei 30 Bildern je Sekunde
		   vier Bilder je Schritt, also 6 * 4 / 30 = 0,8 Sekunden. */
		display_draw_sprite_init(FPS_30);
		SPRITE_T a = { .frames = Sechs,
		               .frame_count = 6, .animation_ms = 800 };
		display_draw_sprite_reset();
		display_draw_sprite_register(&a);
		behaupte(a.intern.frame_interval == 4,
		         "800 ms auf 6 Bilder bei 30 B/s: 4 Bilder je Schritt");
		behaupte(6 * a.intern.frame_interval == 24,
		         "ergibt 24 Bilder, also genau 800 ms");

		/* Dieselbe Angabe bei doppelter Bildrate muss dieselbe Zeit ergeben. */
		display_draw_sprite_init(FPS_60);
		SPRITE_T b = a;
		display_draw_sprite_reset();
		display_draw_sprite_register(&b);
		behaupte(b.intern.frame_interval == 8,
		         "dieselbe Angabe bei 60 B/s: 8 Bilder je Schritt");
		behaupte(6 * b.intern.frame_interval * 1000u / 60u == 800u,
		         "Dauer bleibt bei anderer Bildrate gleich");

		/* Halbiert man die Bildanzahl, bleibt der Durchlauf gleich lang. */
		static const BITMAP_T * const Drei[3] PROGMEM = {
			&Probebild, &Probebild, &Probebild };
		SPRITE_T c = { .frames = Drei,
		               .frame_count = 3, .animation_ms = 800 };
		display_draw_sprite_reset();
		display_draw_sprite_register(&c);
		behaupte(3 * c.intern.frame_interval == 6 * b.intern.frame_interval,
		         "halb so viele Bilder, gleiche Gesamtdauer");

		/* spawn_delay_ms ist eine einzelne Wartezeit und wird NICHT geteilt. */
		SPRITE_T d = { .frames = Sechs,
		               .frame_count = 6, .animation_ms = 800,
		               .spawn_delay_ms = 1000 };
		display_draw_sprite_reset();
		display_draw_sprite_register(&d);
		behaupte(d.intern.spawn_interval == 60,
		         "spawn_delay_ms 1000 bei 60 B/s: 60 Bilder, nicht geteilt");
	}

	/* --- Bilder mit Beschreiber ---------------------------------------- */
	{
		/* Dasselbe Bild wie oben, diesmal ueber einen Beschreiber gezeichnet.
		   Das Ergebnis muss Pixel fuer Pixel gleich sein. */
		display_draw_clear();
		display_draw_bitmap(10, 0, &Probebild);
		display_draw_show();
		const uint16_t alt = gesetzte_pixel();
		behaupte(alt == 16, "Bild mit Beschreiber: 16 Pixel gesetzt");
		behaupte(pixel(4 + 10, 0) == 1 && pixel(4 + 10, 8) == 0,
		         "Beschreiber: Zeilenabstand stimmt");

		/* Transparent darf nichts wegnehmen, deckend schon. */
		display_draw_clear();
		display_draw_rect(10, 0, 4, 16, true);
		display_draw_bitmap_transparent(10, 0, &Probebild);
		display_draw_show();
		behaupte(gesetzte_pixel() == 4 * 16,
		         "transparent gezeichnet bleibt der Untergrund stehen");

		display_draw_clear();
		display_draw_rect(10, 0, 4, 16, true);
		display_draw_bitmap(10, 0, &Probebild);
		display_draw_show();
		behaupte(gesetzte_pixel() == alt,
		         "deckend gezeichnet ersetzt den Untergrund");

		/* Loeschen nimmt genau die Pixel des Bildes weg. */
		display_clear_bitmap(10, 0, &Probebild);
		display_draw_show();
		behaupte(gesetzte_pixel() == 0, "display_clear_bitmap raeumt alles weg");

		/* Ein Nullzeiger darf nichts tun und nicht abstuerzen. */
		display_draw_bitmap(0, 0, 0);
		display_draw_bitmap_transparent(0, 0, 0);
		display_clear_bitmap(0, 0, 0);
		display_draw_show();
		behaupte(gesetzte_pixel() == 0, "NULL als Bild zeichnet nichts");
	}

	printf("\n%d von %d Pruefungen bestanden.\n", geprueft - fehler, geprueft);
	return fehler;
}
