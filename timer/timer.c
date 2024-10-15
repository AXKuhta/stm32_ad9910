#include <stdint.h>

#include "stm32f7xx_hal.h"

// Assumes timers are clocked directly from SYSCLK
// See enable_fast_timers() in init.c
double ns_to_machine_units_factor() {
	return (double)HAL_RCC_GetSysClockFreq() / 1000.0 / 1000.0 / 1000.0;
}

// Convert nanoseconds to timer machine units
uint32_t timer_mu(double time_ns) {
	return time_ns * ns_to_machine_units_factor() + 0.5;
}

// Convert timer machine units to to nanoseconds
double timer_ns(uint32_t time_mu) {
	return (double)time_mu / ns_to_machine_units_factor();
}

// Max time representable by a 16 bit timer
uint32_t max_ns_16bit_timer() {
	return timer_ns( (1 << 16) - 1 );
}
