#ifndef FAKE_UTIL_ATOMIC_H_
#define FAKE_UTIL_ATOMIC_H_

/* Nachbildung von <util/atomic.h>.

   Auf dem AVR sperrt ATOMIC_BLOCK die Interrupts, damit ein Zugriff auf
   mehrbytige Variablen nicht von einer ISR unterbrochen wird. Im Simulator
   laufen ISRs in eigenen Faeden, und Zugriffe bis 64 Bit sind auf dem PC
   ohnehin unteilbar. Der Block wird deshalb nur einmal durchlaufen. Die
   Schreibweise im Programm bleibt dieselbe wie auf der Hardware. */

#define ATOMIC_RESTORESTATE     0
#define ATOMIC_FORCEON          0
#define NONATOMIC_RESTORESTATE  0
#define NONATOMIC_FORCEOFF      0

#define ATOMIC_BLOCK(art) \
	for (int _sim_atomar_einmal = 1; _sim_atomar_einmal; _sim_atomar_einmal = 0)
#define NONATOMIC_BLOCK(art) \
	for (int _sim_atomar_einmal = 1; _sim_atomar_einmal; _sim_atomar_einmal = 0)

#endif /* FAKE_UTIL_ATOMIC_H_ */
