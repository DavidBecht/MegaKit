/*-------------------------------------------------------------------------*\
| Datei:        sound.h
| Version:      1.1
| Projekt:      Toene und Melodien auf der MEGACARD
| Beschreibung: Einstimmiger Tongenerator fuer den Piepser an OC0 (PB3).
|               Spielt Melodien ab, die megasound.py aus einer MIDI-Datei
|               erzeugt hat.
| Schaltung:    MEGACARD V6.11, Piezo an PB3
| Autor:        David Bechtold
| Erstellung:   12.09.2026
|
| Aenderung:    Doku vereinheitlicht
\*-------------------------------------------------------------------------*/

#ifndef SOUND_H_
#define SOUND_H_

/* Nebenlaeufigkeit: das Modul haelt genau einen Zustand und ist nicht fuer den
   gleichzeitigen Gebrauch aus mehreren Unterbrechungsroutinen ausgelegt.
   Aufrufe aus dem Hauptprogramm und aus einer Unterbrechungsroutine sind
   zulaessig, zwei gleichzeitige Aufrufer nicht. */

#include <avr/io.h>
#include <avr/pgmspace.h>
#include <stdbool.h>
#include <stdint.h>

/* ===========================================================================
   Einstellungen und Makros

   Alles, was der Praeprozessor braucht, steht hier am Anfang beisammen.
   Weiter unten folgen nur noch Typen und Funktionen.
   =========================================================================== */

/**
 * @brief Takt der Ablaufsteuerung in Hertz.
 *
 * Eine Dauer von 1 in einem TON_T entspricht einer Hundertstelsekunde. Der
 * Wert muss mit dem uebereinstimmen, den megasound.py bei der Umrechnung
 * verwendet hat.
 */
#define SOUND_TICK_HZ 100


/**
 * @brief Baut aus einer Tontabelle eine Melodie.
 *
 * Die Zahl der Eintraege wird zur Uebersetzungszeit ermittelt. Das Makro ist
 * nur in derjenigen Uebersetzungseinheit verwendbar, in der die Tabelle
 * definiert ist, da ihre Groesse sonst nicht bekannt ist. Aus diesem Grund
 * gehoert die Laenge in den Verbund und wird nicht am Aufrufort bestimmt.
 *
 * Verwendung, so erzeugt es auch megasound.py:
 *
 *     static const TON_T Meine_toene[] PROGMEM = { ... };
 *     const MELODIE_T Meine PROGMEM = MELODIE(Meine_toene);
 *
 * @param tabelle Name der Tontabelle. Hoechstens 255 Eintraege.
 */
#define MELODIE(tabelle) { (tabelle), (uint8_t)(sizeof(tabelle) / sizeof(TON_T)) }


/** @brief Endloses Wiederholen fuer sound_melodie(). */
#define SOUND_ENDLOS (-1)


/**
 * @brief Ein Eintrag einer Melodie, drei Byte gross.
 *
 * Die Tonhoehe ergibt sich aus
 *     f = F_CPU / (2 * Vorteiler * (1 + ocr))
 * Beide Werte werden von megasound.py berechnet.
 */
typedef struct {
	uint8_t ocr;    ///< Timerwert fuer die Tonhoehe. 0 bedeutet Pause.
	uint8_t clock;  ///< Vorteilerwahl: 1 = 1, 2 = 8, 3 = 64, 4 = 256, 5 = 1024
	uint8_t ticks;  ///< Dauer in Ticks der Ablaufsteuerung, siehe SOUND_TICK_HZ
} TON_T;


/**
 * @brief Eine vollstaendige Melodie: die Tontabelle und ihre Laenge.
 *
 * Die Laenge wird zur Uebersetzungszeit aus der Tontabelle ermittelt und kann
 * daher nicht von ihr abweichen.
 *
 * Instanzen werden ausschliesslich ueber das Makro MELODIE() angelegt.
 */
typedef struct {
	const TON_T *toene;   ///< Zeiger auf die Tontabelle im PROGMEM
	uint8_t      laenge;  ///< Zahl der Eintraege, vom Compiler gesetzt
} MELODIE_T;


/**
 * @brief Richtet die beiden Timer ein und schaltet PB3 auf Ausgang.
 *
 * Timer0 erzeugt den Ton, Timer2 taktet den Ablauf. Muss einmal vor dem ersten
 * Ton aufgerufen werden.
 *
 * @note Schaltet die globalen Interrupts frei (sei()). Timer0 und Timer2 sind
 *       danach der Tonausgabe vorbehalten, Timer1 bleibt dem Sprite-System.
 *       Von PORTB wird ausschliesslich PB3 veraendert; PB0 und PB1 bleiben
 *       unberuehrt, da sie das Display bedienen.
 */
void sound_init(void);

/**
 * @brief Startet eine Melodie aus dem Flash.
 *
 * Kehrt sofort zurueck, gespielt wird im Hintergrund. Eine laufende Melodie
 * wird abgebrochen.
 *
 * Aufruf: sound_melodie(&KnightRider, SOUND_ENDLOS);
 *
 * @param melodie        Zeiger auf die Melodie im PROGMEM, also auf einen
 *                       MELODIE_T. NULL haelt den Ton an, wie sound_stop().
 * @param wiederholungen Wie oft die Melodie insgesamt laeuft. 1 spielt sie
 *                       einmal, 3 dreimal, SOUND_ENDLOS wiederholt sie, bis
 *                       sound_stop() kommt. 0 wird wie 1 behandelt.
 */
void sound_melodie(const MELODIE_T *melodie, int8_t wiederholungen);

/**
 * @brief Spielt einen einzelnen Ton, zum Beispiel fuer einen Aufprall.
 *
 * Kehrt sofort zurueck, gespielt wird im Hintergrund.
 *
 * @param ocr         Timerwert fuer die Tonhoehe, 0 bedeutet Stille.
 * @param clock       Vorteilerwahl wie im TON_T.
 * @param dauer_ticks Dauer in Ticks, bei 100 Hz also in Hundertstelsekunden.
 *                    0 wird wie 1 behandelt.
 */
void sound_ton(uint8_t ocr, uint8_t clock, uint8_t dauer_ticks);

/**
 * @brief Schaltet die Tonausgabe stumm oder wieder hoerbar.
 *
 * Im stummen Zustand laeuft die Ablaufsteuerung unveraendert weiter, nur der
 * Tongenerator bleibt ausgeschaltet. Eine laufende Melodie schreitet also
 * lautlos fort und ist nach dem Aufheben wieder an der passenden Stelle zu
 * hoeren. Hoerbar wird sie ab dem naechsten Tonwechsel, bei einem einzelnen
 * Ton aus sound_ton() erst mit dem naechsten Aufruf.
 *
 * Der Zustand bleibt ueber sound_melodie(), sound_ton() und sound_stop()
 * hinweg erhalten. Die dauerhafte Speicherung ist Sache der Anwendung.
 *
 * @param stumm true = keine Ausgabe, false = Ausgabe wieder zulassen.
 */
void sound_stumm(bool stumm);

/**
 * @brief Zeigt an, ob die Tonausgabe stummgeschaltet ist.
 *
 * @return true, wenn stummgeschaltet, sonst false.
 */
bool sound_ist_stumm(void);

/**
 * @brief Bricht sofort ab und macht den Piepser still.
 *
 * @note Auch aus einer Unterbrechungsroutine aufrufbar. Die Ablaufsteuerung
 *       wird zuerst stillgelegt und erst danach werden die Werte
 *       zurueckgesetzt, sodass kein teilweise geaenderter Zustand wirksam wird.
 */
void sound_stop(void);

/**
 * @brief Zeigt an, ob gerade ein Ton oder eine Melodie ausgegeben wird.
 *
 * @return true, solange die Ausgabe laeuft, sonst false.
 */
bool sound_laeuft(void);

#endif /* SOUND_H_ */
