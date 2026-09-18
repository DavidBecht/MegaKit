/*-------------------------------------------------------------------------*\
| Datei:        lama_demo.c
| Version:      1.0
| Projekt:      Einfuehrung in das Programmieren des Atmega16
| Beschreibung: Beispiel: Sprite-Animation mit Zufalls-Respawn (Lama)
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   11.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/

#include <avr/io.h>
#include <stdbool.h>

#include "../megalib/display/display_draw.h"
#include "../megalib/display/display_draw_sprite.h"
#include "../megalib/display/animations/lama.h"
#include "../megalib/display/random.h"

// S1 = PA1, active-low
#define BTN_S1  (1 << PA1)

// --- Knight-Rider -----------------------------------------------------------
static uint8_t led_pos   = 0;
static int8_t  led_dir   = 1;
static uint8_t led_frame = 0;   // Zaehler: nur jeden 2. Frame weiterschalten

// true  = nach jeder Animation an zufaelliger Position neu spawnen (Standard)
// false = immer an derselben Position neu spawnen
static bool random_spawn = true;

// --- Zeichenflaeche ---------------------------------------------------------
// 64x54px, ab x=32 auf dem physischen 128x64 Display zentriert
// Als Aufzaehlung, nicht als const-Variablen: display_draw_init() legt
// daraus den Videopuffer an, die Werte muessen also schon beim Uebersetzen
// feststehen. Eine Aufzaehlung kostet dabei kein einziges Byte.
enum {
	display_width  = 64,
	display_height = 54,
	display_x_pos  = 32,
	display_y_pos  = 0,
};

// --- Callback ---------------------------------------------------------------
// Wird automatisch aufgerufen nachdem die Animation einmal komplett abgespielt
// wurde (ausgeloest durch anim_loops = 1 im SPRITE_T).
// Hier: Lama an einer zufaelligen Position neu spawnen.
static void lama_animation_done(SPRITE_T *s);

// --- Sprite -----------------------------------------------------------------
// lama_bilder: sieben Bilder zu 48x48 Pixel, vertikal abgelegt.
// Breite, Hoehe und Ablageart stehen in den Bildern selbst.
static SPRITE_T lama_sprite = {
	.x            = 8,
	.y            = 4,
	.frames       = lama_bilder,
	.frame_count  = LAMA_BILDER,
	.animation_ms = 1050,  // alle sieben Bilder zusammen, also 150 ms je Bild
	.anim_loops   = 1,     // einmal abspielen, dann on_anim_done aufrufen
	.on_anim_done = lama_animation_done,
};

// Umsetzung des oben angekuendigten Callbacks.
static void lama_animation_done(SPRITE_T *s)
{
	uint8_t new_x, new_y;

	if (random_spawn)
	{
		// Zufaellige neue Position innerhalb des Fensters
		new_x = random_uint8_range(1, display_width  - lama_sprite.width  - 2);
		new_y = random_uint8_range(1, display_height - lama_sprite.height - 2);
	}
	else
	{
		// An derselben Position bleiben
		new_x = (uint8_t)s->x;
		new_y = (uint8_t)s->y;
	}

	display_draw_sprite_activate(s, new_x, new_y);
}

// --- Programm ---------------------------------------------------------------
// Richtet Anzeige, Sprites und Zufall ein, zeichnet einen Rahmen mit etwas
// Beispielgrafik und laesst dann das Lama laufen. Laeuft endlos und kehrt
// nicht zurueck. Mit S1 wird zwischen zufaelligem und festem Spawnpunkt
// umgeschaltet.
void lama_demo_run(void)
{
	// Seed fuer Zufallsgenerator aus ADC-Rauschen initialisieren
	random_init();

	// Buttons S0-S3 (PA0-PA3) als Eingaenge mit Pull-up konfigurieren
	DDRA  &= ~0x0F;
	PORTA |=  0x0F;

	// LEDs (PORTC) als Ausgaenge
	DDRC  = 0x0F;
	PORTC = 0x01;

	display_draw_init(display_width, display_height, display_x_pos, display_y_pos);

	// --- Beispiel: display_draw Zeichenfunktionen ---------------------------

	// Rand um den gesamten Zeichenbereich
	display_draw_rect(0, 0, display_width, display_height, false);

	// Zwei diagonale Linien (Bresenham-Algorithmus)
	display_draw_line(0, 0, display_width - 1, display_height - 1);
	display_draw_line(0, display_height - 1, display_width - 1, 0);

	// Gestrichelte horizontale Linie durch die Mitte (step=3: 3px an, 3px aus)
	display_draw_line_with_step(0, display_height / 2, display_width - 1, display_height / 2, 3);

	// Gefuelltes Rechteck in der Mitte
	display_draw_rect(display_width / 2 - 4, display_height / 2 - 4, 8, 8, true);

	// Text oben links
	display_draw_string_P(2, 2, PSTR("HALLO"), FONT_SIZE_0_5X);

	// --- Beispiel: Sprite-System --------------------------------------------

	// Timer1 fuer Sprite-System konfigurieren (15 Frames pro Sekunde)
	display_draw_sprite_init(FPS_15);

	// Sprite registrieren - ab jetzt verwaltet display_draw_sprite_update_all()
	// Bewegung, Animation und Zeichnen automatisch
	display_draw_sprite_register(&lama_sprite);

	uint8_t pina_alt = PINA;

	while (1)
	{
		// Wartet auf den naechsten Frame-Tick (blocking=true),
		// aktualisiert alle Sprites und sendet geaenderte Regionen per I2C
		display_draw_sprite_update_all(true);

		// S1 (PA1): positive Flanke (1->0, da active-low) -> random_spawn umschalten
		uint8_t pina_neu = PINA;
		if ((pina_alt & BTN_S1) && !(pina_neu & BTN_S1))
			random_spawn = !random_spawn;
		pina_alt = pina_neu;

		// Knight-Rider: alle 2 Frames eine LED weiterschalten
		if (++led_frame >= 8)
		{
			led_frame = 0;
			PORTC = (uint8_t)(1 << led_pos);
			if (led_pos == 7) led_dir = -1;
			if (led_pos == 0) led_dir =  1;
			led_pos = (uint8_t)((int8_t)led_pos + led_dir);
		}
	}
}
