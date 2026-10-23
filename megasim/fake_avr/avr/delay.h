#ifndef FAKE_AVR_DELAY_H_
#define FAKE_AVR_DELAY_H_

/* Nachbildung von <avr/delay.h>.

   In der avr-libc steht hier nichts mehr: Die Datei zieht nur noch
   <util/delay.h> herein und meldet beim Uebersetzen, dass der Inhalt dort
   hingewandert ist. Der Simulator macht es genauso, damit ein Programm mit
   dem alten Include hier so baut wie in Microchip Studio.

   #pragma message und nicht #warning wie im Original: Der Simulator
   uebersetzt mit -w. Das schaltet Warnungen ab, Hinweise aber nicht. */

#pragma message("<avr/delay.h> ist veraltet, bitte <util/delay.h> einbinden.")

#include <util/delay.h>

#endif /* FAKE_AVR_DELAY_H_ */
