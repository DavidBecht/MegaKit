#ifndef FAKE_UTIL_DELAY_H_
#define FAKE_UTIL_DELAY_H_

#include "sim_api.h"

#define _delay_ms(x) sim_delay_ms((uint32_t)(x))
#define _delay_us(x) sim_delay_us((uint32_t)(x))

#endif /* FAKE_UTIL_DELAY_H_ */
