#ifndef SIM_API_H_
#define SIM_API_H_

#include <stdint.h>
#include <stdbool.h>

/* Shared OLED framebuffer: 8 pages x 128 columns.
   Bit 0 = top pixel of page, bit 7 = bottom pixel. */
extern volatile uint8_t sim_framebuffer[8][128];

/* Hardware register state */
extern volatile uint8_t  PINA_reg, DDRA_reg, PORTA_reg;
extern volatile uint8_t  PINB_reg, DDRB_reg, PORTB_reg;
extern volatile uint8_t  PINC_reg, DDRC_reg, PORTC_reg;
extern volatile uint8_t  PIND_reg, DDRD_reg, PORTD_reg;
extern volatile uint8_t  TCCR0_reg,  OCR0_reg,  TCNT0_reg;
extern volatile uint8_t  TCCR1A_reg, TCCR1B_reg;
extern volatile uint16_t OCR1A_reg;
extern volatile uint8_t  TCCR2_reg,  OCR2_reg;
extern volatile uint8_t  TIMSK_reg,  TIFR_reg;
extern volatile uint8_t  ADMUX_reg,  ADCSRA_reg;
extern volatile uint16_t ADCW_reg;
extern volatile uint8_t  ADCH_reg,   ADCL_reg;

/* ISR registry */
typedef void (*sim_isr_func_t)(void);
void sim_register_isr(const char *name, sim_isr_func_t f);
void sim_call_isr    (const char *name);

/* Delay stubs */
void sim_delay_ms(uint32_t ms);
void sim_delay_us(uint32_t us);

/* Nachgebildetes EEPROM, 512 Byte wie beim ATmega16 (E2END = 0x1FF).
   Ein frischer Baustein ist mit 0xFF gefuellt. sim.py sichert den Inhalt
   in eine Datei, damit er einen Neustart und einen Reset ueberlebt. */
#define SIM_EEPROM_SIZE 512
extern volatile uint8_t sim_eeprom[SIM_EEPROM_SIZE];

/* Soft-Reset: Framebuffer + Register zuruecksetzen, ISR-Tabelle bleibt */
void sim_soft_reset(void);


#endif /* SIM_API_H_ */
