/*
 * fake_random.c — Replaces random.c for the simulator.
 * Seeds with time() instead of ADC noise; interface identical to random.h.
 */
#include <stdlib.h>
#include <time.h>
#include <stdint.h>

void random_init(void)
{
    srand((unsigned int)time(NULL));
}

uint8_t random_uint8_range(uint8_t start_inclusive, uint8_t stop_exclusive)
{
    uint8_t low, high;
    if (start_inclusive < stop_exclusive) {
        low  = start_inclusive; high = stop_exclusive;
    } else if (start_inclusive > stop_exclusive) {
        low  = stop_exclusive;  high = start_inclusive;
    } else {
        return start_inclusive;
    }
    return (uint8_t)(rand() % (high - low)) + low;
}

uint8_t random_uint8(uint8_t stop_exclusive)
{
    return random_uint8_range(0, stop_exclusive);
}

float random_float(void)
{
    return (float)(rand() % 255) / 254.0f;
}
