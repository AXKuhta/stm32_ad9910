#include <stdint.h>
#include <stdio.h>

#include "ad9910/units.h"
#include "sequencer.h"
#include "algos.h"

extern uint32_t ad_system_clock;

// Вычислить параметры для свипа с частоты f1 до f2 за указанное время
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
sweep_t calculate_sweep(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns) {
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

	uint32_t lower_ftw = 0;
	uint32_t upper_ftw = 0;

	// Если конечная частота выше начальной частоты, то всё нормально
	// Если же конечная частота НИЖЕ начальной частоты, то нужно использовать "зеркальные" значения частоты
	// В даташите это называется "aliased image"
	if (f2_hz > f1_hz) {
		lower_ftw = ad_calc_ftw(f1_hz);
		upper_ftw = ad_calc_ftw(f2_hz);
	} else {
		lower_ftw = ad_calc_ftw(ad_system_clock - f1_hz);
		upper_ftw = ad_calc_ftw(ad_system_clock - f2_hz);
	}

	return (sweep_t){
		.lower_ftw = lower_ftw,
		.upper_ftw = upper_ftw,
		.fstep_ftw = ftw,
		.tstep = (uint32_t)(req_tstep / tstep_ns)
	};
}
