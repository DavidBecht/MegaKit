/*-------------------------------------------------------------------------*\
| Datei:        knight_rider_melodie.c
| Version:      1.0
| Projekt:      Toene und Melodien auf der MEGACARD
| Beschreibung: Tontabelle, erzeugt von megasound.py. Nicht von Hand aendern.
| Schaltung:    MEGACARD V6.11, Piezo an PB3
| Autor:        megasound.py
| Erstellung:   13.09.2026
|
| Aenderung:
\*-------------------------------------------------------------------------*/

// Ein Eintrag: OCR0, Vorteilerbits fuer TCCR0, Dauer in Ticks.
// OCR0 gleich null bedeutet Pause. Tickrate: 100 Hz.
//
// Dazu gehoert im Programm:
//   typedef struct { uint8_t ocr; uint8_t clock; uint8_t ticks; } TON_T;
//   Timer0: TCCR0 = (1<<WGM01) | (1<<COM00), Tonhoehe ueber OCR0,
//   Vorteiler ueber die unteren drei Bit von TCCR0.
//
// Die Tabelle ist mit static bewusst auf diese Datei beschraenkt.
// Nach aussen sichtbar ist nur die Melodie darunter, und die bringt
// ihre Laenge selbst mit.

#include <avr/pgmspace.h>
#include "../../megalib/sound/sound.h"

static const TON_T KnightRider_toene[] PROGMEM = {
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 0, 0, 13 },                // Pause
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 89, 4, 12 },               // C4  261.6 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 0, 0, 13 },                // Pause
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 79, 4, 12 },               // D4  293.7 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 89, 4, 12 },               // C4  261.6 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 84, 4, 12 },               // Cis4  277.2 Hz, Vorteiler 256
	{ 252, 3, 25 },              // Fis4  370.0 Hz, Vorteiler 64
	{ 238, 3, 12 },              // G4  392.0 Hz, Vorteiler 64
	{ 252, 3, 12 },              // Fis4  370.0 Hz, Vorteiler 64
	{ 168, 3, 100 },             // Cis5  554.4 Hz, Vorteiler 64
	{ 0, 0, 75 },                // Pause
	{ 119, 3, 12 },              // G5  784.0 Hz, Vorteiler 64
	{ 126, 3, 12 },              // Fis5  740.0 Hz, Vorteiler 64
	{ 168, 3, 112 },             // Cis5  554.4 Hz, Vorteiler 64
	{ 0, 0, 38 },                // Pause
	{ 252, 3, 25 },              // Fis4  370.0 Hz, Vorteiler 64
	{ 238, 3, 12 },              // G4  392.0 Hz, Vorteiler 64
	{ 252, 3, 12 },              // Fis4  370.0 Hz, Vorteiler 64
	{ 168, 3, 25 },              // Cis5  554.4 Hz, Vorteiler 64
	{ 126, 3, 25 },              // Fis5  740.0 Hz, Vorteiler 64
	{ 141, 3, 150 },             // E5  659.3 Hz, Vorteiler 64
};

// Absturz: ein Ton, der in gut einer halben Sekunde nach unten faellt.
// Von Hand zusammengestellt, dafuer braucht es keine MIDI-Datei.
static const TON_T Absturz_toene[] PROGMEM = {
	{ 103, 3, 7 },       //  901.4 Hz, Vorteiler 64
	{ 129, 3, 7 },       //  721.2 Hz, Vorteiler 64
	{ 166, 3, 7 },       //  561.4 Hz, Vorteiler 64
	{ 217, 3, 7 },       //  430.0 Hz, Vorteiler 64
	{ 72, 4, 7 },        //  321.1 Hz, Vorteiler 256
	{ 97, 4, 7 },        //  239.2 Hz, Vorteiler 256
	{ 137, 4, 7 },       //  169.8 Hz, Vorteiler 256
	{ 212, 4, 7 },       //  110.0 Hz, Vorteiler 256
};

// MELODIE() laesst den Compiler die Eintraege zaehlen. Die Laenge kann
// dadurch nicht zur Tabelle daneben passen und trotzdem falsch sein.
const MELODIE_T KnightRider PROGMEM = MELODIE(KnightRider_toene);

// MELODIE() laesst den Compiler die Eintraege zaehlen. Die Laenge kann
// dadurch nicht zur Tabelle daneben passen und trotzdem falsch sein.
const MELODIE_T Absturz PROGMEM = MELODIE(Absturz_toene);

// Damit das Programm die Melodie findet, diese Zeile in die Header-Datei
// neben dieser Datei eintragen:
//   extern const MELODIE_T KnightRider PROGMEM;
//	 extern const MELODIE_T Absturz PROGMEM;