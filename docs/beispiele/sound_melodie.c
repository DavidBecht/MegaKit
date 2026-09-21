// beispiel: sound_melodie
// titel: Eine kurze Melodie im Hintergrund
#include <avr/io.h>
#include <avr/pgmspace.h>
#include <util/delay.h>

#include "megalib/display/display_draw.h"
#include "megalib/sound/sound.h"

enum { BREITE = 96, HOEHE = 24, X_POS = 16, Y_POS = 20 };

// Ein Eintrag: Tonhoehe (OCR), Vorteiler und Dauer in Ticks zu je 10 ms.
// ocr = 0 ist eine Pause. Solche Tabellen erzeugt auch megasound aus
// einer MIDI-Datei.
static const TON_T Fanfare_toene[] PROGMEM = {
	{ 142, 3,  20 },      // C
	{ 119, 3,  20 },      // D
	{ 106, 3,  20 },      // E
	{   0, 0,  10 },      // Pause
	{  95, 3,  40 },      // G, laenger
};
static const MELODIE_T Fanfare = MELODIE(Fanfare_toene);

int main(void)
{
	display_draw_init(BREITE, HOEHE, X_POS, Y_POS);
	sound_init();

	display_draw_string_P(4, 8, PSTR("FANFARE"), FONT_SIZE_1X);
	display_draw_show();

	sound_melodie(&Fanfare, 1);        // kehrt sofort zurueck

	while (1)
	{
		// Das Programm laeuft weiter, waehrend die Melodie spielt.
		if (!sound_laeuft())
		{
			display_draw_string_P(4, 16, PSTR("FERTIG"), FONT_SIZE_1X);
			display_draw_show();
		}
		_delay_ms(20);
	}
}
