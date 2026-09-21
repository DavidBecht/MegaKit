/*-------------------------------------------------------------------------*\
| Datei:        display_draw.c
| Version:      1.1
| Projekt:      Zeichenbibliothek fuer die MEGACARD
| Beschreibung: Bibliotheksfunktionen (Implementierung)
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   19.12.2025
|
| Aenderung:    Interne Helfer dokumentiert, Kommentare vereinheitlicht
\*-------------------------------------------------------------------------*/
#include <avr/io.h>
#include <stdarg.h>   // va_list fuer display_draw_printf()
#include <stdio.h>    // vsnprintf()
#include <stdlib.h>   // abs() fuer den Bresenham-Algorithmus
#include <stdbool.h>
#include <string.h>
#include <util/delay.h>
#include <avr/pgmspace.h>
#include "font/FontData.h"
#include "display_draw.h"
#include "display.h"

// Der Puffer liegt flach im Speicher, eine Page nach der anderen. Die Breite
// steht erst zur Laufzeit fest, ein zweidimensionales Array geht deshalb
// nicht. VBUF rechnet Page und Spalte in den laufenden Index um.
static uint8_t *_video_buffer = 0;
#define VBUF(page, x) _video_buffer[(uint16_t)(page) * _display_width + (uint8_t)(x)]
static uint8_t _display_width = 0;
static uint8_t _display_height = 0;
static uint8_t _display_x_pos = 0;
static uint8_t _display_y_pos = 0;
static uint8_t _display_pages = 0;
static uint8_t _display_base_page = 0;
static uint8_t _display_shift = 0;
static bool _display_initialized = false;

// Gesammelte, noch nicht uebertragene Aenderungen. Je Page ein Bit in
// _dirty_pages und dazu die erste und letzte geaenderte Spalte. Daraus baut
// display_draw_show() eine einzige Burst-Transaktion je Page.
static uint8_t _dirty_pages = 0;
static uint8_t _dirty_x_min[OLED_PAGES];
static uint8_t _dirty_x_max[OLED_PAGES];

// Abfragefunktionen, damit anderer Code die Fenstermasse nicht doppelt
// festlegen muss. Beschreibung siehe display_draw.h.
uint8_t display_draw_get_width(void)  { return _display_width; }
uint8_t display_draw_get_height(void) { return _display_height; }
uint8_t display_draw_get_x_pos(void)  { return _display_x_pos; }
uint8_t display_draw_get_y_pos(void)  { return _display_y_pos; }

void _display_draw_init(uint8_t *puffer, uint8_t display_width, uint8_t display_height,
                        uint8_t display_x_pos, uint8_t display_y_pos)
{
	display_init();
	_display_initialized = false;

	// Genau ein Aufruf je Programm. Ein zweiter Aufruf mit abweichendem Puffer
	// laesst den ersten ungenutzt im Speicher zurueck. Zur Uebersetzungszeit
	// ist dieser Fall nicht erkennbar, daher die Pruefung zur Laufzeit.
	if (_video_buffer != 0 && _video_buffer != puffer)
	{
		display_string_pos_P(0, 0, PSTR("draw_init 2x!"));
		display_string_pos_P(0, 1, PSTR("Nur einmal rufen."));
		for (;;) { }
	}

	/* Das Fenster muss vollstaendig auf dem Display liegen, nicht nur seine
	   Abmessungen muessen passen. Andernfalls ueberschreiten x + _display_x_pos
	   und y + _display_y_pos den Wertebereich von uint8_t, laufen auf kleine
	   Werte zurueck und adressieren eine falsche Stelle des Displays. Auch der
	   Page-Index wuerde den Bereich von acht Pages verlassen. */
	if ((uint16_t)display_x_pos + display_width  > OLED_PIXEL_X ||
	    (uint16_t)display_y_pos + display_height > OLED_PIXEL_Y ||
	    display_width == 0 || display_height == 0)
	{
		display_string_pos_P(0, 0, PSTR("draw_init failed!"));
		display_string_pos_P(0, 1, PSTR("Window off screen!"));
		for (;;) { }
	}

	_video_buffer      = puffer;
	_display_width     = display_width;
	_display_height    = display_height;
	_display_pages     = (display_height + 7) / 8;
	_display_base_page = display_y_pos >> 3;
	_display_x_pos     = display_x_pos;
	_display_y_pos     = display_y_pos;
	_display_shift     = display_y_pos % 8;

	memset(_video_buffer, 0, (uint16_t)_display_pages * _display_width);
	_dirty_pages = 0;
	_display_initialized = true;
}

void display_draw_clear(void)
{
	if (_display_initialized)
	{
		display_clear();
		memset(_video_buffer, 0, (uint16_t)_display_pages * _display_width);
		_dirty_pages = 0;
	}
}

/* Meldet, dass sich eine Spalte des Videopuffers geaendert hat.
   Das ist die einzige Stelle, an der die beiden Betriebsarten auseinandergehen:

   Fenster auf Page-Grenze (_display_shift == 0): die Spalte wird nur
   vorgemerkt. Uebertragen wird spaeter gesammelt in display_draw_show().

   Verschobenes Fenster (_display_shift != 0): eine Pufferzeile liegt ueber
   zwei Display-Pages. Die beiden Haelften muessen aus je zwei Pufferzeilen
   zusammengesetzt werden, dafuer gibt es keinen Burst. Hier wird deshalb
   sofort geschrieben, und display_draw_show() hat spaeter nichts mehr zu tun.

   x_video_buffer/video_buffer_page  Ort im Puffer
   x_display/display_page            derselbe Ort auf dem Display */
static void _display_draw_column(uint8_t x_video_buffer, uint8_t x_display,
                                uint8_t video_buffer_page, uint8_t display_page)
{
	if (_display_shift == 0)
	{
		// Nur vormerken, uebertragen wird am Bildende in display_draw_show()
		uint8_t mask = (uint8_t)(1u << video_buffer_page);
		if (_dirty_pages & mask)
		{
			if (x_video_buffer < _dirty_x_min[video_buffer_page]) _dirty_x_min[video_buffer_page] = x_video_buffer;
			if (x_video_buffer > _dirty_x_max[video_buffer_page]) _dirty_x_max[video_buffer_page] = x_video_buffer;
		}
		else
		{
			_dirty_pages |= mask;
			_dirty_x_min[video_buffer_page] = x_video_buffer;
			_dirty_x_max[video_buffer_page] = x_video_buffer;
		}
	}
	else
	{
		// Verschobenes Fenster: sofort schreiben, Burst ist hier nicht moeglich
		uint8_t byte_to_draw = VBUF(video_buffer_page, x_video_buffer);
		uint8_t upper_from_video = (video_buffer_page > 0)
			? (uint8_t)(VBUF(video_buffer_page - 1, x_video_buffer) >> (8 - _display_shift))
			: (uint8_t)0;
		uint8_t lower_from_video = (video_buffer_page + 1 < _display_pages)
			? (uint8_t)(VBUF(video_buffer_page + 1, x_video_buffer) << _display_shift)
			: (uint8_t)0;
		uint8_t upper = (uint8_t)(byte_to_draw << _display_shift) | upper_from_video;
		uint8_t lower = (uint8_t)(byte_to_draw >> (8 - _display_shift)) | lower_from_video;
		display_pixel_byte(x_display, display_page << 3, upper);
		if (display_page + 1 < OLED_PAGES)
			display_pixel_byte(x_display, (display_page + 1) << 3, lower);
	}
}

void display_draw_show(void)
{
	for (uint8_t vp = 0; vp < _display_pages; vp++)
	{
		if (!(_dirty_pages & (uint8_t)(1u << vp))) continue;

		uint8_t x_min = _dirty_x_min[vp];
		uint8_t x_max = _dirty_x_max[vp];
		uint8_t pcol  = (uint8_t)(x_min + _display_x_pos);
		uint8_t prow  = (uint8_t)((_display_base_page + vp) << 3);

		display_burst_start(pcol, prow);
		for (uint8_t x = x_min; x <= x_max; x++)
			display_burst_write(VBUF(vp, x));
		display_burst_end();
	}
	_dirty_pages = 0;
}

// Zweitname, leitet an display_draw_show() weiter.
void display_draw_flush(void)
{
	display_draw_show();
}

/* Gemeinsamer Kern von display_draw_pixel() und display_clear_pixel().
   clear = false setzt das Pixel, clear = true loescht es. Aendert sich am
   Puffer nichts, wird auch nichts gemeldet. */
static void _display_draw_clear_pixel(uint8_t x, uint8_t y, bool clear)
{
	if (x >= _display_width || y >= _display_height) return;
	uint8_t x_abs = x + _display_x_pos;
	uint8_t y_abs = y + _display_y_pos;
	if (x_abs >= OLED_PIXEL_X || y_abs >= OLED_PIXEL_Y) return;
	uint8_t video_buffer_page = y >> 3;
	uint8_t display_page = _display_base_page + video_buffer_page;

	uint8_t bit = (1 << (uint8_t)(y % 8));
	
	
	if (clear == false)
	{
		// bereits gesetzt: keine Aenderung, keine Meldung noetig
		if ((VBUF(video_buffer_page, x) & bit) == bit) return;
		VBUF(video_buffer_page, x) |= bit;
	}
	else
	{
		// bereits geloescht: keine Aenderung, keine Meldung noetig
		if (!((VBUF(video_buffer_page, x) & bit) == bit)) return;
		VBUF(video_buffer_page, x) &= ~bit;
	}
	_display_draw_column(x, x_abs, video_buffer_page, display_page);
	
}

/* Gemeinsamer Kern von display_draw_byte() und display_clear_byte().
   Setzt oder loescht bis zu acht uebereinanderliegende Pixel. Liegt y nicht
   auf einer Page-Grenze, verteilt sich das Byte auf zwei Pufferzeilen und
   wird in zwei Haelften geschrieben. */
static void _display_draw_clear_byte(uint8_t x, uint8_t y, uint8_t byte, bool clear)
{
	if (x >= _display_width || y >= _display_height) return;
	uint8_t x_abs = x + _display_x_pos;
	uint8_t y_abs = y + _display_y_pos;
	if (x_abs >= OLED_PIXEL_X || y_abs >= OLED_PIXEL_Y) return;

	uint8_t shift = y & 7;
	uint8_t video_buffer_page = y >> 3;
	uint8_t display_page = _display_base_page + video_buffer_page;

	if (shift == 0)
	{
		if (clear == false)
		{
			// nichts zu tun, wenn alle Bits schon gesetzt sind
			if ((uint8_t)(VBUF(video_buffer_page, x) | byte) == VBUF(video_buffer_page, x)) return;
			VBUF(video_buffer_page, x) |= byte;
		}
		else
		{
			if (!(VBUF(video_buffer_page, x) & byte)) return;
			VBUF(video_buffer_page, x) &= ~byte;
		}
		_display_draw_column(x, x_abs, video_buffer_page, display_page);
	}
	else
	{
		// Das Byte liegt ueber zwei Pages: in zwei Haelften aufteilen
		uint8_t upper_bits = byte << shift;
		uint8_t lower_bits = byte >> (8 - shift);

		uint8_t old_u = VBUF(video_buffer_page, x);
		uint8_t new_u = clear ? (old_u & ~upper_bits) : (old_u | upper_bits);
		if (new_u != old_u)
		{
			VBUF(video_buffer_page, x) = new_u;
			_display_draw_column(x, x_abs, video_buffer_page, display_page);
		}

		if (lower_bits && video_buffer_page + 1 < _display_pages)
		{
			uint8_t old_l = VBUF(video_buffer_page + 1, x);
			uint8_t new_l = clear ? (old_l & ~lower_bits) : (old_l | lower_bits);
			if (new_l != old_l)
			{
				VBUF(video_buffer_page + 1, x) = new_l;
				_display_draw_column(x, x_abs, video_buffer_page + 1, display_page + 1);
			}
		}
	}
}

/* Waagrechte Linie. Alle Pixel liegen in derselben Page, deshalb genuegt je
   Spalte ein einziges Bit. Deutlich schneller als der Bresenham-Weg und die
   Grundlage fuer gefuellte Rechtecke. */
static void _display_draw_clear_hline(uint8_t x, uint8_t y, uint8_t width, bool clear)
{
	if (width == 0) return;
	if (y >= _display_height) return;
	if (x >= _display_width) return;

	// Breite auf das Zeichenfenster beschneiden
	if ((uint16_t)x + width > _display_width)
		width = (uint8_t)(_display_width - x);

	uint8_t page = y >> 3;
	uint8_t bit  = (1 << (y & 7));
	uint8_t display_page = (uint8_t)(_display_base_page + page);

	for (uint8_t i = 0; i < width; i++)
	{
		uint8_t xi = x + i;

		uint8_t oldv = VBUF(page, xi);
		uint8_t newv = clear ? (uint8_t)(oldv & (uint8_t)~bit) : (uint8_t)(oldv | bit);

		if (newv == oldv) continue;

		VBUF(page, xi) = newv;

		uint8_t x_abs = (uint8_t)(xi + _display_x_pos);
		uint8_t y_abs = (uint8_t)(y  + _display_y_pos);

		// Sicherheitshalber noch gegen das physische Display pruefen
		if (x_abs >= OLED_PIXEL_X || y_abs >= OLED_PIXEL_Y) continue;

		// nur diese eine Spalte melden
		_display_draw_column(xi, x_abs, page, display_page);
	}
}

/* Senkrechte Linie, Pixel fuer Pixel. Hier gibt es keine Abkuerzung, denn die
   Pixel liegen ueber mehrere Pages verteilt. */
static void _display_draw_clear_vline(uint8_t x, uint8_t y, uint8_t height, bool clear)
{
	if (height == 0) return;
	if (x >= _display_width || y >= _display_height) return;

	uint16_t ende = (uint16_t)y + height;
	if (ende > _display_height) ende = _display_height;

	for (uint16_t yy = y; yy < ende; yy++)
		_display_draw_clear_pixel(x, (uint8_t)yy, clear);
}

void display_draw_pixel(uint8_t x, uint8_t y)
{
	_display_draw_clear_pixel(x, y, false);
}

void display_clear_pixel(uint8_t x, uint8_t y)
{
	_display_draw_clear_pixel(x, y, true);
}

void display_draw_byte(uint8_t x, uint8_t y, uint8_t byte)
{
	_display_draw_clear_byte(x, y, byte, false);
}

void display_clear_byte(uint8_t x, uint8_t y, uint8_t byte)
{
	_display_draw_clear_byte(x, y, byte, true);
}

/* Gemeinsamer Kern aller Linienfunktionen, nach Bresenham.

   step gibt an, jedes wievielte Pixel gesetzt wird: 1 ergibt eine durchgehende,
   groessere Werte eine gestrichelte Linie.

   Waagrechte und senkrechte Linien gehen an die schnelleren Sonderfaelle, aber
   nur bei step 1. Eine waagrechte Linie liegt ganz in einer Page, dort genuegt
   je Spalte ein einziges Bit statt eines vollen Pixelaufrufs.

   Punkte ausserhalb des Fensters werden NICHT auf den Rand gezogen. Das wuerde
   die Steigung veraendern und aus der Linie eine andere machen. Stattdessen
   laeuft der Algorithmus ueber die volle Strecke, und die Pixelfunktion laesst
   weg, was ausserhalb liegt. */
static void _display_draw_clear_line_step(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, bool clear, uint8_t step)
{
	if (step == 0) step = 1;

	if (step == 1 && y1 == y2)
	{
		const uint8_t links  = (x1 < x2) ? x1 : x2;
		const uint8_t rechts = (x1 < x2) ? x2 : x1;
		_display_draw_clear_hline(links, y1, (uint8_t)(rechts - links + 1), clear);
		return;
	}
	if (step == 1 && x1 == x2)
	{
		const uint8_t oben  = (y1 < y2) ? y1 : y2;
		const uint8_t unten = (y1 < y2) ? y2 : y1;
		_display_draw_clear_vline(x1, oben, (uint8_t)(unten - oben + 1), clear);
		return;
	}

	int16_t dx = abs((int16_t)x2 - (int16_t)x1);
	int16_t dy = abs((int16_t)y2 - (int16_t)y1);
	int8_t sx  = (x1 < x2) ? 1 : -1;
	int8_t sy  = (y1 < y2) ? 1 : -1;
	int16_t err = dx - dy;

	uint8_t ctr = 0;

	while (1)
	{
		if ((ctr % step) == 0)
		{
			_display_draw_clear_pixel(x1, y1, clear);
		}
		ctr++;

		if (x1 == x2 && y1 == y2) break;

		int16_t e2 = err << 1;
		if (e2 > -dy) { err -= dy; x1 = (uint8_t)(x1 + sx); }
		if (e2 <  dx) { err += dx; y1 = (uint8_t)(y1 + sy); }
	}
}

void display_draw_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	_display_draw_clear_line_step(x1, y1, x2, y2, false, 1);
}

void display_clear_line(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2)
{
	_display_draw_clear_line_step(x1, y1, x2, y2, true, 1);
}

void display_draw_line_with_step(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t step)
{
	_display_draw_clear_line_step(x1, y1, x2, y2, false, step);
}

void display_clear_line_with_step(uint8_t x1, uint8_t y1, uint8_t x2, uint8_t y2, uint8_t step)
{
	_display_draw_clear_line_step(x1, y1, x2, y2, true, step);
}


/* Rechteckrahmen aus zwei waagrechten und zwei senkrechten Linien.

   Gerechnet wird in 16 Bit. In 8 Bit wuerde zum Beispiel y = 60 mit h = 200
   auf 3 zurueckkippen, und die untere Kante erschiene mitten im Bild. */
static void _display_draw_clear_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool clear)
{
	if (w == 0 || h == 0) return;
	if (x >= _display_width || y >= _display_height) return;

	const uint16_t unten  = (uint16_t)y + h - 1;
	const uint16_t rechts = (uint16_t)x + w - 1;

	// Oberkante, und die Unterkante nur wenn sie noch im Fenster liegt
	_display_draw_clear_hline(x, y, w, clear);
	if (h > 1 && unten < _display_height)
		_display_draw_clear_hline(x, (uint8_t)unten, w, clear);

	// Seitenkanten
	if (h > 2)
	{
		const uint8_t seitenhoehe = (uint8_t)(h - 2);
		_display_draw_clear_vline(x, (uint8_t)(y + 1), seitenhoehe, clear);
		if (w > 1 && rechts < _display_width)
			_display_draw_clear_vline((uint8_t)rechts, (uint8_t)(y + 1), seitenhoehe, clear);
	}
}

/* Gefuelltes Rechteck als Stapel waagrechter Linien.

   Die Endzeile wird vorher in 16 Bit ausgerechnet und auf das Fenster
   begrenzt, sonst laeuft y + yy bei grossen Hoehen ueber 255 hinaus. */
static void _display_draw_clear_fill_rect(uint8_t x, uint8_t y, uint8_t w, uint8_t h, bool clear)
{
	if (w == 0 || h == 0) return;
	if (x >= _display_width || y >= _display_height) return;

	uint16_t ende = (uint16_t)y + h;
	if (ende > _display_height) ende = _display_height;

	for (uint16_t yy = y; yy < ende; yy++)
		_display_draw_clear_hline(x, (uint8_t)yy, w, clear);
}

void display_draw_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled)
{
	if (filled) _display_draw_clear_fill_rect(x, y, width, height, false);
	else        _display_draw_clear_rect(x, y, width, height, false);
}

void display_clear_rect(uint8_t x, uint8_t y, uint8_t width, uint8_t height, bool filled)
{
	if (filled) _display_draw_clear_fill_rect(x, y, width, height, true);
	else        _display_draw_clear_rect(x, y, width, height, true);
}

/* Ersetzt bis zu acht Pixel vollstaendig, statt sie nur zu verodern.
   Darin unterscheidet sich deckendes von transparentem Zeichnen: hier loeschen
   auch die Nullbits des Bildes den darunterliegenden Inhalt.
   mask sagt, welche der acht Bits ueberhaupt zum Bild gehoeren. Bei der
   letzten, angebrochenen Page sind das weniger als acht, und die Pixel
   darunter muessen unberuehrt bleiben. */
static void _display_draw_overwrite_byte_masked(uint8_t x, uint8_t y, uint8_t byte, uint8_t mask)
{
	if (x >= _display_width || y >= _display_height) return;

	uint8_t x_abs = x + _display_x_pos;
	uint8_t y_abs = y + _display_y_pos;
	if (x_abs >= OLED_PIXEL_X || y_abs >= OLED_PIXEL_Y) return;

	uint8_t shift = y & 7;
	uint8_t video_buffer_page = y >> 3;
	uint8_t display_page = _display_base_page + video_buffer_page;

	if (shift == 0)
	{
		uint8_t oldv = VBUF(video_buffer_page, x);
		uint8_t newv = (uint8_t)((oldv & (uint8_t)~mask) | (byte & mask));
		if (newv == oldv) return;
		VBUF(video_buffer_page, x) = newv;
		_display_draw_column(x, x_abs, video_buffer_page, display_page);
	}
	else
	{
		// Das Byte liegt ueber zwei Pages: in zwei Haelften aufteilen
		uint8_t upper_byte = byte << shift;
		uint8_t upper_mask = mask << shift;
		uint8_t lower_byte = byte >> (8 - shift);
		uint8_t lower_mask = mask >> (8 - shift);

		uint8_t old_u = VBUF(video_buffer_page, x);
		uint8_t new_u = (uint8_t)((old_u & ~upper_mask) | (upper_byte & upper_mask));
		if (new_u != old_u)
		{
			VBUF(video_buffer_page, x) = new_u;
			_display_draw_column(x, x_abs, video_buffer_page, display_page);
		}

		if (lower_mask && video_buffer_page + 1 < _display_pages)
		{
			uint8_t old_l = VBUF(video_buffer_page + 1, x);
			uint8_t new_l = (uint8_t)((old_l & ~lower_mask) | (lower_byte & lower_mask));
			if (new_l != old_l)
			{
				VBUF(video_buffer_page + 1, x) = new_l;
				_display_draw_column(x, x_abs, video_buffer_page + 1, display_page + 1);
			}
		}
	}
}

/* Gemeinsamer Kern aller sechs Bitmap-Funktionen. Die vier Schalter legen
   fest, welche davon gemeint ist:

     clear        true  = die Pixel des Bildes loeschen
     transparent  true  = nur die gesetzten Pixel verodern
                  false = das ganze Rechteck ersetzen (nur wenn clear false ist)
     horizontal   Ablage der Daten, siehe display_draw.h

   width hat zwei Bedeutungen: die Breite des Bildes und zugleich den Abstand
   zwischen zwei Bildzeilen im Datenblock. Beim Beschneiden am Fensterrand
   verringert sich daher nur die Zahl der gezeichneten Spalten; der
   Zeilenabstand bleibt die tatsaechliche Breite. Andernfalls setzte jede Zeile
   an einer falschen Stelle an und das Bild erschiene schraeg verzogen. */
static void _display_draw_clear_bitmap(uint8_t x, uint8_t y, const uint8_t *bitmap,
                                      uint8_t width, uint8_t height,
                                      bool clear, bool horizontal, bool transparent)
{
	if (!bitmap) return;
	if (width == 0 || height == 0) return;

	// Startpunkt ausserhalb des Fensters: nichts zu zeichnen
	if (x >= _display_width || y >= _display_height) return;

	// Der Zeilenabstand im Datenblock. Bleibt immer die echte Breite.
	const uint8_t stride = width;

	// Wieviele Spalten und Zeilen davon im Fenster liegen
	uint8_t eff_w = width;
	if ((uint16_t)x + eff_w > _display_width)
		eff_w = (uint8_t)(_display_width - x);

	uint8_t eff_h = height;
	if ((uint16_t)y + eff_h > _display_height)
		eff_h = (uint8_t)(_display_height - y);

	if (eff_w == 0 || eff_h == 0) return;

	const uint8_t pages = (uint8_t)((eff_h + 7) >> 3);
	const uint8_t last_rem  = (uint8_t)(eff_h & 7);
	const uint8_t last_mask = (last_rem == 0) ? 0xFFu : (uint8_t)((1u << last_rem) - 1u);

	// Die drei Faelle noch einmal kurz:
	//   clear                       -> die Pixel des Bildes loeschen
	//   transparent                 -> nur die gesetzten Pixel verodern
	//   weder noch                  -> das ganze Rechteck ersetzen

	if (!horizontal)
	{
		for (uint8_t page = 0; page < pages; page++)
		{
			uint8_t mask = (page == (uint8_t)(pages - 1)) ? last_mask : 0xFFu;

			for (uint8_t col = 0; col < eff_w; col++)
			{
				const uint16_t idx = (uint16_t)page * stride + col;
				const uint8_t out = pgm_read_byte(bitmap + idx);

				const uint8_t xx = (uint8_t)(x + col);
				const uint8_t yy = (uint8_t)(y + (page << 3));

				if (clear)
				{
					// nur die Bits der letzten, angebrochenen Page loeschen
					_display_draw_clear_byte(xx, yy, (uint8_t)(out & mask), true);
				}
				else if (transparent)
				{
					// nur die Bits der letzten, angebrochenen Page setzen
					_display_draw_clear_byte(xx, yy, (uint8_t)(out & mask), false);
				}
				else
				{
					// ersetzen, aber nur die Bits der letzten Page
					_display_draw_overwrite_byte_masked(xx, yy, out, mask);
				}
			}
		}
		return;
	}

	// Horizontale Ablage: jede Zielspalte aus einzelnen Quellbits zusammensetzen.
	// Auch hier zaehlt die echte Breite, nicht die beschnittene.
	const uint8_t bytes_per_row = (uint8_t)((stride + 7) >> 3);

	for (uint8_t col = 0; col < eff_w; col++)
	{
		const uint8_t byte_in_row = (uint8_t)(col >> 3);
		const uint8_t mask_src = (uint8_t)(0x80u >> (col & 7));

		for (uint8_t page = 0; page < pages; page++)
		{
			uint8_t out = 0;

			for (uint8_t bit = 0; bit < 8; bit++)
			{
				const uint8_t row = (uint8_t)(page * 8u + bit);
				if (row >= eff_h) break;

				const uint16_t idx = (uint16_t)row * bytes_per_row + byte_in_row;

				const uint8_t v = pgm_read_byte(bitmap + idx);

				if (v & mask_src) out |= (uint8_t)(1u << bit);
			}

			uint8_t mask = (page == (uint8_t)(pages - 1)) ? last_mask : 0xFFu;

			const uint8_t xx = (uint8_t)(x + col);
			const uint8_t yy = (uint8_t)(y + (page << 3));

			if (clear)
			{
				_display_draw_clear_byte(xx, yy, (uint8_t)(out & mask), true);
			}
			else if (transparent)
			{
				_display_draw_clear_byte(xx, yy, (uint8_t)(out & mask), false);
			}
			else
			{
				_display_draw_overwrite_byte_masked(xx, yy, out, mask);
			}
		}
	}
}

/* Holt die Beschreibung eines Bildes aus dem Flash und reicht sie weiter.

   Der Beschreiber liegt selbst im Programmspeicher, alle vier Felder muessen
   also von dort geholt werden. Fuer den Zeiger unbedingt pgm_read_ptr und
   nicht pgm_read_word: auf dem AVR ist ein Zeiger zwei Byte gross und beides
   waere gleich, im Simulator aber acht. */
static void _bitmap_zeichnen(uint8_t x, uint8_t y, const BITMAP_T *bild,
                           bool clear, bool transparent)
{
	if (bild == 0) return;

	const uint8_t *daten  = (const uint8_t *)pgm_read_ptr(&bild->daten);
	const uint8_t  breite = pgm_read_byte(&bild->breite);
	const uint8_t  hoehe  = pgm_read_byte(&bild->hoehe);
	const bool     horizontal = pgm_read_byte(&bild->horizontal) != 0;

	_display_draw_clear_bitmap(x, y, daten, breite, hoehe, clear,
	                           horizontal, transparent);
}

void display_draw_bitmap(uint8_t x, uint8_t y, const BITMAP_T *bild)
{
	_bitmap_zeichnen(x, y, bild, false, false);
}

void display_draw_bitmap_transparent(uint8_t x, uint8_t y, const BITMAP_T *bild)
{
	_bitmap_zeichnen(x, y, bild, false, true);
}

void display_clear_bitmap(uint8_t x, uint8_t y, const BITMAP_T *bild)
{
	_bitmap_zeichnen(x, y, bild, true, true);
}


static inline const uint8_t* _font_glyph_ptr(char c)
{
	if ((uint8_t)c < fontParam.char_first || (uint8_t)c > fontParam.char_last)
	c = '?';

	uint8_t idx = (uint8_t)c - fontParam.char_first;
	// Bei 5x8 / 7x8 gilt: bytes_per_char == fontParam.width
	return &fontData[(uint16_t)idx * fontParam.width];
}


void display_draw_char(uint8_t x, uint8_t y, char c, FontSize_t size, bool clear_bg)
{
	uint16_t scale_q = (uint16_t)size;   // direkt Q8.8 Wert

	if (scale_q == 0)
		scale_q = FONT_SIZE_1X;

	const uint8_t* g = _font_glyph_ptr(c);
	uint8_t src_w = fontParam.width;
	uint8_t src_h = fontParam.height;

	uint16_t dst_w = ((uint16_t)src_w * scale_q + 128) >> 8;
	uint16_t dst_h = ((uint16_t)src_h * scale_q + 128) >> 8;

	if (clear_bg)
	display_clear_rect(x, y, (uint8_t)dst_w, (uint8_t)dst_h, true);

	// Ein Zielpixel deckt beim Verkleinern mehrere Quellpixel ab. Es wird
	// gesetzt, sobald eines davon gesetzt ist. Punktweises Abtasten wuerde
	// ganze Spalten und Zeilen ueberspringen, und ein 5x8-Zeichensatz hat
	// keine ueberfluessigen Pixel: aus der Acht wuerde eine Null.
	// Beim Vergroessern deckt ein Zielpixel genau ein Quellpixel ab, dort
	// verhaelt sich die Rechnung wie vorher.
	for (uint16_t dy = 0; dy < dst_h; dy++)
	{
		uint16_t sy0 = (uint16_t)(((uint32_t)dy       << 8) / scale_q);
		uint16_t sy1 = (uint16_t)(((uint32_t)(dy + 1) << 8) / scale_q);

		if (sy1 <= sy0)   sy1 = (uint16_t)(sy0 + 1);
		if (sy0 >= src_h) sy0 = (uint16_t)(src_h - 1);
		if (sy1 > src_h)  sy1 = src_h;

		// alle beteiligten Quellzeilen als Bitmaske
		uint8_t maske = 0;
		for (uint16_t sy = sy0; sy < sy1; sy++)
		{
			maske |= (uint8_t)(1u << sy);
		}

		for (uint16_t dx = 0; dx < dst_w; dx++)
		{
			uint16_t sx0 = (uint16_t)(((uint32_t)dx       << 8) / scale_q);
			uint16_t sx1 = (uint16_t)(((uint32_t)(dx + 1) << 8) / scale_q);

			if (sx1 <= sx0)   sx1 = (uint16_t)(sx0 + 1);
			if (sx0 >= src_w) sx0 = (uint16_t)(src_w - 1);
			if (sx1 > src_w)  sx1 = src_w;

			// alle beteiligten Quellspalten zusammenfassen
			uint8_t bits = 0;
			for (uint16_t sx = sx0; sx < sx1; sx++)
			{
				bits |= pgm_read_byte(&g[sx]);
			}

			if (bits & maske)
			display_draw_pixel((uint8_t)(x + dx), (uint8_t)(y + dy));
		}
	}
}


/* Gemeinsamer Kern von display_draw_string() und display_draw_string_P().
   from_progmem sagt, ob der Text im Flash oder im RAM liegt. clear_bg loescht
   vor jedem Zeichen dessen Hintergrund samt Zeichenabstand, damit beim
   Ueberschreiben keine Reste des vorigen Textes stehen bleiben. Beide
   oeffentlichen Funktionen rufen mit clear_bg = true auf. */
static void _display_draw_string(uint8_t x, uint8_t y, const char* s, FontSize_t size, bool clear_bg, bool from_progmem)
{
	if (!s) return;

	uint16_t scale_q = (uint16_t)size;
	if (scale_q == 0)
	scale_q = FONT_SIZE_1X;

	uint8_t src_w = fontParam.width;
	uint8_t src_h = fontParam.height;

	// skalierte Masse
	uint16_t dst_w       = ((uint16_t)src_w * scale_q + 128) >> 8;
	uint16_t dst_h       = ((uint16_t)src_h * scale_q + 128) >> 8;
	uint16_t dst_spacing = ((uint16_t)fontParam.spacing * scale_q + 128) >> 8;

	// advance (pro Zeichen)
	uint16_t adv16 = dst_w + dst_spacing;
	uint8_t adv = (uint8_t)(adv16 > 255 ? 255 : adv16);

	while (1)
	{
		char c = from_progmem ? pgm_read_byte(s++) : *s++;
		if (c == '\0') break;

		// Hintergrund samt Zeichenabstand loeschen, sonst bleiben beim
		// Ueberschreiben Reste des vorigen Textes stehen
		if (clear_bg)
		{
			uint16_t bg_w16 = dst_w + dst_spacing;
			uint8_t bg_w = (uint8_t)(bg_w16 > 255 ? 255 : bg_w16);
			uint8_t bg_h = (uint8_t)(dst_h > 255 ? 255 : dst_h);

			display_clear_rect(x, y, bg_w, bg_h, true);
		}

		// Zeichen zeichnen
		display_draw_char(x, y, c, size, false);

		// Cursor auf das naechste Zeichen setzen
		x = (uint8_t)(x + adv);
	}
}

void display_draw_string(uint8_t x, uint8_t y, const char* s, FontSize_t size)
{
	_display_draw_string(x, y, s, size, true, false);
}

void display_draw_string_P(uint8_t x, uint8_t y, PGM_P s, FontSize_t size)
{
	_display_draw_string(x, y, s, size, true, true);
}


void display_draw_zahl(uint8_t x, uint8_t y, uint16_t wert, FontSize_t size)
{
	// Rueckwaerts in den Puffer schreiben: die letzte Ziffer steht rechts.
	// 65535 sind fuenf Ziffern, dazu die abschliessende Null.
	char puffer[6];
	uint8_t i = sizeof puffer - 1;

	puffer[i] = '\0';
	do
	{
		puffer[--i] = (char)('0' + (wert % 10));
		wert /= 10;
	} while (wert > 0);

	display_draw_string(x, y, &puffer[i], size);
}


void display_draw_binaer(uint8_t x, uint8_t y, uint8_t wert, FontSize_t size)
{
	char puffer[9];

	// Bit 7 steht links: die Maske wandert von 0x80 nach 0x01
	for (uint8_t i = 0; i < 8; i++)
	{
		puffer[i] = (wert & (uint8_t)(0x80u >> i)) ? '1' : '0';
	}
	puffer[8] = '\0';

	display_draw_string(x, y, puffer, size);
}


void display_draw_printf(uint8_t x, uint8_t y, FontSize_t size, const char *format, ...)
{
	// Der Puffer liegt auf dem Stack und ist nach dem Aufruf wieder frei.
	char puffer[DISPLAY_DRAW_TEXT_MAX];
	va_list argumente;

	va_start(argumente, format);
	vsnprintf(puffer, sizeof puffer, format, argumente);
	va_end(argumente);

	display_draw_string(x, y, puffer, size);
}


void display_draw_printf_P(uint8_t x, uint8_t y, FontSize_t size, PGM_P format, ...)
{
	char puffer[DISPLAY_DRAW_TEXT_MAX];
	va_list argumente;

	va_start(argumente, format);
	vsnprintf_P(puffer, sizeof puffer, format, argumente);
	va_end(argumente);

	display_draw_string(x, y, puffer, size);
}