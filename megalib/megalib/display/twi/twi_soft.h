/*-------------------------------------------------------------------------*\
| Datei:        twi_soft.h
| Version:      1.2
| Projekt:      I2C Softwareloesung fuer die MEGACARD
| Beschreibung: I2C in Software fuer die Anzeige an der MEGACARD.
|               Der ATmega16 hat zwar eine Hardware-Schnittstelle, die Pins
|               der Anzeige liegen aber nicht darauf.
| Schaltung:    siehe twi_soft_port_def.h
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   02.03.2021
|
| Aenderung:    twi_s_rep_start als eigene Funktion, Buserholung beim Start
\*-------------------------------------------------------------------------*/

#ifndef TWI_SOFT_H_
#define TWI_SOFT_H_

#include <stdint.h>

#define  ACK 1   ///< Nach dem Lesen: weitere Bytes folgen
#define NACK 0   ///< Nach dem Lesen: das war das letzte Byte

#define I2C_READ    1   ///< Richtungsbit: vom Baustein lesen
#define I2C_WRITE   0   ///< Richtungsbit: in den Baustein schreiben

/**
 * @brief Schaltet die beiden Busleitungen auf ihren Ruhezustand.
 *
 * Muss einmal vor der ersten Uebertragung aufgerufen werden. Welche Pins
 * verwendet werden und ob mit 100 oder 400 kHz gearbeitet wird, steht in
 * twi_soft_port_def.h.
 *
 * Haelt ein Baustein SDA noch fest, weil der Controller mitten in einer
 * Uebertragung zurueckgesetzt wurde, gibt der Aufruf bis zu neun Takte aus
 * und beendet die haengende Uebertragung. Ohne diese Erholung bliebe die
 * Anzeige nach einem solchen Reset dauerhaft dunkel.
 *
 * @note SCL wird aktiv getrieben, nicht als Open Drain. Clock-Stretching
 *       ist damit ausgeschlossen, ebenso ein zweiter Master am Bus. Fuer das
 *       SSD1306 ist beides ohne Belang; die Begruendung steht in twi_soft.c.
 */
void twi_s_init(void);

/**
 * @brief Legt eine Start-Bedingung auf den Bus und sendet die Adresse.
 *
 * @param address Adresse des Bausteins, das unterste Bit ist die Richtung
 *                (I2C_READ oder I2C_WRITE). Die Anzeige hat 0x78, das ist
 *                die 8-Bit-Form der 7-Bit-Adresse 0x3C aus dem Datenblatt:
 *                0x3C << 1 mit I2C_WRITE im untersten Bit.
 * @return ACK, wenn sich ein Baustein gemeldet hat, sonst NACK.
 *
 * @warning Setzt voraus, dass der Bus frei ist, beide Leitungen also oben
 *          liegen. Innerhalb einer laufenden Uebertragung ist stattdessen
 *          twi_s_rep_start() zu verwenden.
 */
uint8_t twi_s_start(uint8_t address);

/**
 * @brief Legt eine wiederholte Start-Bedingung auf den Bus.
 *
 * Fuer den Richtungswechsel innerhalb einer Uebertragung, ohne den Bus
 * zwischendurch freizugeben: erst schreiben, welches Register gemeint ist,
 * dann ohne Stop-Bedingung lesen.
 *
 * Nach einem uebertragenen Byte liegt SCL unten. Diese Funktion bringt
 * zuerst beide Leitungen wieder hoch, denn eine Start-Bedingung ist eine
 * fallende SDA-Flanke bei hohem SCL.
 *
 * @param address Wie bei twi_s_start().
 * @return ACK, wenn sich ein Baustein gemeldet hat, sonst NACK.
 */
uint8_t twi_s_rep_start(uint8_t address);

/**
 * @brief Legt eine Stop-Bedingung auf den Bus und gibt ihn frei.
 */
void twi_s_stop(void);

/**
 * @brief Uebertraegt ein Byte an den Baustein.
 *
 * @param data Zu sendendes Byte.
 * @return ACK, wenn der Baustein bestaetigt hat, sonst NACK.
 */
uint8_t twi_s_write(uint8_t data);

/**
 * @brief Liest ein Byte vom Baustein.
 *
 * @param ack ACK, wenn weitere Bytes folgen sollen, NACK beim letzten Byte.
 * @return Das gelesene Byte.
 */
uint8_t twi_s_read(uint8_t ack);

#endif /* TWI_SOFT_H_ */
