/*-------------------------------------------------------------------------*\
| Datei:        sound.c
| Version:      1.0
| Projekt:      Toene und Melodien auf der MEGACARD
| Beschreibung: Bibliotheksfunktionen (Implementierung)
| Schaltung:    MEGACARD V6.11, Piezo an PB3
| Autor:
| Erstellung:
|
| Aenderung:
\*-------------------------------------------------------------------------*/

#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#include <avr/interrupt.h>
#include <avr/io.h>
#include <avr/pgmspace.h>

#include "sound.h"

/* --- Timer0 erzeugt den Ton -----------------------------------------------
   CTC-Modus mit umschaltendem Ausgang: bei jedem Vergleichstreffer kippt
   OC0 (PB3). Eine volle Schwingung braucht also zwei Treffer, daher die
   Zwei in der Formel:

       f = F_CPU / (2 * Vorteiler * (1 + OCR0))

   Das Tastverhaeltnis liegt dabei bauartbedingt bei 50 Prozent, genau das
   will ein Piezo. Die Tonhoehe steht in OCR0, der Vorteiler in den unteren
   drei Bit von TCCR0. Alles Null in diesen Bits haelt den Timer an, dann
   ist Ruhe. */
#define TCCR0_GRUNDBITS ((1 << WGM01) | (1 << COM00))
#define TCCR0_TAKTMASKE ((1 << CS02) | (1 << CS01) | (1 << CS00))

/* --- Timer2 taktet den Ablauf ---------------------------------------------
   CTC mit Vorteiler 1024. Bei 12 MHz sind das 11718,75 Schritte je Sekunde,
   mit OCR2 = 116 ergibt das 100,2 Ausloesungen je Sekunde. Das passt zu
   SOUND_TICK_HZ und damit zu den Dauern aus megasound.py. */
#define OCR2_WERT ((F_CPU / 1024UL / SOUND_TICK_HZ) - 1)

/* Obergrenze fuer die Warteschleifen in _ton_an und _ton_aus. Der laengste vorkommende
   Halbzyklus betraegt bei 110 Hz rund 4,5 ms, das sind 54000 Takte. Eine
   Schleifenrunde kostet etwa sechs Takte, 12000 Runden reichen also aus.
   Die Grenze verhindert ein Haengenbleiben, wenn der Pin nicht wie erwartet
   reagiert, und im Simulator, der Timer0 nicht nachbildet. */
#define WARTEN_MAX 12000u

// Diese Werte werden in der ISR gelesen und ausserhalb geschrieben und sind
// deshalb durchgehend volatile. Andernfalls duerfte der Uebersetzer sie in
// Registern halten und Aenderungen von aussen blieben unbemerkt.
static const TON_T *volatile _melodie = 0;
static volatile uint8_t _laenge = 0;
static volatile uint8_t _index = 0;
static volatile uint8_t _rest = 0;          // verbleibende Ticks des Tones
static volatile bool _aktiv = false;
static volatile int8_t _wiederholungen = 1; // SOUND_ENDLOS = ohne Ende
static volatile uint8_t _durchlaeufe = 0;   // schon vollstaendig gespielt
static volatile bool _stumm = false;        // Ausgabe unterdrueckt

/* Wartet auf den naechsten Vergleichstreffer von Timer0.

   Im CTC-Modus ist OCR0 nicht gepuffert: ein neuer Wert wirkt sofort, auch
   mitten in einer Halbwelle. Dabei entsteht ein verkuerzter Impuls, den der
   Piezo als kurzes Pfeifen nahe seiner Eigenfrequenz wiedergibt. Unmittelbar
   nach einem Vergleichstreffer steht TCNT0 dagegen auf null, und ein neuer
   Wert greift sauber zum naechsten Durchlauf.

   Wird aus der Unterbrechungsroutine von Timer2 aufgerufen und blockiert
   dort hoechstens einen Halbzyklus lang. */
static void _auf_umschaltpunkt_warten(void)
{
	TIFR = (1 << OCF0);                 // Merker loeschen, durch Schreiben einer 1
	for (uint16_t n = 0; n < WARTEN_MAX; n++)
	{
		if (TIFR & (1 << OCF0)) return;
	}
}

/* Schaltet den Ton ab und legt den Pin definiert auf low.

   Solange COM00 gesetzt ist, steuert der Timer den Pin OC0 und PORTB bleibt
   ohne Wirkung. TCCR0 wird deshalb vollstaendig geloescht; damit ist OC0 vom
   Pin getrennt, PORTB uebernimmt wieder, und das Loeschen von PB3 wirkt.

   Laeuft der Ton noch, wird zuvor abgewartet, bis der Ausgang von selbst auf
   low wechselt. Ein Abbruch waehrend einer High-Halbwelle hinterliesse einen
   verkuerzten Impuls. Endet der Ton in einer Low-Halbwelle, entsteht kein
   zusaetzlicher Wechsel, und es kann sofort abgeschaltet werden. */
static void _ton_aus(void)
{
	if (TCCR0 & TCCR0_TAKTMASKE)
	{
		for (uint16_t n = 0; n < WARTEN_MAX; n++)
		{
			if (!(PINB & (1 << PB3))) break;
		}
	}
	TCCR0 = 0;
	PORTB &= (uint8_t)~(1 << PB3);
}

/* Legt Tonhoehe und Vorteiler an. ocr gleich null bedeutet Pause.

   Laeuft bereits ein Ton, wird der Wechsel an einen Vergleichstreffer
   gebunden, siehe _auf_umschaltpunkt_warten(). Steht der Timer, beginnt der
   neue Ton mit TCNT0 = 0 von vorn. */
static void _ton_an(uint8_t ocr, uint8_t clock)
{
	// Pause oder stummgeschaltet: der Ablauf zaehlt weiter, der Timer bleibt aus
	if (ocr == 0 || clock == 0 || _stumm)
	{
		_ton_aus();
		return;
	}

	if (TCCR0 & TCCR0_TAKTMASKE)
	{
		_auf_umschaltpunkt_warten();
	}
	else
	{
		TCNT0 = 0;
	}
	OCR0  = ocr;
	TCCR0 = (uint8_t)(TCCR0_GRUNDBITS | (clock & TCCR0_TAKTMASKE));
}

/* Holt den Eintrag mit dem angegebenen Index aus dem Flash und legt ihn an. */
static void _eintrag_setzen(uint8_t i)
{
	uint8_t ocr   = pgm_read_byte(&_melodie[i].ocr);
	uint8_t clock = pgm_read_byte(&_melodie[i].clock);

	_rest = pgm_read_byte(&_melodie[i].ticks);
	if (_rest == 0)
	{
		_rest = 1;
	}
	_ton_an(ocr, clock);
}

void sound_init(void)
{
	DDRB |= (1 << PB3);            // nur dieses Bit, PB0 und PB1 sind I2C
	PORTB &= (uint8_t)~(1 << PB3);

	TCCR0 = 0;                     // Timer aus, OC0 vom Pin getrennt
	OCR0  = 0;

	TCCR2 = (1 << WGM21) | (1 << CS22) | (1 << CS21) | (1 << CS20);
	OCR2 = (uint8_t)OCR2_WERT;
	TIMSK |= (1 << OCIE2);

	sei();
}

void sound_melodie(const MELODIE_T *melodie, int8_t wiederholungen)
{
	if (melodie == 0)
	{
		sound_stop();
		return;
	}

	// Der Verbund liegt im Programmspeicher, beide Felder sind daher von dort
	// zu lesen. Anschliessend wird nur noch mit Tabelle und Laenge gearbeitet.
	//
	// Fuer den Zeiger ist pgm_read_ptr zu verwenden, nicht pgm_read_word: auf
	// dem AVR sind Zeiger zwei Byte breit und beide Varianten gleichwertig, in
	// der PC-Simulation dagegen acht Byte. pgm_read_word wuerde dort die oberen
	// Bytes abschneiden.
	const TON_T *toene = (const TON_T *)pgm_read_ptr(&melodie->toene);
	uint8_t      laenge = pgm_read_byte(&melodie->laenge);

	if (toene == 0 || laenge == 0)
	{
		sound_stop();
		return;
	}

	// Waehrend des Umschaltens darf der Ablauf nicht dazwischenfunken
	TIMSK &= (uint8_t)~(1 << OCIE2);
	_melodie = toene;
	_laenge = laenge;
	_index = 0;
	_wiederholungen = (wiederholungen == 0) ? 1 : wiederholungen;
	_durchlaeufe = 0;
	_aktiv = true;
	_eintrag_setzen(0);
	TIMSK |= (1 << OCIE2);
}

void sound_ton(uint8_t ocr, uint8_t clock, uint8_t dauer_ticks)
{
	TIMSK &= (uint8_t)~(1 << OCIE2);
	_melodie = 0;
	_laenge = 0;
	_index = 0;
	_wiederholungen = 1;
	_durchlaeufe = 0;
	_rest = (dauer_ticks == 0) ? 1 : dauer_ticks;
	_aktiv = true;
	_ton_an(ocr, clock);
	TIMSK |= (1 << OCIE2);
}

void sound_stumm(bool stumm)
{
	// Zuerst den Merker setzen, danach abschalten. Trifft dazwischen ein
	// Tonwechsel aus der ISR ein, sieht er den Merker bereits gesetzt.
	_stumm = stumm;
	if (stumm)
	{
		_ton_aus();
	}
}

bool sound_ist_stumm(void)
{
	return _stumm;
}

void sound_stop(void)
{
	_aktiv = false;
	_melodie = 0;
	_laenge = 0;
	_rest = 0;
	_ton_aus();
}

bool sound_laeuft(void)
{
	return _aktiv;
}

/* Ein Tick der Ablaufsteuerung. Der Handler ist kurz gehalten, damit er das
   Zeichnen der Sprites nicht ausbremst. Einzige Ausnahme ist ein Tonwechsel:
   dabei wird auf den naechsten Umschaltpunkt von Timer0 gewartet, hoechstens
   einen Halbzyklus lang, bei 110 Hz also rund 4,5 ms. */
ISR(TIMER2_COMP_vect)
{
	if (!_aktiv)
	{
		return;
	}

	if (_rest > 1)
	{
		_rest--;
		return;
	}

	// Einzelton ohne Melodie: danach ist Schluss
	if (_melodie == 0)
	{
		sound_stop();
		return;
	}

	_index++;
	if (_index >= _laenge)
	{
		// Melodie vollstaendig abgespielt, gegebenenfalls wiederholen
		if (_durchlaeufe < 255)
		{
			_durchlaeufe++;
		}
		if (_wiederholungen == SOUND_ENDLOS
		    || _durchlaeufe < (uint8_t)_wiederholungen)
		{
			_index = 0;
			_eintrag_setzen(0);
			return;
		}
		sound_stop();
		return;
	}
	_eintrag_setzen(_index);
}
