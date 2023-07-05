#include <stdint.h>
#include <stdio.h>

#include "ad9910/units.h"

extern uint32_t ad_system_clock;

// Вычислить значение FTW для указанной частоты
// Если частота превышает SYSCLK/2, то на выходе получится противоположная частота, т.е. f_настоящая = SYSCLK - f_указанная
uint32_t ad_calc_ftw(double freq_hz) {
	double ratio = freq_hz / (double)ad_system_clock;
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);
	
	return ftw;
}

double ad_backconvert_ftw(uint32_t ftw) {
	return (double)ftw * ((double)ad_system_clock / 4294967296.0);
}

double ad_backconvert_pow(uint16_t pow) {
	return 360.0 * ((double)pow / 65536.0);
}

double ad_backconvert_asf(uint16_t asf) {
	return (double)asf / (double)0x3FFF;
}

double ad_backconvert_step_time(uint16_t step_time) {
	double tstep = 1.0 / (ad_system_clock / 4.0);
	return step_time * tstep;
}
