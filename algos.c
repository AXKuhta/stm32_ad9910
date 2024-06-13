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

#define ABS(X) (X < 0 ? -X : X)

// Перебор различных шаговых интервалов DRG для поиска интервала с наименьшей ошибкой итоговой ширины свипа
static uint16_t search_sweep_step_intervals(uint32_t time_ns, uint32_t delta_hz) {
	double fstep = (ad_system_clock + 0.0) / (1ull << 32);
	uint32_t clocks = time_ns / 4;
	uint32_t overhead = 1;

	if (time_ns % 4) {
		printf("Not a factor of 4, unable to represent\n");
		return 0;
	}

	uint32_t min_overhead = 1;
	double min_error = ad_calc_ftw(delta_hz / clocks) * fstep * clocks - delta_hz;

	while (1) {
		if (clocks % overhead) {
			// Нет возможности точно подогнать длительность
			// Шаг пропускается
		} else {
			uint32_t steps = clocks/overhead;
			double req_fstep = (delta_hz + 0.0) / steps;
			uint32_t step_ftw = ad_calc_ftw(req_fstep);

			double f_error = step_ftw*fstep*steps - delta_hz;

			printf("%lu ns\t%lf Hz error\n", overhead*4, f_error);

			if (ABS(f_error) < ABS(min_error)) {
				min_overhead = overhead;
				min_error = f_error;
			}
		}

		overhead++;

		if (overhead > 4) {
			break;
		}
	}

	printf("Min overhead: %lu\n", min_overhead);
	printf("Min error: %lf\n", min_error);

	return min_overhead;
}

// Вычислить параметры для свипа с частоты f1 до f2 за указанное время
// Улучшенная версия
sweep_t calculate_sweep_v2(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns) {
	uint32_t lower_ftw = 0;
	uint32_t upper_ftw = 0;
	uint32_t delta_hz = 0;

	// Если конечная частота выше начальной частоты, то всё нормально
	// Если же конечная частота НИЖЕ начальной частоты, то нужно использовать "зеркальные" значения частоты
	// В даташите это называется "aliased image"
	if (f2_hz > f1_hz) {
		lower_ftw = ad_calc_ftw(f1_hz);
		upper_ftw = ad_calc_ftw(f2_hz);
		delta_hz = f2_hz - f1_hz;
	} else {
		lower_ftw = ad_calc_ftw(ad_system_clock - f1_hz);
		upper_ftw = ad_calc_ftw(ad_system_clock - f2_hz);
		delta_hz = f1_hz - f2_hz;
	}

	printf("Delta: %ld Hz\n", delta_hz);

	uint16_t interval = search_sweep_step_intervals(time_ns, delta_hz);
	uint32_t steps = (time_ns / 4) / interval;

	return (sweep_t){
		.lower_ftw = lower_ftw,
		.upper_ftw = upper_ftw,
		.fstep_ftw = ad_calc_ftw((delta_hz + 0.0) / steps),
		.tstep = interval
	};
}

// Представить некоторое время в наносекундах используя интервал шага * некоторое количество шагов оперативной памяти AD9910
// TODO: учитывать ad_system_clock, сейчас предполагается 1 ГГц
uint16_t fit_time(uint32_t time_ns) {
	uint32_t clocks = time_ns / 4;
	uint32_t overhead = 1;

	if (time_ns % 4) {
		printf("Not a factor of 4, unable to fit\n");
		return 0;
	}

	while(clocks/overhead > 0xFFFF || time_ns % (4*overhead) > 0) {
		overhead++;

		if (overhead > 1023) {
			printf("Unable to get an exact fit\n");
			return 0;
		}
	}

	printf("Overhead: %lu\n", overhead);
	printf("Step: %lu\n", clocks/overhead);

	return clocks/overhead;
}

// См. Auxiliary DAC в AD9910 datasheet
static double fsc_i(uint8_t x) {
	return (86.4 / 10000) * (1 + (x/96.0));
}

// Подобрать наилучшую комбинацию значений ASF и FSC для представления определённого уровня V (rms)
// Предполагается нагрузка 50 ом на выходе платы синтезатора
//
// Значение ASF отвечает за цифровое масштабирование синусоиды перед ЦАПом
// Маленькие значения ASF хуже - с ними теряется часть разрешения ЦАПа
//
// Здесь реализован поиск максимального подходящего значения ASF
//
int best_asf_fsc(double voltage_vrms, uint16_t* asfp, uint8_t* fscp) {
	for (int fsc = 0; fsc < 256; fsc++) {
		double cost =  fsc_i(fsc) * (50.0/3.0 / 1.41421356237309504 / 16383);
		double asf = voltage_vrms / cost;
		
		if (asf <= 16383) {
			*asfp = (uint16_t)(asf + 0.5);
			*fscp = fsc;

			return 1;
		}
	}

	return 0;
}
