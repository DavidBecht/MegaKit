/*-------------------------------------------------------------------------*\
| Datei:        twi_soft.c
| Version:      1.1
| Projekt:      I2C Softwareloesung Bibliothek
| Beschreibung: I2C in Software fuer die Anzeige an der MEGACARD
| Schaltung:    siehe twi_soft_port_def.h
| Autor:        D.I. Leopold Moosbrugger
| Erstellung:   02.03.2021 21:05:43
|
| Aenderung:    Buserholung beim Start, wiederholte Start-Bedingung richtig,
|               SCL-High-Zeit auf das zulaessige Minimum, Makros aufgeraeumt
\*-------------------------------------------------------------------------*/
#include <avr/io.h>
#include <util/delay.h>

// TWI Pins festlegen (Port und Pin)
#include "twi_soft_port_def.h"
#include "twi_soft.h"

/* --- Zeiten ---------------------------------------------------------------
 *
 * Die Taktdauer besteht nicht nur aus diesen Pausen. Der Befehlsaufwand der
 * Bitschleife zaehlt mit, und zwar deutlich: Schieben, Bit pruefen, Leitung
 * setzen und die Schleife selbst brauchen zusammen rund 16 Takte. Wer hier
 * etwas aendert, zaehlt die Zyklen am besten im erzeugten Assembler nach
 * (avr-gcc -Os -S) und nicht im Kopf.
 *
 * Am Assembler abgezaehlt, F_CPU = 12 MHz:
 *
 *   Fast Mode     30 bis 31 Zyklen je Bit, je nach Bitwert,
 *                 also 2,5 bis 2,6 us oder 385 bis 400 kHz.
 *                 SCL 0,67 us oben (Minimum 0,6 us),
 *                     1,58 us unten (Minimum 1,3 us).
 *   Standard Mode rund 122 Zyklen je Bit = 10,2 us = 98 kHz.
 */
#ifdef TWI_S_FAST
   // I2C Fast Mode, bis 400 kHz
   #define TWI_S_DELAY1()    _delay_us(0.7)   // Daten stehen vor der SCL-Flanke
   #define TWI_S_DELAY2()    _delay_us(0.6)   // SCL-High-Zeit
   #define TWI_S_DELAY_BUS() _delay_us(1.5)   // Start, Stop und Buspause
#else
   // I2C Standard Mode, bis 100 kHz
   #define TWI_S_DELAY1()    _delay_us(3.0)
   #define TWI_S_DELAY2()    _delay_us(6.0)
   #define TWI_S_DELAY_BUS() _delay_us(6.0)
#endif

#define P_SCL_LIN (1<<PSCL_TWI_S)
#define P_SDA_LIN (1<<PSDA_TWI_S)

/* --- Die beiden Busleitungen ----------------------------------------------
 *
 * SDA arbeitet als Open Drain, wie es I2C vorschreibt: Der Ausgang zieht nur
 * nach Masse, den High-Pegel stellt der Pull-up auf der Anzeigenplatine her.
 * Umgeschaltet wird deshalb die Richtung (DDR), das PORT-Bit bleibt dauerhaft
 * auf 0:
 *
 *    low  = DDR auf Ausgang, PORT 0
 *    high = DDR auf Eingang, kein Pull-up
 *
 * SCL wird dagegen aktiv getrieben. Das ist schneller, weil die steigende
 * Flanke nicht vom Pull-up abhaengt, schliesst aber zweierlei aus:
 * Clock-Stretching, denn ein Baustein, der SCL festhaelt, laege gegen den
 * Treiber, und einen zweiten Master am Bus. Fuer die Anzeige ist beides ohne
 * Belang, das SSD1306 streckt den Takt nicht. Wer einen Baustein anschliesst,
 * der streckt, stellt SCL hier auf dieselbe Open-Drain-Form um wie SDA und
 * wartet in der Bitschleife nach SCL_HIGH, bis die Leitung wirklich oben ist.
 *
 * Alle drei Register liegen beim ATmega16 unter 0x20. Der Compiler erzeugt
 * daraus sbi und cbi, also einzelne, nicht unterbrechbare Befehle. Eine
 * Interruptroutine, die einen anderen Pin desselben Ports schaltet, etwa der
 * Ton auf PB3, kann den Bus deshalb nicht zerstoeren.
 */
#define SDA_RUHE    (PRT_TWI_S &= ~P_SDA_LIN)   // PORT-Bit auf 0
#define SDA_HIGH    (DDR_TWI_S &= ~P_SDA_LIN)
#define SDA_LOW     (DDR_TWI_S |=  P_SDA_LIN)
#define SDA_IN      (PIN_TWI_S &   P_SDA_LIN)

#define SCL_AUSGANG (DDR_TWI_S |=  P_SCL_LIN)   // Richtung einmalig setzen
#define SCL_HIGH    (PRT_TWI_S |=  P_SCL_LIN)
#define SCL_LOW     (PRT_TWI_S &= ~P_SCL_LIN)


// Init TWI
void twi_s_init(void)
{
/* SCL XXX/''''
   SDA XX/'''''
*/

   SDA_HIGH;
   SDA_RUHE;
   SCL_HIGH;
   SCL_AUSGANG;
   TWI_S_DELAY_BUS();

   /* Buserholung. Wird der Controller mitten in einer Uebertragung
      zurueckgesetzt, etwa durch den Reset-Taster oder den Programmer, wartet
      der Baustein auf die restlichen Takte und haelt SDA fest. Ohne Zutun
      bliebe die Anzeige dauerhaft dunkel, ohne jeden Hinweis auf die Ursache.
      Bis zu neun Takte bringen das angefangene Byte zu Ende, danach gibt der
      Baustein die Leitung frei. Bei freiem Bus geschieht hier nichts. */
   if (SDA_IN == 0)
   {
      for (uint8_t i = 0; i < 9 && SDA_IN == 0; i++)
      {
         SCL_LOW;  TWI_S_DELAY_BUS();
         SCL_HIGH; TWI_S_DELAY_BUS();
      }
      // Start- und Stop-Bedingung setzen den Baustein in den Ruhezustand
      twi_s_stop();
   }
}

// Start TWI
uint8_t twi_s_start(uint8_t address)
{
/* SCL ''''\...
   SDA '\......
*/
   SDA_RUHE;
   SCL_AUSGANG;
   SDA_LOW; TWI_S_DELAY_BUS();
   SCL_LOW; TWI_S_DELAY_BUS();

   return twi_s_write(address);
}

// Wiederholte Start-Bedingung
uint8_t twi_s_rep_start(uint8_t address)
{
/* SCL ..../'''\...
   SDA ../''''\....
*/
   /* Nach einem uebertragenen Byte liegt SCL unten und SDA ist freigegeben.
      twi_s_start() setzt aber beide Leitungen oben voraus, denn eine
      Start-Bedingung ist eine fallende SDA-Flanke bei hohem SCL. Beide
      Leitungen muessen deshalb zuerst wieder hoch. */
   SDA_HIGH; TWI_S_DELAY_BUS();
   SCL_HIGH; TWI_S_DELAY_BUS();

   return twi_s_start(address);
}

// Stop TWI
void twi_s_stop(void)
{
/* SCL ..../''''''
   SDA '\....../''
*/
   SDA_RUHE;
   SCL_AUSGANG;
   SDA_LOW;  TWI_S_DELAY_BUS();
   SCL_HIGH; TWI_S_DELAY_BUS();
   SDA_HIGH; TWI_S_DELAY_BUS();   // zugleich die Buspause vor dem naechsten Start
}

// TWI schreiben
uint8_t twi_s_write(uint8_t data)
{
   uint8_t i;
   uint8_t hlp;

   SDA_RUHE;
   SCL_AUSGANG;

/* SCL .../'0'\../'1'\../'2'\../'3'\../'4'\../'5'\../'6'\../'7'\../'A\
   SDA 'X======X======X======X======X======X======X======X======/'''''
*/
   for (i=0;i<8;i++)
   {  // 8 Bit ausgeben
      if ((data & (1<<7)) != 0)
         SDA_HIGH; // wenn Bit gesetzt
      else
         SDA_LOW;  // wenn Bit nicht gesetzt

      data <<= 1;  // Bit schieben, 7 bis 0
      TWI_S_DELAY1();
      SCL_HIGH;
      TWI_S_DELAY2();
      SCL_LOW;
   }

   SDA_HIGH;
   TWI_S_DELAY1();
   SCL_HIGH;
   TWI_S_DELAY1();

   if (SDA_IN == 0) // Testen SDA auf Acknowledge low
      hlp = ACK;    // Flag Ack setzen
   else
      hlp = NACK;   // sonst Flag Ack loeschen

   SCL_LOW;

   return hlp;
}

// TWI Byte lesen

uint8_t twi_s_read(uint8_t ack)
{
   uint8_t data = 0;
   uint8_t i;

   SDA_RUHE;
   SCL_AUSGANG;

/* SCL ./'0\../'1\../'2\../'3\../'4\../'5\../'6\../'7\../''\.
   SDA ===D=====D=====D=====D=====D=====D=====D=====D=A=====/
*/
   for (i=0;i<8;i++)
   {  // 8 Bit ausgeben
      data <<= 1;			// Bit schieben, 7 bis 0
      TWI_S_DELAY1();
      SCL_HIGH;
      TWI_S_DELAY1();
      if (SDA_IN != 0) // Testen SDA auf high
         data |= 1;    // wenn high, Bit 0 setzen
      SCL_LOW;
   }

   if (ack)
      SDA_LOW;  // Acknowledge
   else
      SDA_HIGH;  // No Acknowledge

   TWI_S_DELAY1();
   SCL_HIGH;
   TWI_S_DELAY2();
   SCL_LOW;
   SDA_HIGH;

   return data;  // Daten zurueckgeben
}
