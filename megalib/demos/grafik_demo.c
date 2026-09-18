/*-------------------------------------------------------------------------*\
| Datei:        grafik_demo.c
| Version:      1.0
| Projekt:      Einfuehrung in das Programmieren des Atmega16
| Beschreibung: Zeigt alle Zeichenfunktionen von display_draw an einem Stueck.
|               Erst ein Titel, dann eine Fuehrung durch die einzelnen
|               Werkzeuge, zuletzt eine Nachtlandschaft mit Bergbahn.
|               Sprites kommen hier bewusst nicht vor, alles wird von Hand
|               gezeichnet und von Hand wieder weggeraeumt.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   14.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdio.h>       
#include <stdbool.h>
#include <util/delay.h>

#include "../megalib/display/display_draw.h"

// --- Zeichenflaeche ---------------------------------------------------------
// 128x48 braucht 128 * 6 Pages = 768 Byte SRAM. Mehr geht nicht: fuer die
// Variablen und den Stack muss auch noch Platz bleiben.
enum {
	FENSTER_B = 128,
	FENSTER_H = 48,
	FENSTER_X = 0,
	FENSTER_Y = 0,
};

// Zeile ganz unten fuer die Beschriftung, darueber wird gezeichnet
enum {
	TEXT_Y   = 41,       // Oberkante der Beschriftung, 5 Pixel hoch
	BUEHNE_O = 2,        // oberste Zeile der Buehne
	BUEHNE_U = 38,       // unterste Zeile der Buehne
};


// --- Bilder im Flash --------------------------------------------------------
// Vertikal abgelegt, also im Format des Displays selbst. Ein Byte sind acht
// uebereinanderliegende Pixel, Bit 0 ist das oberste.

// Mondsichel am Nachthimmel
// 16x16 Pixel, vertikal abgelegt, 32 Byte im Flash
static const uint8_t Mond_daten[] PROGMEM = {
	0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x03, 0x06, 0x1E, 0xFE,
	0xFC, 0xF8, 0xF0, 0x80, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xC0,
	0xC0, 0x60, 0x78, 0x7F, 0x3F, 0x1F, 0x0F, 0x01,
};
BITMAP_VERTIKAL(Mond, Mond_daten, 16, 16);

// Gondel der Bergbahn, Aufhaengung oben, zwei dunkle Fenster
// 12x10 Pixel, vertikal abgelegt, 24 Byte im Flash
static const uint8_t Gondel_daten[] PROGMEM = {
	0xF0, 0xF8, 0x98, 0xFC, 0xFF, 0x9F, 0x9C, 0xF8, 0xF8, 0x98, 0xF8, 0xF0,
	0x00, 0x01, 0x03, 0x01, 0x01, 0x01, 0x01, 0x01, 0x01, 0x03, 0x01, 0x00,
};
BITMAP_VERTIKAL(Gondel, Gondel_daten, 12, 10);


// --- Vier Karos wachsender Groesse ------------------------------------------
// Dienen unten dem Vergleich zwischen deckend und transparent gezeichnet.
static const uint8_t Karo1_daten[] PROGMEM = { 0x00, 0x00, 0x08, 0x1C, 0x08, 0x00, 0x00, 0x00, };
BITMAP_VERTIKAL(Karo1, Karo1_daten, 8, 8);
static const uint8_t Karo2_daten[] PROGMEM = { 0x00, 0x08, 0x1C, 0x3E, 0x1C, 0x08, 0x00, 0x00, };
BITMAP_VERTIKAL(Karo2, Karo2_daten, 8, 8);
static const uint8_t Karo3_daten[] PROGMEM = { 0x08, 0x1C, 0x3E, 0x7F, 0x3E, 0x1C, 0x08, 0x00, };
BITMAP_VERTIKAL(Karo3, Karo3_daten, 8, 8);
static const uint8_t Karo4_daten[] PROGMEM = { 0x1C, 0x3E, 0x7F, 0xFF, 0x7F, 0x3E, 0x1C, 0x08, };
BITMAP_VERTIKAL(Karo4, Karo4_daten, 8, 8);

// Der Reihe nach, damit sie sich in einer Schleife durchlaufen lassen
static const BITMAP_T * const Karos[4] PROGMEM = { &Karo1, &Karo2, &Karo3, &Karo4 };


// --- Sterne -----------------------------------------------------------------
// Feste Plaetze am Himmel, x und y abwechselnd. Im Flash, das spart RAM.
static const uint8_t Sterne[] PROGMEM = {
	  7,  4,   15, 11,   24,  6,   32, 14,   41,  3,   48, 12,
	 57,  8,   65, 15,   72,  5,   81, 13,   89,  9,   97, 16,
	 13, 16,   36, 10,   60, 14,   85,  4,
};
#define STERNE_ANZAHL (sizeof(Sterne) / 2)


// --- Zeit -------------------------------------------------------------------
/* _delay_ms() braucht eine feste Zahl. Fuer eine Wartezeit, die sich erst zur
   Laufzeit ergibt, zaehlt man sie deshalb in festen Haeppchen ab. Zuerst in
   Zehnerschritten, den Rest einzeln. */
static void warten(uint16_t ms)
{
	while (ms >= 10)
	{
		_delay_ms(10);
		ms = (uint16_t)(ms - 10);
	}
	while (ms--)
	{
		_delay_ms(1);
	}
}


// --- Beschriftung -----------------------------------------------------------
/* Schreibt eine Zeile ganz unten und raeumt vorher die alte weg. Genau dafuer
   ist display_clear_rect da: es loescht einen Bereich, ohne den Rest des
   Bildes anzutasten. display_draw_clear() wuerde alles loeschen. */
static void beschriftung(PGM_P text)
{
	display_clear_rect(1, TEXT_Y, FENSTER_B - 2, 6, true);
	display_draw_string_P(3, TEXT_Y, text, FONT_SIZE_1X);
	display_draw_show();
}

/* Raeumt die Buehne, laesst Rahmen und Beschriftung stehen. */
static void buehne_leeren(void)
{
	display_clear_rect(1, BUEHNE_O - 1, FENSTER_B - 2,
	                   BUEHNE_U - BUEHNE_O + 2, true);
}


// ===========================================================================
//  Bild 1: der Titel
// ===========================================================================
static void titel(void)
{
	char zeile[22];

	display_draw_clear();

	// Rahmen um die ganze Zeichenflaeche. Die Masse holen wir uns von der
	// Bibliothek, statt sie ein zweites Mal hinzuschreiben.
	const uint8_t breite = display_draw_get_width();
	const uint8_t hoehe  = display_draw_get_height();
	display_draw_rect(0, 0, breite, hoehe, false);

	// Die Ueberschrift Buchstabe fuer Buchstabe. display_draw_char zeichnet
	// genau ein Zeichen, damit laesst sich der Aufbau in Ruhe zeigen.
	// Beim 3x5-Zeichensatz ist ein Zeichen dreifach vergroessert 9 Pixel
	// breit, dazu ein Pixel Abstand, macht 12 Pixel Vorschub.
	static const char wort[] PROGMEM = "MEGALIB";
	const uint8_t vorschub = 12;
	const uint8_t start_x  = (uint8_t)((breite - 7 * vorschub) / 2);

	for (uint8_t i = 0; i < 7; i++)
	{
		char c = (char)pgm_read_byte(&wort[i]);
		display_draw_char((uint8_t)(start_x + i * vorschub), 6, c,
		                  FONT_SIZE_3X, false);
		display_draw_show();
		warten(120);
	}

	// Untertitel aus dem Flash, eineinhalbfach vergroessert
	display_draw_string_P(30, 24, PSTR("Zeichendemo"), FONT_SIZE_1_5X);

	// Eine Zeile aus dem RAM: hier steht etwas drin, das erst zur Laufzeit
	// feststeht. Deshalb display_draw_string ohne _P.
	sprintf_P(zeile, PSTR("Fenster %ux%u ab %u,%u"),
	          breite, hoehe,
	          display_draw_get_x_pos(), display_draw_get_y_pos());
	display_draw_string(6, 34, zeile, FONT_SIZE_1X);

	display_draw_show();
	warten(2500);

	// Uebergang: eine senkrechte Linie wandert einmal durch das Bild und
	// loescht dabei alles hinter sich her.
	for (uint8_t x = 1; x < breite - 1; x += 2)
	{
		display_clear_line(x, 1, x, (uint8_t)(hoehe - 2));
		display_clear_line((uint8_t)(x + 1), 1, (uint8_t)(x + 1),
		                   (uint8_t)(hoehe - 2));
		display_draw_show();
		_delay_ms(8);
	}
}


// ===========================================================================
//  Bild 2: die Werkzeuge der Reihe nach
//  Jedes wird gezeichnet, kurz gezeigt und mit seinem Gegenstueck wieder
//  weggeraeumt. Zu jeder Zeichenfunktion gibt es genau eine zum Loeschen.
// ===========================================================================
static void werkzeuge(void)
{
	// --- Pixel --------------------------------------------------------
	beschriftung(PSTR("display_draw_pixel"));
	for (uint8_t i = 0; i < 60; i++)
	{
		display_draw_pixel((uint8_t)(10 + i), (uint8_t)(8 + (i % 20)));
	}
	display_draw_show();
	warten(900);

	beschriftung(PSTR("display_clear_pixel  jedes 2."));
	for (uint8_t i = 0; i < 60; i += 2)
	{
		display_clear_pixel((uint8_t)(10 + i), (uint8_t)(8 + (i % 20)));
	}
	display_draw_show();
	warten(1200);
	buehne_leeren();

	// --- Byte ---------------------------------------------------------
	// Ein Byte sind acht uebereinanderliegende Pixel auf einen Schlag.
	// Das ist der schnellste Weg, eine Flaeche zu fuellen.
	beschriftung(PSTR("display_draw_byte  Muster"));
	for (uint8_t x = 8; x < 120; x++)
	{
		display_draw_byte(x, 16, (x & 1) ? 0x55 : 0xAA);
	}
	display_draw_show();
	warten(900);

	beschriftung(PSTR("display_clear_byte  Schlitz"));
	for (uint8_t x = 8; x < 120; x++)
	{
		display_clear_byte(x, 16, 0x18);      // Bit 3 und 4 heraus
	}
	display_draw_show();
	warten(1200);
	buehne_leeren();

	// --- Linien -------------------------------------------------------
	beschriftung(PSTR("display_draw_line"));
	display_draw_line(10, 6, 118, 34);
	display_draw_line(10, 34, 118, 6);
	display_draw_show();
	warten(900);

	beschriftung(PSTR("display_clear_line  eine weg"));
	display_clear_line(10, 34, 118, 6);
	display_draw_show();
	warten(1000);

	// --- Gestrichelte Linien ------------------------------------------
	beschriftung(PSTR("display_draw_line_with_step"));
	display_draw_line_with_step(10, 10, 118, 10, 3);
	display_draw_line_with_step(10, 30, 118, 30, 5);
	display_draw_show();
	warten(1200);

	beschriftung(PSTR("display_clear_line_with_step"));
	display_clear_line_with_step(10, 10, 118, 10, 3);
	display_draw_show();
	warten(1000);
	buehne_leeren();

	// --- Rechtecke ----------------------------------------------------
	beschriftung(PSTR("display_draw_rect  Rahmen/voll"));
	display_draw_rect(10, 6, 40, 30, false);
	display_draw_rect(70, 6, 40, 30, true);
	display_draw_show();
	warten(1200);

	beschriftung(PSTR("display_clear_rect  Loch hinein"));
	display_clear_rect(80, 14, 20, 14, true);
	display_draw_show();
	warten(1200);
	buehne_leeren();

	// --- Bild aus dem RAM ---------------------------------------------
	// Das Karo entsteht erst beim Laufen, deshalb RAM und deshalb die
	// Funktionen ohne _P.
	beschriftung(PSTR("display_draw_bitmap  vier Bilder"));
	for (uint8_t g = 0; g < 4; g++)
	{
		display_draw_bitmap(20, 14, (const BITMAP_T *)pgm_read_ptr(&Karos[g]));
		display_draw_show();
		warten(300);
	}
	warten(600);

	// Gestreifter Untergrund. Auf einer vollen Flaeche waere kein Unterschied
	// zu sehen, denn transparent zeichnen heisst Pixel dazugeben.
	beschriftung(PSTR("deckend  vs.  transparent"));
	for (uint8_t x = 44; x < 116; x++)
	{
		display_draw_byte(x, 12, 0xAA);
	}
	display_draw_show();
	warten(900);

	// links deckend: das ganze Rechteck wird ersetzt, die Streifen
	// verschwinden also rund um das Karo
	display_draw_bitmap(56, 12, &Karo4);
	// rechts transparent: die Streifen bleiben stehen, das Karo kommt dazu
	display_draw_bitmap_transparent(96, 12, &Karo4);
	display_draw_show();
	warten(2500);

	beschriftung(PSTR("display_clear_bitmap  rechts"));
	display_clear_bitmap(96, 12, &Karo4);
	display_draw_show();
	warten(1500);
	buehne_leeren();

	// --- Bild aus dem Flash -------------------------------------------
	beschriftung(PSTR("display_draw_bitmap  aus dem Flash"));
	display_draw_bitmap(20, 12, &Mond);
	display_draw_show();
	warten(1200);

	beschriftung(PSTR("transparent ueber dem Muster"));
	for (uint8_t x = 60; x < 110; x++)
	{
		display_draw_byte(x, 12, 0xAA);
	}
	display_draw_bitmap_transparent(70, 12, &Mond);
	display_draw_show();
	warten(2000);

	beschriftung(PSTR("display_clear_bitmap"));
	display_clear_bitmap(70, 12, &Mond);
	display_draw_show();
	warten(1500);

	// --- Schriftgroessen ----------------------------------------------
	buehne_leeren();
	beschriftung(PSTR("FONT_SIZE_1X bis 3X"));
	display_draw_string_P(6,  4, PSTR("1x normal"), FONT_SIZE_1X);
	display_draw_string_P(6, 12, PSTR("1.5x"), FONT_SIZE_1_5X);
	display_draw_string_P(6, 22, PSTR("2x"), FONT_SIZE_2X);
	display_draw_string_P(50, 18, PSTR("3x"), FONT_SIZE_3X);
	display_draw_show();
	warten(2500);
}


// ===========================================================================
//  Bild 3: Bergbahn am Nachthimmel
//
//  Hier laeuft alles in einer einzigen Bildschleife ab, und jedes Bild hat
//  denselben Ablauf: erst wird alles Bewegte geloescht, dann wird nachgezogen,
//  was darunter lag, dann wird neu gezeichnet, und ganz zuletzt einmal
//  angezeigt.
//
//  Diese Reihenfolge ist der Grund, warum nichts flackert und nichts
//  verschwindet. Wer mitten in der Schleife wartet, waehrend etwas geloescht
//  ist, sieht genau das fehlen. Die Sternschnuppe zieht deshalb Bild fuer Bild
//  weiter, statt die Schleife anzuhalten.
// ===========================================================================

// Seil waagrecht zwischen zwei Masten, die auf dem Boden stehen.
// Die Gondel ist 12 Pixel breit und haengt mittig unter ihrem Punkt auf dem
// Seil. Das Seil beginnt deshalb erst bei x=10, sonst wuerde die Gondel am
// linken Ende ueber den Rahmen hinausragen und ihn beim Loeschen mitnehmen.
enum {
	SEIL_X1  = 10,   SEIL_Y1 = 22,
	SEIL_X2  = 101,  SEIL_Y2 = 22,
	BODEN_Y  = 34,        // Linie, auf der die Masten stehen
	SCHRIFT_Y = 39,       // Schriftzug darunter
	GONDEL_B = 12,
	GONDEL_H = 10,
};

// Bahn der Sternschnuppe: Startpunkt, Schritt je Bild und Laenge des Schweifs
enum {
	SCHNUPPE_X0       = 14,
	SCHNUPPE_Y0       = 5,
	SCHNUPPE_DX       = 6,
	SCHNUPPE_DY       = 1,
	SCHNUPPE_SCHRITTE = 11,
	SCHNUPPE_SCHWEIF  = 8,
	SCHNUPPE_ALLE     = 70,   // alle so viele Bilder faellt eine
};

/* Setzt alle Sterne und loescht genau einen. Dadurch funkelt der Himmel.
   Nur auslassen wuerde nicht genuegen: der Stern von vorhin steht ja noch
   auf dem Display, er muss aktiv weggenommen werden. Mit ausser >= der
   Anzahl leuchten alle. */
static void sterne_zeichnen(uint8_t ausser)
{
	for (uint8_t i = 0; i < STERNE_ANZAHL; i++)
	{
		const uint8_t x = pgm_read_byte(&Sterne[2 * i]);
		const uint8_t y = pgm_read_byte(&Sterne[2 * i + 1]);

		if (i == ausser) display_clear_pixel(x, y);
		else             display_draw_pixel(x, y);
	}
}

/* Zeichnet Seil und Masten.

   Wird in jedem Bild erneut gebraucht. display_clear_bild nimmt genau die
   Pixel des Bildes weg, ganz gleich, was darunter lag. Alles, was die Gondel
   ueberdeckt hat, muss das Programm also selbst nachziehen. Genau diese
   Buchfuehrung nimmt einem display_draw_sprite ab. */
static void seil_und_masten(void)
{
	display_draw_rect(SEIL_X1 - 2, SEIL_Y1, 2, (uint8_t)(BODEN_Y - SEIL_Y1), true);
	display_draw_rect(SEIL_X2, SEIL_Y2, 2, (uint8_t)(BODEN_Y - SEIL_Y2), true);
	display_draw_line(SEIL_X1, SEIL_Y1, SEIL_X2, SEIL_Y2);
}

/* Zeichnet die Teile, die sich nie bewegen. */
static void himmel_aufbauen(void)
{
	display_draw_clear();
	display_draw_rect(0, 0, FENSTER_B, FENSTER_H, false);

	sterne_zeichnen(STERNE_ANZAHL);
	display_draw_bitmap(108, 2, &Mond);

	// Boden, auf dem die Masten stehen
	display_draw_line(1, BODEN_Y, FENSTER_B - 2, BODEN_Y);

	// Schriftzug mittig darunter, 18 Zeichen zu je 4 Pixel Vorschub
	display_draw_string_P((FENSTER_B - 18 * 4) / 2, SCHRIFT_Y,
	                      PSTR("Bergbahn bei Nacht"), FONT_SIZE_1X);

	seil_und_masten();
	display_draw_show();
}

static void bergbahn(void)
{
	const uint8_t schritte = 56;

	uint8_t  schritt   = 0;      // Stand der Gondel auf dem Seil
	int8_t   richtung  = 1;
	uint8_t  gondel_x  = SEIL_X1 - GONDEL_B / 2;   // zuletzt gezeichnet
	bool     gondel_da = false;

	bool     schnuppe_laeuft = false;
	bool     schnuppe_da     = false;
	uint8_t  schnuppe_nr     = 0;
	uint8_t  schnuppe_x = 0, schnuppe_y = 0;       // zuletzt gezeichnet

	uint16_t takt = 0;

	himmel_aufbauen();

	for (;;)
	{
		// --- 1. alles Bewegte loeschen --------------------------------
		if (gondel_da)
		{
			display_clear_bitmap(gondel_x, SEIL_Y1, &Gondel);
			gondel_da = false;
		}
		if (schnuppe_da)
		{
			display_clear_line_with_step(schnuppe_x, schnuppe_y,
			                             (uint8_t)(schnuppe_x - SCHNUPPE_SCHWEIF),
			                             (uint8_t)(schnuppe_y - 3), 2);
			schnuppe_da = false;
		}

		// --- 2. nachziehen, was darunter lag --------------------------
		// Das Loeschen kennt den Untergrund nicht, es nimmt einfach die
		// Pixel des Bildes weg. Seil, Masten und Sterne kommen deshalb
		// jedes Mal neu.
		seil_und_masten();
		// Durch drei geteilt, sonst wandert die Luecke zu hastig ueber den
		// Himmel und es sieht nach Fehler statt nach Funkeln aus.
		sterne_zeichnen((uint8_t)((takt / 3) % STERNE_ANZAHL));

		// --- 3. Zustand fortschreiben ---------------------------------
		if (schritt == schritte)   richtung = -1;
		else if (schritt == 0)     richtung = 1;
		schritt = (uint8_t)(schritt + richtung);

		if (!schnuppe_laeuft && (takt % SCHNUPPE_ALLE) == 0 && takt > 0)
		{
			schnuppe_laeuft = true;
			schnuppe_nr = 0;
		}

		// --- 4. neu zeichnen ------------------------------------------
		// Position der Gondel auf dem Seil
		{
			const int16_t sx = SEIL_X1
			    + ((int16_t)(SEIL_X2 - SEIL_X1) * schritt) / schritte;
			gondel_x = (uint8_t)(sx - GONDEL_B / 2);
		}
		display_draw_bitmap_transparent(gondel_x, SEIL_Y1, &Gondel);
		gondel_da = true;

		if (schnuppe_laeuft)
		{
			schnuppe_x = (uint8_t)(SCHNUPPE_X0 + schnuppe_nr * SCHNUPPE_DX);
			schnuppe_y = (uint8_t)(SCHNUPPE_Y0 + schnuppe_nr * SCHNUPPE_DY);

			display_draw_line_with_step(schnuppe_x, schnuppe_y,
			                            (uint8_t)(schnuppe_x - SCHNUPPE_SCHWEIF),
			                            (uint8_t)(schnuppe_y - 3), 2);
			schnuppe_da = true;

			if (++schnuppe_nr > SCHNUPPE_SCHRITTE)
			{
				schnuppe_laeuft = false;
			}
		}

		// --- 5. einmal anzeigen ---------------------------------------
		display_draw_show();
		warten(70);
		takt++;
	}
}


// ===========================================================================
//  Programm
// ===========================================================================
/* Zeigt nacheinander Titel, Werkzeuge und Landschaft. Die Landschaft laeuft
   danach endlos weiter, das Programm kehrt also nicht zurueck. */
void grafik_demo_run(void)
{
	display_draw_init(FENSTER_B, FENSTER_H, FENSTER_X, FENSTER_Y);

	titel();
	werkzeuge();
	bergbahn();
}
