#ifndef FAKE_AVR_EEPROM_H_
#define FAKE_AVR_EEPROM_H_

#include <stdint.h>
#include "sim_api.h"

/* Auf dem AVR legt EEMEM eine Variable in den EEPROM-Adressraum.
   Hier ist es wirkungslos: der Simulator arbeitet mit festen Adressen,
   also mit Zeigern, deren Zahlenwert direkt der EEPROM-Adresse entspricht. */
#define EEMEM

uint8_t  eeprom_read_byte   (const uint8_t  *addr);
uint16_t eeprom_read_word   (const uint16_t *addr);
void     eeprom_write_byte  (uint8_t  *addr, uint8_t  value);
void     eeprom_write_word  (uint16_t *addr, uint16_t value);
void     eeprom_update_byte (uint8_t  *addr, uint8_t  value);
void     eeprom_update_word (uint16_t *addr, uint16_t value);

#include <stddef.h>
void     eeprom_read_block  (void *ziel, const void *addr, size_t anzahl);
void     eeprom_write_block (const void *quelle, void *addr, size_t anzahl);
void     eeprom_update_block(const void *quelle, void *addr, size_t anzahl);

#endif /* FAKE_AVR_EEPROM_H_ */
