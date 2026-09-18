/*-------------------------------------------------------------------------*\
| Datei:        knight_rider_melodien.h
| Version:      1.1
| Projekt:      Toene und Melodien auf der MEGACARD
| Beschreibung: Die Melodien des Traffic Racer, erzeugt von megasound.py.
|               Wer eine neue erzeugt, traegt sie hier ein.
| Schaltung:    MEGACARD V6.11, Piezo an PB3
| Autor:        David Bechtold
| Erstellung:   13.09.2026
|
| Aenderung:    Melodien als MELODIE_T, Laenge kommt vom Compiler
\*-------------------------------------------------------------------------*/

#ifndef KNIGHT_RIDER_MELODIEN_H_
#define KNIGHT_RIDER_MELODIEN_H_

#include <avr/pgmspace.h>
#include <stdint.h>

#include "../../megalib/sound/sound.h"

/* Jede von megasound.py erzeugte .c-Datei enthaelt eine Melodie: die Tontabelle
   und ihre Laenge zusammen in einem MELODIE_T. Die Laenge zaehlt der Compiler,
   sie kann also nicht zur Tabelle daneben passen und trotzdem falsch sein.
   Hier steht deshalb nur noch eine Zeile je Melodie.

   Aufruf im Programm:
       #include "knight_rider_melodien.h"
       sound_melodie(&KnightRider, SOUND_ENDLOS); */

/** @brief Titelmelodie Knight Rider, aus knight_rider_melodie.c. */
extern const MELODIE_T KnightRider PROGMEM;
extern const MELODIE_T Absturz PROGMEM;

#endif /* KNIGHT_RIDER_MELODIEN_H_ */
