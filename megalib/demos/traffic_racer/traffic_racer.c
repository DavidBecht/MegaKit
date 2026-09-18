/*-------------------------------------------------------------------------*\
| Datei:        traffic_racer.c
| Version:      1.0
| Projekt:      Traffic Racer - Videospiel
| Beschreibung: Eigenes Videospiel Programmieren
| Schaltung:    MEGACARD V6.11
| Autor:        Emil Zimmermann
| Erstellung:   2026
|
| Aenderung:    David Bechtold
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <avr/eeprom.h>
#include <stdio.h>     // sprintf_P
#include <util/delay.h>
#include <stdbool.h>
#include "traffic_racer_bitmaps.h"
#include "../../megalib/display/display_draw.h"
#include "../../megalib/display/display.h"
#include "../../megalib/display/display_draw_sprite.h"
#include "../../megalib/display/random.h"
#include "../../megalib/sound/sound.h"
#include "traffic_racer_sounds.h"

// --- Einstellungen ----------------------------------------------------------
// Tastenbelegung. Die Klammern entsprechen den Pfeiltasten im Simulator.
#define BTN_HOCH        (1 << PA3)                  // S3  (Pfeil oben)
#define BTN_RUNTER      (1 << PA2)                  // S2  (Pfeil unten)
#define BTN_START       (1 << PA0)                  // S0  (Pfeil rechts)
#define BTN_TON         (1 << PA1)                  // S1  (Pfeil links), Ton ein/aus

#define HINDERNIS_BALKEN    1
#define HINDERNIS_HYDRANT   2

#define BALKEN_Y            4        // Hoehe des Baustellenbalkens
#define HYDRANT_Y          21        // Hoehe des Hydranten
#define HINDERNIS_ZIEL_X    0        // ab hier kommt das naechste Hindernis

#define SCROLL_PIXEL        3        // Pixel, die die Welt pro Bild wandert
#define AUTO_RAND           2        // Abstand des Autos zum Rahmen
#define AUTO_START_X       20
#define AUTO_START_Y        2
#define STRASSE_X           1
#define STRASSE_Y          17
#define ANZEIGE_PAGE        5        // Textzeile unterhalb der Zeichenflaeche

// --- Zeichenflaeche ---------------------------------------------------------
// Als Aufzaehlung, nicht als const-Variablen: display_draw_init() legt
// daraus den Videopuffer an, die Werte muessen also schon beim Uebersetzen
// feststehen. Eine Aufzaehlung kostet dabei kein einziges Byte.
enum {
	display_width  = 125,
	display_height = 35,
	display_x_pos  = 0,
	display_y_pos  = 0,
};

// --- Bestenliste ------------------------------------------------------------
// Drei Werte zu je zwei Byte ab Adresse 0 im EEPROM. Der ATmega16 hat davon
// 512 Byte, gebraucht werden sechs. Ein ungebrannter Baustein liefert 0xFFFF,
// das gilt hier als leerer Platz.
//
// Bewusst mit festen Adressen statt mit EEMEM-Variablen. Auf dem AVR ist ein
// EEPROM-Zeiger nichts anderes als eine Adresse von 0 bis 511, und nur so
// kann der Simulator dieselbe Zahl als Index in sein nachgebildetes EEPROM
// nehmen und den Inhalt in eine Datei sichern.
// Belegung des EEPROM:
//   0..5  drei Highscores zu je zwei Byte
//   6     Ton: 0 = aus, sonst ein (ein ungebrannter Baustein liefert 0xFF)
#define EE_HIGHSCORE    0
#define HIGHSCORES      3
#define HS_LEER         0xFFFF
#define EE_TON          6

#define HS_Y            17       // erste Zeile, unterhalb des Trennbalkens
#define HS_ZEILE_H      6        // 5 Pixel Zeichenhoehe plus ein Pixel Abstand
#define HS_ZEICHEN_B    4        // Vorschub je Zeichen beim 3x5-Satz

// --- Taster -----------------------------------------------------------------
// true, sobald mindestens einer der Taster in der Maske gedrueckt ist.
// Die Taster sind active low, ein gedrueckter Taster zieht sein Bit auf 0.
static bool taster_gedrueckt(uint8_t maske)
{
	return (PINA & maske) != maske;
}

// --- Ton ein/aus ------------------------------------------------------------
// Liest den gespeicherten Zustand und setzt die Tonausgabe entsprechend.
// Ein ungebrannter Baustein liefert 0xFF, das gilt als eingeschaltet.
static void ton_laden(void)
{
	sound_stumm(eeprom_read_byte((const uint8_t *)EE_TON) == 0);
}

// Fragt S1 ab und schaltet bei jedem neuen Druck den Ton um. Der neue Zustand
// wird sofort gespeichert; eeprom_update_byte schreibt nur bei Aenderung.
//
// Entprellt wird ueber die Zahl der Abfragen: ein neuer Druck zaehlt erst,
// nachdem der Taster zweimal hintereinander losgelassen gesehen wurde. Ein
// einzelner Prellimpuls setzt den Zaehler nicht zurueck.
static void ton_pruefen(void)
{
	static uint8_t losgelassen = 2;

	if (taster_gedrueckt(BTN_TON))
	{
		if (losgelassen >= 2)
		{
			const bool stumm = !sound_ist_stumm();
			sound_stumm(stumm);
			eeprom_update_byte((uint8_t *)EE_TON, stumm ? 0 : 1);
		}
		losgelassen = 0;
	}
	else if (losgelassen < 2)
	{
		losgelassen++;
	}
}

// Wartet auf einen sauberen Tastendruck: druecken, entprellen, loslassen.
// Der Tonschalter bleibt waehrenddessen bedienbar.
static void warte_auf_taster(uint8_t maske)
{
	while(1)
	{
		while (!taster_gedrueckt(maske)) { ton_pruefen(); _delay_ms(10); }

		_delay_ms(20);                       // entprellen
		if (taster_gedrueckt(maske)) break;  // wirklich gedrueckt
	}

	while (taster_gedrueckt(maske)) { ton_pruefen(); _delay_ms(10); }
	_delay_ms(20);                           // entprellen beim Loslassen
}

// --- Eingabe ----------------------------------------------------------------
static void auto_steuern(void)
{
	if (taster_gedrueckt(BTN_HOCH) && Racecar_Sprite.y >= AUTO_RAND)
	{
		Racecar_Sprite.y -= 1;
	}

	if (taster_gedrueckt(BTN_RUNTER) &&
	    Racecar_Sprite.y <= display_height - Racecar_Sprite.height - AUTO_RAND)
	{
		Racecar_Sprite.y += 1;
	}
}

// --- Hindernisse ------------------------------------------------------------
// Setzt genau ein Hindernis am rechten Rand neu und gibt zurueck, welches.
static uint8_t hindernis_spawnen(void)
{
	uint8_t hindernis = random_uint8_range(HINDERNIS_BALKEN, HINDERNIS_HYDRANT + 1);

	display_draw_sprite_kill(&Baustellenbalken_Sprite);
	display_draw_sprite_kill(&Hydrant_Sprite);

	if (hindernis == HINDERNIS_BALKEN)
	{
		display_draw_sprite_activate(&Baustellenbalken_Sprite, display_width, BALKEN_Y);
	}
	else
	{
		display_draw_sprite_activate(&Hydrant_Sprite, display_width, HYDRANT_Y);
	}

	return hindernis;
}

// true, sobald das genannte Hindernis links aus dem Bild gefahren ist.
// Dann ist Platz fuer das naechste.
static bool hindernis_durch(uint8_t hindernis)
{
	const SPRITE_T *s = (hindernis == HINDERNIS_BALKEN) ? &Baustellenbalken_Sprite
	                                                    : &Hydrant_Sprite;
	return s->x <= HINDERNIS_ZIEL_X;
}

// Prueft beide Hindernisse. Bei einem Treffer stehen in tx und ty die
// Koordinaten der Beruehrungsstelle, sonst bleiben sie unveraendert.
static bool auto_kollidiert(int16_t *tx, int8_t *ty)
{
	return display_draw_sprite_collision_point(&Racecar_Sprite, &Baustellenbalken_Sprite, tx, ty) ||
	       display_draw_sprite_collision_point(&Racecar_Sprite, &Hydrant_Sprite,          tx, ty);
}

// Liest einen Platz der Bestenliste. Ein noch nie beschriebenes EEPROM
// liefert 0xFFFF, das gilt hier als leerer Platz und wird zu 0.
static uint16_t highscore_lesen(uint8_t platz)
{
	uint16_t w = eeprom_read_word((uint16_t *)(EE_HIGHSCORE + 2 * platz));
	return (w == HS_LEER) ? 0 : w;
}

// Sortiert die Strecke ein, der letzte Platz fliegt hinaus. Laeuft nur einmal
// beim Game Over, denn ein EEPROM-Wort zu schreiben dauert etwa drei
// Millisekunden und blockiert dabei.
static void highscore_eintragen(uint16_t strecke)
{
	uint16_t liste[HIGHSCORES];

	for (uint8_t i = 0; i < HIGHSCORES; i++)
	{
		liste[i] = highscore_lesen(i);
	}

	for (uint8_t i = 0; i < HIGHSCORES; i++)
	{
		if (strecke > liste[i])
		{
			for (uint8_t j = HIGHSCORES - 1; j > i; j--)
			{
				liste[j] = liste[j - 1];
			}
			liste[i] = strecke;
			break;
		}
	}

	// update statt write: schreibt nur, was sich geaendert hat, und schont
	// damit die rund 100000 Schreibzyklen einer Zelle
	for (uint8_t i = 0; i < HIGHSCORES; i++)
	{
		eeprom_update_word((uint16_t *)(EE_HIGHSCORE + 2 * i), liste[i]);
	}
}

// Zeichnet die Liste rechtsbuendig in die freie Ecke unten rechts. Wird nach
// dem Bitmap aufgerufen und liegt damit darueber.
static void highscore_zeichnen(void)
{
	char text[7];

	for (uint8_t i = 0; i < HIGHSCORES; i++)
	{
		uint16_t wert = highscore_lesen(i);
		if (wert == 0) continue;                  // leerer Platz bleibt leer

		// sprintf_P gibt die Anzahl der geschriebenen Zeichen zurueck.
		// Das _P haelt den Formatstring im Flash statt im RAM.
		uint8_t stellen = (uint8_t)sprintf_P(text, PSTR("%u"), wert);

		display_draw_string((uint8_t)(display_width - 1 - stellen * HS_ZEICHEN_B),
		                    (uint8_t)(HS_Y + i * HS_ZEILE_H),
		                    text, FONT_SIZE_1X);
	}
}

// --- Bildschirme ------------------------------------------------------------
// Zeigt das Startbild mit der Bestenliste und wartet auf den Startknopf.
static void startbildschirm(void)
{
	display_draw_clear();   // leert Anzeige und Zeichenpuffer gemeinsam

	display_draw_bitmap(0, 0, &Startbild);
	highscore_zeichnen();
	display_draw_show();    // ohne das bleibt das Bild im Puffer stehen

	sound_melodie(&KnightRider, SOUND_ENDLOS);
	warte_auf_taster(BTN_START);
}

// Zeigt das Schlussbild mit der Bestenliste und wartet auf den Neustart.
// Traegt die gefahrene Strecke vorher in die Liste ein.
static void game_over_bildschirm(uint16_t strecke)
{
	display_draw_clear();

	display_draw_bitmap(0, 0, &Gameoverbild);
	highscore_eintragen(strecke);
	highscore_zeichnen();
	display_draw_show();
	display_printf_pos_P(2, ANZEIGE_PAGE, PSTR("Strecke: %u"), strecke);
	sound_melodie(&KnightRider, SOUND_ENDLOS);
	warte_auf_taster(BTN_START);
}

// --- Spielablauf ------------------------------------------------------------
// Setzt alle Sprites auf ihre Startwerte zurueck. Noetig, weil eine Runde
// Positionen und Geschwindigkeiten veraendert.
static void runde_vorbereiten(void)
{
	display_draw_sprite_kill(&Explosion_Sprite);
	display_draw_sprite_kill(&Baustellenbalken_Sprite);
	display_draw_sprite_kill(&Hydrant_Sprite);

	display_draw_clear();

	Racecar_Sprite.vx = 0;
	display_draw_sprite_activate(&Strasse_Sprite, STRASSE_X, STRASSE_Y);
	display_draw_sprite_activate(&Racecar_Sprite, AUTO_START_X, AUTO_START_Y);
	
	Baustellenbalken_Sprite.vx = -SCROLL_PIXEL;
	Hydrant_Sprite.vx          = -SCROLL_PIXEL;
}

// Haelt die Welt an und spielt die Explosion genau einmal ab.
static void crash_abspielen(int16_t tx, int8_t ty)
{
	// Alles einfrieren: keine Bewegung, keine Animation. Die Sprites
	// bleiben stehen und sichtbar. display_draw_sprite_activate() im
	// naechsten Rundenaufbau hebt das wieder auf.
	display_draw_sprite_pause(&Strasse_Sprite,          true);
	display_draw_sprite_pause(&Racecar_Sprite,          true);
	display_draw_sprite_pause(&Baustellenbalken_Sprite, true);
	display_draw_sprite_pause(&Hydrant_Sprite,          true);

	// Funkenstern mittig auf die Beruehrungsstelle setzen. Damit sitzt er
	// auch dann richtig, wenn man seitlich in ein Hindernis faehrt.
	display_draw_sprite_activate(&Explosion_Sprite,
	                             tx - Explosion_Sprite.width  / 2,
	                             ty - Explosion_Sprite.height / 2);

	sound_melodie(&Absturz, 1);

	// anim_loops = 1: der Sprite deaktiviert sich selbst, wenn er durch ist
	while (Explosion_Sprite.active)
	{
		ton_pruefen();
		display_draw_sprite_update_all(true);
	}
}

// Spielt eine Runde bis zum Zusammenstoss und gibt die gefahrene Strecke zurueck.
static uint16_t spiel_runde(void)
{
	runde_vorbereiten();

	uint16_t strecke   = 0;
	uint8_t  hindernis = hindernis_spawnen();
	int16_t  tx        = 0;    // Beruehrungsstelle beim Zusammenstoss
	int8_t   ty        = 0;

	display_printf_pos_P(0, ANZEIGE_PAGE, PSTR("Strecke: %-5u"), strecke);

	while (!auto_kollidiert(&tx, &ty))
	{
		display_draw_rect(0, 0, display_width, display_height, false);

		auto_steuern();
		ton_pruefen();

		strecke += SCROLL_PIXEL;
		display_printf_pos_P(0, ANZEIGE_PAGE, PSTR("Strecke: %-5u"), strecke);

		if (hindernis_durch(hindernis))
		{
			hindernis = hindernis_spawnen();
		}

		display_draw_sprite_update_all(true);
	}

	crash_abspielen(tx, ty);
	return strecke;
}

// --- Hauptprogramm ----------------------------------------------------------
// Richtet Taster, Anzeige, Sprites, Ton und Zufall ein und laeuft dann
// endlos zwischen Spielrunde und Schlussbild hin und her.
//
// Die Reihenfolge der Registrierung ist die Zeichenreihenfolge: die Strasse
// liegt hinten, darueber das Auto und die Hindernisse, die Explosion ganz
// vorn. Wer hier tauscht, aendert, was ueber was gezeichnet wird.
void traffic_racer_run(void)
{
	DDRA  &= ~(BTN_HOCH | BTN_RUNTER | BTN_START | BTN_TON);
	PORTA |=  (BTN_HOCH | BTN_RUNTER | BTN_START | BTN_TON);

	display_draw_init(display_width, display_height, display_x_pos, display_y_pos);
	display_draw_sprite_init(FPS_30);
	sound_init();
	ton_laden();
	random_init();

	display_draw_sprite_register(&Strasse_Sprite);
	display_draw_sprite_register(&Racecar_Sprite);
	display_draw_sprite_register(&Baustellenbalken_Sprite);
	display_draw_sprite_register(&Hydrant_Sprite);
	display_draw_sprite_register(&Explosion_Sprite);
	

	startbildschirm();

	while(1)
	{
		uint16_t strecke = spiel_runde();
		game_over_bildschirm(strecke);
	}
}
