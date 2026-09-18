/*-------------------------------------------------------------------------*\
| Datei:        traffic_racer_bitmaps.h
| Version:      1.0
| Projekt:      Traffic Racer - Videospiel
| Beschreibung: Bilder und Sprites des Spiels. Die Bilddaten liegen im Flash,
|               die Sprites werden in traffic_racer.c registriert.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   11.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/


#ifndef TRAFFIC_RACER_BITMAPS_H_
#define TRAFFIC_RACER_BITMAPS_H_

#include <avr/io.h>
#include <avr/pgmspace.h>
#include "../../megalib/display/display_draw.h"
#include "../../megalib/display/display_draw_sprite.h"

// Die beiden Vollbilder. Breite, Hoehe und Ablageart stecken im Bild
// selbst, beim Zeichnen genuegt deshalb display_draw_bitmap(0, 0, &Startbild).
extern const BITMAP_T Startbild PROGMEM;
extern const BITMAP_T Gameoverbild PROGMEM;
extern SPRITE_T Strasse_Sprite;
extern SPRITE_T Racecar_Sprite;
extern SPRITE_T Explosion_Sprite;
extern SPRITE_T Hydrant_Sprite;
extern SPRITE_T Baustellenbalken_Sprite;

#endif /* TRAFFIC_RACER_BITMAPS_H_ */