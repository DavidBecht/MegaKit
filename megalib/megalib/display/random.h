/*-------------------------------------------------------------------------*\
| Datei:        random.h
| Version:      1.1
| Projekt:      Zufallszahlen fuer die MEGACARD
| Beschreibung: Pseudo-Zufallszahlen, wahlweise mit einem Startwert aus dem
|               Rauschen des Analogeingangs.
| Schaltung:    MEGACARD V6.11
| Autor:        David Bechtold
| Erstellung:   24.03.2025
|
| Aenderung:    Doku vereinheitlicht
\*-------------------------------------------------------------------------*/

#ifndef RANDOM_H_
#define RANDOM_H_

#include <avr/io.h>
#include <stdint.h>

/**
 * @brief Initialisiert den Pseudo-Zufallsgenerator mit einem echten Seed.
 *
 * Liest 32x ADC-Rauschen eines floating Pins (ADC0/ADC1 Differenz, 200x Gain)
 * und XOR-verknuepft die Werte zu einem Seed fuer srand().
 * Muss einmalig vor dem ersten Aufruf von random_uint8() aufgerufen werden.
 */
void random_init(void);

/**
 * @brief Initialisiert den Pseudo-Zufallsgenerator mit einem festen Startwert.
 *
 * Derselbe Startwert erzeugt stets dieselbe Zahlenfolge und macht Ablaeufe
 * damit reproduzierbar, was der Fehlersuche dient. Einmalig vor dem ersten
 * Aufruf von random_uint8() aufzurufen.
 *
 * @param seed Startwert der Zahlenfolge.
 */
void random_init_fixed_seed(uint16_t seed);

/**
 * @brief Gibt eine Zufallszahl im Bereich [0, stop_exclusive) zurueck.
 *
 * @param stop_exclusive Obere Grenze (exklusiv). Muss > 0 sein.
 * @return Zufaellige Zahl in [0, stop_exclusive).
 */
uint8_t random_uint8(uint8_t stop_exclusive);

/**
 * @brief Gibt eine Zufallszahl im Bereich [start_inclusive, stop_exclusive) zurueck.
 *
 * Wenn start_inclusive == stop_exclusive, wird start_inclusive zurueckgegeben.
 * Wenn start_inclusive > stop_exclusive, werden die Parameter automatisch getauscht.
 *
 * @param start_inclusive Untere Grenze (inklusive).
 * @param stop_exclusive  Obere Grenze (exklusiv).
 * @return Zufaellige Zahl im angegebenen Bereich.
 */
uint8_t random_uint8_range(uint8_t start_inclusive, uint8_t stop_exclusive);

/**
 * @brief Gibt eine zufaellige Gleitkommazahl im Bereich [0.0, 1.0] zurueck.
 *
 * Hinweis: Der ATmega16 besitzt keine Gleitkommaeinheit; entsprechende
 * Operationen werden in Software nachgebildet und sind deutlich langsamer als
 * Ganzzahloperationen. Nur einzusetzen, wenn ein Gleitkommawert erforderlich ist.
 *
 * @return Zufaelliger float in [0.0, 1.0].
 */
float random_float(void);

#endif /* RANDOM_H_ */
