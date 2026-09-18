/*-------------------------------------------------------------------------*\
| Datei:        twi_soft.h
| Version:      1.1
| Projekt:      I2C Softwareloesung fuer die MEGACARD
| Beschreibung: I2C in Software fuer die Anzeige an der MEGACARD.
|               Der ATmega16 hat zwar eine Hardware-Schnittstelle, die Pins
|               der Anzeige liegen aber nicht darauf.
| Schaltung:    siehe twi_soft_port_def.h
| noch offen:   Clock-Stretching implementieren!
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   02.03.2021
|
| Aenderung:    Doku vereinheitlicht
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
 */
void twi_s_init(void);

/**
 * @brief Legt eine Start-Bedingung auf den Bus und sendet die Adresse.
 *
 * Wird auch fuer eine wiederholte Start-Bedingung verwendet, dafuer gibt es
 * den Zweitnamen twi_s_rep_start.
 *
 * @param address Adresse des Bausteins, das unterste Bit ist die Richtung
 *                (I2C_READ oder I2C_WRITE). Die Anzeige hat 0x78.
 * @return ACK, wenn sich ein Baustein gemeldet hat, sonst NACK.
 */
uint8_t twi_s_start(uint8_t address);
#define twi_s_rep_start twi_s_start

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
