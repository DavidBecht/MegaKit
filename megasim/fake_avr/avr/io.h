#ifndef FAKE_AVR_IO_H_
#define FAKE_AVR_IO_H_

#include <stdint.h>
#include <stdbool.h>
#include "sim_api.h"   /* register definitions — found via -I fake_src/ */

/* ---- Port A (Buttons S0-S3 on PA0-PA3, active-low) ---- */
#define PINA  PINA_reg
#define DDRA  DDRA_reg
#define PORTA PORTA_reg

/* ---- Port B (TWI: SCL=PB0, SDA=PB1) ---- */
#define PINB  PINB_reg
#define DDRB  DDRB_reg
#define PORTB PORTB_reg

/* ---- Port C (LEDs) ---- */
#define PINC  PINC_reg
#define DDRC  DDRC_reg
#define PORTC PORTC_reg

/* ---- Port D ---- */
#define PIND  PIND_reg
#define DDRD  DDRD_reg
#define PORTD PORTD_reg

/* ---- Timer 1 (Sprite FPS) ---- */
#define TCCR0 TCCR0_reg
#define OCR0  OCR0_reg
#define TCNT0 TCNT0_reg

#define TCCR1A TCCR1A_reg
#define TCCR1B TCCR1B_reg
#define OCR1A  OCR1A_reg
#define OCR1B  OCR1B_reg
#define TCNT1  TCNT1_reg

/* ---- Timer 2 (Game timing) ---- */
#define TCCR2 TCCR2_reg
#define OCR2  OCR2_reg

/* ---- Interrupt mask / flag ---- */
#define TIMSK TIMSK_reg
#define TIFR  TIFR_reg

/* ---- ADC ----
   Ueber Zugriffsfunktionen, damit eine mit ADSC gestartete Wandlung beim
   naechsten Lesen fertig ist. ADC ist wie beim AVR ein zweiter Name fuer ADCW. */
#define ADMUX  ADMUX_reg
#define ADCSRA (*sim_adcsra())
#define ADCW   (*sim_adcw())
#define ADC    ADCW
#define ADCH   (*sim_adch())
#define ADCL   (*sim_adcl())

/* ---- Bit-position names (0-7, same across all ports) ---- */
#define PA0 0
#define PA1 1
#define PA2 2
#define PA3 3
#define PA4 4
#define PA5 5
#define PA6 6
#define PA7 7

#define PB0 0
#define PB1 1
#define PB2 2
#define PB3 3
#define PB4 4
#define PB5 5
#define PB6 6
#define PB7 7

#define PC0 0
#define PC1 1
#define PC2 2
#define PC3 3
#define PC4 4
#define PC5 5
#define PC6 6
#define PC7 7

#define PD0 0
#define PD1 1
#define PD2 2
#define PD3 3
#define PD4 4
#define PD5 5
#define PD6 6
#define PD7 7

/* ---- TCCR1B bits ---- */
#define CS10  0
#define CS11  1
#define CS12  2
#define WGM12 3
#define WGM13 4
#define ICES1 6
#define ICNC1 7

/* ---- TCCR1A bits ---- */
#define WGM10  0
#define WGM11  1
#define COM1B0 4
#define COM1B1 5
#define COM1A0 6
#define COM1A1 7

/* ---- TCCR0 bits ---- */
#define CS00   0
#define CS01   1
#define CS02   2
#define WGM01  3
#define COM00  4
#define COM01  5
#define WGM00  6
#define FOC0   7

/* ---- TCCR2 bits ---- */
#define CS20  0
#define CS21  1
#define CS22  2
#define WGM21 3
#define COM20 4
#define COM21 5
#define WGM20 6
#define FOC2  7

/* ---- TIMSK bits ---- */
#define TOIE0  0
#define OCIE0  1
#define TOIE1  2
#define OCIE1B 3
#define OCIE1A 4
#define TICIE1 5
#define TOIE2  6
#define OCIE2  7

/* TIFR: Merker der Timer. Im Simulator setzt sie niemand, Warteschleifen
   darauf muessen deshalb eine Obergrenze haben. */
#define TOV0   0
#define OCF0   1

/* ---- ADMUX bits ---- */
#define MUX0  0
#define MUX1  1
#define MUX2  2
#define MUX3  3
#define MUX4  4
#define ADLAR 5
#define REFS0 6
#define REFS1 7

/* ---- ADCSRA bits ---- */
#define ADPS0 0
#define ADPS1 1
#define ADPS2 2
#define ADIE  3
#define ADIF  4
#define ADATE 5
#define ADSC  6
#define ADEN  7

/* ---- Utility ---- */
#define _BV(bit) (1u << (bit))

#ifndef F_CPU
#define F_CPU 12000000UL
#endif

#endif /* FAKE_AVR_IO_H_ */
