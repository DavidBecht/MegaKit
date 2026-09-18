/*-------------------------------------------------------------------------*\
| Datei:        random.c
| Version:      1.1
| Projekt:      Zufallszahlen fuer die MEGACARD
| Beschreibung: Bibliotheksfunktionen (Implementierung)
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   24.03.2025
|
| Aenderung:    Doku vereinheitlicht
\*-------------------------------------------------------------------------*/

#include <stdlib.h>
#include "random.h"

// --- ADC-Rausch-Seeding ----------------------------------------------------

/**
 * @brief Konfiguriert den ADC fuer die Seed-Generierung.
 *
 * Differenzmodus ADC0 minus ADC1 mit 200x Gain und AVCC als Referenz.
 * Ein offener Eingang erzeugt in dieser Betriebsart analoges
 * Rauschen, das als Startwert dient.
 */
static void _adc_init(void)
{
    ADMUX  = (1 << REFS0) | (1 << MUX3) | (1 << MUX1) | (1 << MUX0);
    ADCSRA = (1 << ADEN)  | (1 << ADPS2) | (1 << ADPS1) | (1 << ADPS0);
}

/**
 * @brief Startet eine ADC-Einzelwandlung und gibt das Ergebnis zurueck.
 *
 * Blockiert bis die Wandlung abgeschlossen ist (~104 us bei Prescaler 128, 12 MHz).
 *
 * @return 10-Bit ADC-Ergebnis (0..1023).
 */
static uint16_t _adc_read(void)
{
    ADCSRA |= (1 << ADSC);
    while (ADCSRA & (1 << ADSC));
    return ADCW;
}

// --- Oeffentliche Funktionen -----------------------------------------------
// Beschreibung der einzelnen Funktionen siehe random.h.

void random_init(void)
{
    _adc_init();

    uint16_t seed = 0;
    for (uint8_t i = 0; i < 32; i++)
        seed ^= _adc_read();

    srand(seed);
}

void random_init_fixed_seed(uint16_t seed)
{
	srand(seed);
}

uint8_t random_uint8(uint8_t stop_exclusive)
{
    return random_uint8_range(0, stop_exclusive);
}

float random_float(void)
{
    return (float)random_uint8(255) / 254.0f;
}

uint8_t random_uint8_range(uint8_t start_inclusive, uint8_t stop_exclusive)
{
    uint8_t low, high;

    if (start_inclusive < stop_exclusive)
    {
        low  = start_inclusive;
        high = stop_exclusive;
    }
    else if (start_inclusive > stop_exclusive)
    {
        low  = stop_exclusive;
        high = start_inclusive;
    }
    else
    {
        return start_inclusive;
    }

    return (uint8_t)(rand() % (high - low)) + low;
}
