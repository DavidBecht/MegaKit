#ifndef FAKE_UTIL_DELAY_H_
#define FAKE_UTIL_DELAY_H_

#include "sim_api.h"

/* Auf dem AVR muss die Wartezeit beim Uebersetzen feststehen: Mit einer
   Variablen bricht avr-gcc mit "__builtin_avr_delay_cycles expects a compile
   time integer constant" ab. Der Simulator prueft dasselbe, damit ein
   Programm hier nicht laeuft, das auf der Hardware gar nicht baut. */
#define _SIM_DELAY_KONSTANT(x) \
	_Static_assert(__builtin_constant_p(x), \
	    "_delay_ms/_delay_us brauchen einen konstanten Wert, wie auf dem AVR. " \
	    "Fuer variable Zeiten eine Schleife um _delay_ms(1) oder einen Timer verwenden.")

#define _delay_ms(x) do { _SIM_DELAY_KONSTANT(x); sim_delay_ms((uint32_t)(x)); } while (0)
#define _delay_us(x) do { _SIM_DELAY_KONSTANT(x); sim_delay_us((uint32_t)(x)); } while (0)

#endif /* FAKE_UTIL_DELAY_H_ */
