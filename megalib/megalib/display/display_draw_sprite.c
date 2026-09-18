/*-------------------------------------------------------------------------*\
| Datei:        display_draw_sprite.c
| Version:      1.1
| Projekt:      Zeichenbibliothek fuer die MEGACARD
| Beschreibung: Bibliotheksfunktionen (Implementierung)
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   30.04.2026
|
| Aenderung:    Doku vereinheitlicht, Pruefung von MAX_SPRITES
\*-------------------------------------------------------------------------*/
#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#include "display_draw_sprite.h"
#include "display_draw.h"
#include <avr/io.h>
#include <avr/interrupt.h>

// Das Neuzeichnen merkt sich je Sprite ein Bit in einer uint8_t-Variablen.
// Mehr als acht Sprites passen dort nicht hinein, deshalb der Riegel.
#if MAX_SPRITES > 8
#error "MAX_SPRITES darf hoechstens 8 sein, sonst passen die Masken in display_draw_sprite_update_all nicht."
#endif

static SPRITE_T* sprite_list[MAX_SPRITES];
static uint8_t sprite_count = 0;
static volatile bool frame_ready = false;
static uint8_t _fps = 30;

/* Bildtakt. Setzt nur eine Marke, gezeichnet wird im Hauptprogramm.
   Ein Interrupt muss kurz sein, das Zeichnen eines ganzen Bildes dauert
   dafuer viel zu lange. */
ISR(TIMER1_COMPA_vect)
{
	frame_ready = true;
}

// ------------------------------------------------------------

/* Holt das Bild mit der angegebenen Nummer. Die Liste liegt im Flash, der
   Zeiger darin muss deshalb mit pgm_read_ptr geholt werden. */
static const BITMAP_T *_bild(const SPRITE_T *s, uint8_t nummer)
{
	return (const BITMAP_T *)pgm_read_ptr(&s->frames[nummer]);
}

void display_draw_sprite_init(SPRITE_FPS_T fps)
{
	if (fps == 0) fps = FPS_30;
	_fps = (uint8_t)fps;

	/* Timer1 im CTC-Modus. Der Vergleichswert ergibt sich aus
	       OCR1A = F_CPU / (Vorteiler * Bilder je Sekunde) - 1

	   Der Vorteiler darf dabei nicht fest sein. OCR1A ist 16 Bit breit, und
	   bei 12 MHz und 15 Bildern je Sekunde braeuchte es mit Vorteiler 8 den
	   Wert 99999. Der passt nicht hinein, die oberen Bits fielen heraus, und
	   herausgekommen sind 43,5 statt 15 Bilder je Sekunde.

	   Deshalb wird der kleinste Vorteiler gesucht, mit dem der Wert passt.
	   Klein ist gut, denn er bestimmt die Feinheit des Zeitrasters. */
	const uint32_t takte = F_CPU / (uint32_t)fps;   // Takte je Bild

	uint16_t teiler = 8;
	uint8_t  cs     = (1 << CS11);                  // Vorteiler 8

	if (takte / 8UL > 65536UL)
	{
		teiler = 64;
		cs     = (1 << CS11) | (1 << CS10);         // Vorteiler 64
	}
	if (takte / 64UL > 65536UL)
	{
		teiler = 256;
		cs     = (1 << CS12);                       // Vorteiler 256
	}

	uint32_t wert = takte / teiler;
	if (wert == 0)      wert = 1;                   // sehr hohe Bildrate
	if (wert > 65536UL) wert = 65536UL;             // sehr niedrige Bildrate

	TCCR1B = (uint8_t)((1 << WGM12) | cs);
	OCR1A  = (uint16_t)(wert - 1);
	TIMSK |= (1 << OCIE1A);
	sei();
}

// ------------------------------------------------------------

void display_draw_sprite_register(SPRITE_T *s)
{
	/* Ohne Bilder ist ein Sprite weder zeichen- noch loeschbar. Ein solcher
	   Sprite fuehrte in display_draw_sprite_update_all zu einer
	   Nullzeiger-Dereferenzierung, und zwar erst beim ersten Bildaufbau und
	   damit fern der Ursache. Die Pruefung erfolgt deshalb hier. */
	if (s == 0 || s->frames == 0 || s->frame_count == 0)
	{
		return;
	}

	// Groesse aus dem ersten Bild uebernehmen, dort steht sie ohnehin.
	{
		const BITMAP_T *erstes = _bild(s, 0);
		if (erstes == 0) return;
		s->width  = pgm_read_byte(&erstes->breite);
		s->height = pgm_read_byte(&erstes->hoehe);
		if (s->width == 0 || s->height == 0) return;
	}

	if (sprite_count < MAX_SPRITES)
	{
		s->intern.prev_x = s->x;
		s->intern.prev_y = s->y;
		s->intern.current_frame = 0;
		s->intern.prev_frame = 0;
		s->intern.counter = 0;
		s->intern.anim_loop_count = 0;
		/* animation_ms bezeichnet die Dauer des vollstaendigen Durchlaufs,
		   nicht die eines Einzelbildes. Die Zeit wird daher durch die
		   Bildanzahl geteilt. Die Angabe bleibt damit bei geaenderter
		   Bildanzahl gueltig.

		   Gerundet wird zur naechstliegenden ganzen Bildzahl. Ein Abschneiden
		   faellt bei kurzen Animationen merklich ins Gewicht. */
		const uint32_t nenner = 1000UL * s->frame_count;
		uint16_t takte = (uint16_t)(((uint32_t)s->animation_ms * _fps + nenner / 2) / nenner);

		// Die Zaehler sind 8 Bit breit, mehr als 255 Bilder je Schritt gibt
		// es also nicht. Bei 15 Bildern je Sekunde sind das 17 Sekunden.
		if (takte == 0)   takte = 1;
		if (takte > 255)  takte = 255;
		s->intern.frame_interval = (uint8_t)takte;

		// Die Wartezeit bis zum erneuten Erscheinen ist eine einzelne Dauer
		// und wird nicht durch die Bildanzahl geteilt.
		takte = (uint16_t)((uint32_t)s->spawn_delay_ms * _fps / 1000);
		if (takte == 0 && s->spawn_delay_ms > 0) takte = 1;
		if (takte > 255) takte = 255;
		s->intern.spawn_interval = (uint8_t)takte;
		s->intern.spawn_timer = 0;
		s->active = true;
		s->needs_draw = true;
		sprite_list[sprite_count++] = s;
	}
}

// ------------------------------------------------------------

// Leert ausschliesslich die Liste. Displayinhalt und Zustand der Sprites
// bleiben unveraendert, siehe die Hinweise in display_draw_sprite.h.
void display_draw_sprite_reset(void)
{
	sprite_count = 0;
}

// ------------------------------------------------------------

bool display_draw_sprite_collides(const SPRITE_T *a, const SPRITE_T *b)
{
	if (!a->active || !b->active) return false;
	return a->x              < b->x + b->width  &&
	       a->x + a->width  > b->x              &&
	       a->y              < b->y + b->height  &&
	       a->y + a->height > b->y;
}

// ------------------------------------------------------------

bool display_draw_sprite_collision_point(const SPRITE_T *a, const SPRITE_T *b,
                                         int16_t *x, int8_t *y)
{
	if (!display_draw_sprite_collides(a, b)) return false;

	// Die vier Kanten der Ueberschneidung. Genau diese Vergleiche macht
	// display_draw_sprite_collides auch, es wirft das Ergebnis nur weg.
	const int16_t links  = (a->x > b->x) ? a->x : b->x;
	const int16_t oben   = (a->y > b->y) ? a->y : b->y;
	const int16_t rechts = (a->x + (int16_t)a->width  < b->x + (int16_t)b->width)
	                     ? (int16_t)(a->x + a->width)  : (int16_t)(b->x + b->width);
	const int16_t unten  = (a->y + (int16_t)a->height < b->y + (int16_t)b->height)
	                     ? (int16_t)(a->y + a->height) : (int16_t)(b->y + b->height);

	if (x) *x = (int16_t)((links + rechts) / 2);
	if (y) *y = (int8_t) ((oben  + unten ) / 2);
	return true;
}

// ------------------------------------------------------------

void display_draw_sprite_kill(SPRITE_T *s)
{
	if (!s->active) return;
	display_clear_bitmap(s->intern.prev_x, s->intern.prev_y,
	                     _bild(s, s->intern.prev_frame));
	s->intern.prev_x = s->x;
	s->intern.prev_y = s->y;
	s->active = false;
	s->intern.spawn_timer = (s->spawn_delay_ms > 0) ? s->intern.spawn_interval : 0;
}

// ------------------------------------------------------------

void display_draw_sprite_activate(SPRITE_T *s, int16_t x, int16_t y)
{
	s->x = x;
	s->y = (int8_t)y;
	s->intern.prev_x = x;
	s->intern.prev_y = (int8_t)y;
	s->intern.current_frame = 0;
	s->intern.prev_frame = 0;
	s->intern.counter = 0;
	s->intern.anim_loop_count = 0;
	s->active = true;
	s->paused = false;
	s->needs_draw = true;
}

// ------------------------------------------------------------

void display_draw_sprite_pause(SPRITE_T *s, bool pause)
{
	s->paused = pause;
}

// ------------------------------------------------------------

// Ueberschneiden sich zwei Rechtecke? Gleiche Rechnung wie bei den Kollisionen.
static bool _rects_overlap(int16_t ax, int16_t ay, uint8_t aw, uint8_t ah,
                           int16_t bx, int16_t by, uint8_t bw, uint8_t bh)
{
	return ax < bx + (int16_t)bw && ax + (int16_t)aw > bx &&
	       ay < by + (int16_t)bh && ay + (int16_t)ah > by;
}

// ------------------------------------------------------------

/* Ablauf in vier Phasen. Massgeblich ist, dass saemtliche Loeschvorgaenge vor
   allen Zeichenvorgaengen ausgefuehrt werden. Andernfalls entfernt das Loeschen
   eines Sprites Pixel, die ein zuvor gezeichneter Sprite im selben Bild bereits
   gesetzt hat, und die Ebenenlage haengt von der Bearbeitungsreihenfolge statt
   von der Registrierungsreihenfolge ab. */
void display_draw_sprite_update_all(bool blocking)
{
	if (blocking)
		while (!frame_ready);
	else if (!frame_ready)
		return;
	frame_ready = false;

	const int16_t dw = (int16_t)display_draw_get_width();
	const int16_t dh = (int16_t)display_draw_get_height();

	// Je ein Bit pro Sprite, MAX_SPRITES ist 8.
	uint8_t clear_mask = 0;   // alte Position wird freigeraeumt
	uint8_t draw_mask  = 0;   // wird danach neu gezeichnet
	uint8_t off_mask   = 0;   // wird nach dem Freiraeumen deaktiviert
	uint8_t cb_mask    = 0;   // on_anim_done steht noch aus

	// ----------------------------------------------------------
	// Phase A: Zustand fortschreiben. Hier wird noch nichts gezeichnet,
	// nur vorgemerkt. prev_x/prev_y/prev_frame bleiben unangetastet und
	// zeigen weiter auf das, was gerade auf dem Display steht.
	// ----------------------------------------------------------
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		SPRITE_T *s = sprite_list[i];
		const uint8_t bit = (uint8_t)(1u << i);

		// --- Inaktiv: Spawn-Countdown, ruht ebenfalls wenn angehalten ---
		if (!s->active)
		{
			if (!s->paused && s->intern.spawn_timer > 0)
			{
				s->intern.spawn_timer--;
				if (s->intern.spawn_timer == 0)
				{
					s->x = s->respawn_x;
					s->y = s->respawn_y;
					s->intern.prev_x = s->respawn_x;
					s->intern.prev_y = s->respawn_y;
					s->intern.current_frame = 0;
					s->intern.prev_frame = 0;
					s->intern.counter = 0;
					s->intern.anim_loop_count = 0;
					s->active = true;
					s->needs_draw = true;
					clear_mask |= bit;
					draw_mask  |= bit;
				}
			}
			continue;
		}

		const int16_t old_x     = s->intern.prev_x;
		const int16_t old_y     = s->intern.prev_y;
		const uint8_t old_frame = s->intern.prev_frame;

		// Angehalten: weder bewegen noch animieren. Der Sprite bleibt aber
		// aktiv, wird also weiter gezeichnet und in Phase B beruecksichtigt,
		// wenn ein anderer Sprite ueber ihn hinwegloescht.
		if (!s->paused)
		{
			// ------------------------
			// Bewegung
			// ------------------------
			s->x += s->vx;
			s->y += s->vy;

			// ------------------------
			// Animation
			// ------------------------
			s->intern.counter++;
			if (s->intern.counter >= s->intern.frame_interval)
			{
				s->intern.counter = 0;
				s->intern.current_frame++;

				if (s->intern.current_frame >= s->frame_count)
				{
					s->intern.current_frame = 0;

					// Animationszyklus abgeschlossen
					if (s->anim_loops > 0)
					{
						s->intern.anim_loop_count++;
						if (s->intern.anim_loop_count >= s->anim_loops)
						{
							s->intern.anim_loop_count = 0;
							// Auto-Stop. Freigeraeumt und deaktiviert wird in
							// Phase C, der Callback laeuft erst danach.
							clear_mask |= bit;
							off_mask   |= bit;
							cb_mask    |= bit;
							continue;
						}
					}
				}
			}

			// ------------------------
			// Ausserhalb des Bildbereichs -> deaktivieren
			// ------------------------
			if (s->x + (int16_t)s->width <= 0 || s->x >= dw ||
			    s->y + (int16_t)s->height <= 0 || s->y >= dh)
			{
				clear_mask |= bit;
				off_mask   |= bit;
				continue;
			}
		}

		// ------------------------
		// Neuzeichnen erforderlich
		// ------------------------
		if (s->needs_draw || old_x != s->x || old_y != s->y || old_frame != s->intern.current_frame)
		{
			clear_mask |= bit;
			draw_mask  |= bit;
		}
	}

	// ----------------------------------------------------------
	// Phase B: vom Freiraeumen mitbetroffene Sprites ermitteln
	// Ein geloeschtes Rechteck nimmt auch die Pixel der Sprites weg, die
	// dort mitliegen. Die muessen deshalb ebenfalls neu gezeichnet werden,
	// selbst wenn sich an ihnen nichts geaendert hat. Die Markierung ist
	// ansteckend, darum wird wiederholt bis nichts Neues mehr dazukommt.
	// ----------------------------------------------------------
	bool changed = true;
	while (changed)
	{
		changed = false;

		for (uint8_t i = 0; i < sprite_count; i++)
		{
			if (!(clear_mask & (uint8_t)(1u << i))) continue;
			const SPRITE_T *s = sprite_list[i];

			for (uint8_t j = 0; j < sprite_count; j++)
			{
				const uint8_t bit = (uint8_t)(1u << j);
				if (j == i || (clear_mask & bit)) continue;

				const SPRITE_T *t = sprite_list[j];
				if (!t->active) continue;

				if (_rects_overlap(s->intern.prev_x, s->intern.prev_y, s->width, s->height,
				                   t->intern.prev_x, t->intern.prev_y, t->width, t->height))
				{
					clear_mask |= bit;
					draw_mask  |= bit;
					changed = true;
				}
			}
		}
	}

	// ----------------------------------------------------------
	// Phase C: erst loeschen, danach ist die Flaeche frei.
	// ----------------------------------------------------------
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		const uint8_t bit = (uint8_t)(1u << i);
		if (!(clear_mask & bit)) continue;

		SPRITE_T *s = sprite_list[i];
		display_clear_bitmap(s->intern.prev_x, s->intern.prev_y,
		                     _bild(s, s->intern.prev_frame));

		if (off_mask & bit)
		{
			s->intern.prev_x = s->x;
			s->intern.prev_y = s->y;
			s->intern.prev_frame = s->intern.current_frame;
			s->active = false;
			s->intern.spawn_timer = (s->spawn_delay_ms > 0) ? s->intern.spawn_interval : 0;
		}
	}

	// Callbacks erst an dieser Stelle. Startet ein Callback den Sprite neu, zeigt
	// prev_x/prev_y danach auf die neue Position. Die alte ist zu diesem
	// Zeitpunkt schon freigeraeumt.
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		if (!(cb_mask & (uint8_t)(1u << i))) continue;
		SPRITE_T *s = sprite_list[i];
		if (s->on_anim_done) s->on_anim_done(s);
	}

	// Nachzuegler: Sprites, die ein Callback gerade aktiviert hat. Ihr
	// Platz wird noch vor dem Zeichnen freigeraeumt, damit die Reihenfolge
	// loeschen-vor-zeichnen auch fuer sie gilt.
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		const uint8_t bit = (uint8_t)(1u << i);
		SPRITE_T *s = sprite_list[i];

		if (!s->active || !s->needs_draw || (draw_mask & bit)) continue;

		display_clear_bitmap(s->intern.prev_x, s->intern.prev_y,
		                     _bild(s, s->intern.prev_frame));
		draw_mask |= bit;
	}

	// ----------------------------------------------------------
	// Phase D: zeichnen. Die Listenreihenfolge ist die Ebene,
	// zuletzt registriert heisst ganz vorn.
	// ----------------------------------------------------------
	for (uint8_t i = 0; i < sprite_count; i++)
	{
		const uint8_t bit = (uint8_t)(1u << i);
		SPRITE_T *s = sprite_list[i];

		if (!s->active || !(draw_mask & bit)) continue;

		if (s->transparent)
		{
			// Nur die gesetzten Pixel verodern, der Untergrund bleibt stehen.
			display_draw_bitmap_transparent(s->x, s->y,
			                                _bild(s, s->intern.current_frame));
		}
		else
		{
			display_draw_bitmap(s->x, s->y,
			                    _bild(s, s->intern.current_frame));
		}

		s->intern.prev_x = s->x;
		s->intern.prev_y = s->y;
		s->intern.prev_frame = s->intern.current_frame;
		s->needs_draw = false;
	}

	display_draw_show();
}
