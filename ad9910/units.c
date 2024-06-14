#include <stdint.h>
#include <stdio.h>

#include "ad9910/units.h"

extern uint32_t ad_system_clock;

// Вычислить значение FTW для указанной частоты
// Если частота превышает SYSCLK/2, то на выходе получится противоположная частота, т.е. f_настоящая = SYSCLK - f_указанная
uint32_t ad_calc_ftw(double freq_hz) {
	return freq_hz/ad_system_clock * (1ull << 32) + 0.5;
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

// Ток ЦАПа
// См. Auxiliary DAC в AD9910 datasheet
double ad_fsc_i(uint8_t fsc) {
	return (86.4 / 10000) * (1 + (fsc/96.0));
}

// Предполагает нагрузку 50 ом на выходе платы синтезатора
double ad_voltage_vrms_from_asf_fsc(uint16_t asf, uint8_t fsc) {
	return asf * ad_fsc_i(fsc) * (50.0/3.0 / 1.41421356237309504 / 16383);
}
