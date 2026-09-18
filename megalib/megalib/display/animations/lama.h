/*-------------------------------------------------------------------------*\
| Datei:        lama.h
| Version:      1.0
| Projekt:      Zeichenbibliothek fuer die MEGACARD
| Beschreibung: Sieben Animationsbilder eines Lamas, je 48x48 Pixel.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   25.02.2026
|
| Aenderung:    
\*-------------------------------------------------------------------------*/


#ifndef LAMA_H_
#define LAMA_H_
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "../display_draw.h"

// Sieben Bilder zu je 48x48 Pixel, vertikal abgelegt.
// Verwendung als Sprite:
//     SPRITE_T lama = { .frames = lama_bilder, .frame_count = LAMA_BILDER, ... };
#define LAMA_BILDER 7
extern const BITMAP_T * const lama_bilder[LAMA_BILDER];



#endif /* LAMA_H_ */