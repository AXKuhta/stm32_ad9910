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

// Вычислить размер шага, необходимый, чтобы пройти с частоты f1 до f2 за указанное время
// Используется минимальная возможная задержка между шагами
//
// Вычисления имеют следующий вид:
//
// sysclk = 1 GHz
// fstep = sysclk / 2^31
// tstep = 1s / 1s / (sysclk / 4)
//
// f1 = 10 MHz
// f2 = 20 MHz
// f_delta = f2 - f1
// t_delta = 1 ms
//
// coverage = f_delta / (fstep * (t_delta / tstep))
// req_tstep = coverage < 1.0 ? tstep * round(1/coverage) : tstep
// req_fstep = f_delta / (t_delta / req_tstep)
// ftw = round(2^31 * (req_fstep/sysclk))
//
ad_ramp_t ad_calc_ramp(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns) {
	double fstep_hz = ad_system_clock / 4294967296.0;
	double tstep_ns = 1*1000*1000*1000 / (ad_system_clock / 4);
	double f_delta = (double)f2_hz - (double)f1_hz;
	double t_delta = (double)time_ns;

	if (f_delta < 0.0)
		f_delta = -f_delta;

	double coverage = f_delta / (fstep_hz * (t_delta / tstep_ns));
	double req_tstep = coverage < 1.0 ? tstep_ns * (uint32_t)(1.0 / coverage + 0.5) : tstep_ns;
	double req_fstep = f_delta / (t_delta / req_tstep);

	double ratio = req_fstep / (double)ad_system_clock;	
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);

	printf("Req fstep: %lf\n", req_fstep);
	printf("Req tstep: %lf\n", req_tstep);
	
	return (ad_ramp_t){
		.fstep_ftw = ftw,
		.tstep_mul = (uint32_t)(req_tstep / tstep_ns)
	};
}

double ad_backconvert_step_time(uint16_t step_time) {
	double tstep = 1.0 / (ad_system_clock / 4.0);
	return step_time * tstep;
}
