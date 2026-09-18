#ifndef FAKE_AVR_INTERRUPT_H_
#define FAKE_AVR_INTERRUPT_H_

#include "sim_api.h"

/* No-op interrupt control */
#define sei() ((void)0)
#define cli() ((void)0)

/* ISR(name) { ... }
   Defines the ISR body as a static function and registers it via a
   constructor so sim_call_isr("name") can invoke it later. */
#define ISR(name)                                                       \
    static void _sim_isr_impl_##name(void);                             \
    __attribute__((constructor))                                        \
    static void _sim_isr_reg_##name(void) {                             \
        sim_register_isr(#name, _sim_isr_impl_##name);                  \
    }                                                                   \
    static void _sim_isr_impl_##name(void)

#endif /* FAKE_AVR_INTERRUPT_H_ */
