/*-------------------------------------------------------------------------*\
| Datei:        tontest.c
| Version:      1.1
| Projekt:      Einfuehrung in das Programmieren des Atmega16
| Beschreibung: Spielt das Ende der Knight-Rider-Melodie samt dem Uebergang
|               zum Anfang in einer Schleife ab. Die aktuelle Tonhoehe steht
|               auf dem Display. Dient dazu, eine Stoerung am Wiederholpunkt
|               der Melodie auf der Hardware gezielt nachzustellen.
| Schaltung:    MEGACARD V6.11, Piezo an PB3
| Autor:        David Bechtold
| Erstellung:   18.09.2026
|
| Aenderung:    Tonfolge aus TON_T samt Frequenz, Ende von Knight Rider
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "../megalib/display/display_draw.h"
#include "../megalib/sound/sound.h"

enum {
	FENSTER_B = 128,
	FENSTER_H = 32,
};

/* Ein Ton wie in einer Melodietabelle, ergaenzt um die Frequenz fuer die
   Anzeige. Die Frequenz ergibt sich zwar aus ocr und clock, steht hier aber
   ausdruecklich, damit die Tabelle ohne Nachrechnen lesbar ist. */
typedef struct {
	TON_T    ton;   // ocr, clock und ticks wie in der Melodie
	uint16_t hz;    // 0 = Pause
} TESTTON_T;

/* Das Ende der Knight-Rider-Melodie. Die Frequenzen in der zweiten Spalte
   sind aus ocr und clock nachgerechnet, f = F_CPU / (2 * Vorteiler * (1 + ocr)),
   bei clock = 3 also mit Vorteiler 64. */
static const TESTTON_T Folge[] PROGMEM = {
	{ { 119, 3,  12 }, 781 },   // 116  G5
	{ { 126, 3,  12 }, 738 },   // 117  Fis5
	{ { 168, 3, 112 }, 555 },   // 118  Cis5
	{ {   0, 0,  38 },   0 },   // 119  Pause
	{ { 252, 3,  25 }, 371 },   // 120  Fis4
	{ { 238, 3,  12 }, 392 },   // 121  G4
	{ { 252, 3,  12 }, 371 },   // 122  Fis4
	{ { 168, 3,  25 }, 555 },   // 123  Cis5
	{ { 126, 3,  25 }, 738 },   // 124  Fis5
	{ { 141, 3, 150 }, 660 },   // 125  E5, dieser Ton pfeift
};

#define FOLGE_LAENGE (sizeof(Folge) / sizeof(Folge[0]))

/* Wandelt eine Zahl in Text, ohne sprintf. Spart rund 1,5 kB Flash. */
static void zahl_als_text(uint16_t wert, char *ziel)
{
	char ziffern[5];
	uint8_t z = 0;
	do { ziffern[z++] = (char)('0' + wert % 10); wert /= 10; } while (wert);
	while (z) *ziel++ = ziffern[--z];
	*ziel = '\0';
}

/* Zeigt Position und Tonhoehe an. Nur der Bereich der Anzeige wird geloescht,
   damit der Aufruf kurz bleibt und den Takt der Tonfolge kaum verschiebt. */
static void anzeigen(uint8_t nr, uint16_t hz)
{
	char text[8];

	display_clear_rect(0, 12, FENSTER_B, 20, true);
	zahl_als_text(nr, text);
	display_draw_string_P(4, 12, PSTR("Nr"), FONT_SIZE_1X);
	display_draw_string(20, 12, text, FONT_SIZE_1X);

	if (hz == 0)
	{
		display_draw_string_P(4, 20, PSTR("Pause"), FONT_SIZE_2X);
	}
	else
	{
		zahl_als_text(hz, text);
		display_draw_string(4, 20, text, FONT_SIZE_2X);
		display_draw_string_P(40, 20, PSTR("Hz"), FONT_SIZE_2X);
	}
	display_draw_show();
}

/* Spielt die Folge in einer Schleife, so wie das Spiel die Melodie spielt.

   Jeder Ton wird mit sound_ton() gestartet, bevor der vorige von selbst
   endet. Dazu erhaelt jeder Ton einen Tick mehr als seine eigentliche Dauer,
   und nach genau seiner Dauer folgt schon der naechste. Der Timer laeuft so
   zwischen zwei Toenen durch, und der Wechsel geht ueber denselben Weg wie in
   der Melodie. Nur die Pausen halten den Timer an, genau wie dort. */
void tontest_run(void)
{
	display_draw_init(FENSTER_B, FENSTER_H, 0, 0);
	sound_init();

	display_draw_clear();
	display_draw_string_P(4, 2, PSTR("Knight Rider Ende"), FONT_SIZE_1X);
	display_draw_show();

	for (;;)
	{
		for (uint8_t i = 0; i < FOLGE_LAENGE; i++)
		{
			const uint8_t  ocr   = pgm_read_byte(&Folge[i].ton.ocr);
			const uint8_t  clock = pgm_read_byte(&Folge[i].ton.clock);
			const uint8_t  ticks = pgm_read_byte(&Folge[i].ton.ticks);
			const uint16_t hz    = pgm_read_word(&Folge[i].hz);

			sound_ton(ocr, clock, (uint8_t)(ticks + 1));
			anzeigen(i, hz);

			// Dauer des Tons in Ticks zu 10 ms abwarten
			for (uint8_t t = 0; t < ticks; t++)
			{
				_delay_ms(10);
			}
		}
	}
}
