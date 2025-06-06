#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "stm32f7xx_hal.h"

#include "freertos_extras/allocator.h"
#include "isr.h"
#include "performance.h"
#include "ad9910.h"
#include "state.h"
#include "units.h"
#include "timer.h"
#include "timer/emulator.h"
#include "sequencer.h"
#include "algos.h"
#include "json.h"
#include "vec.h"

extern uint32_t ad_system_clock;

#define ABS(x) (x < 0 ? -x : x)

// Подготовка кодов для регистров CCMR
// TODO: вынести в другое место
#define DR_CTL	  0b00000001
#define DRHOLD    0b00000010
#define TXE       0b00000100
#define OSK       0b00001000
#define P_0       0b00010000
#define P_1       0b00100000
#define P_2       0b01000000
#define IO_UPDATE 0b10000000

#define PROFILE0  (0)
#define PROFILE1  (P_0)
#define PROFILE2  (P_1)
#define PROFILE3  (P_1 | P_0)
#define PROFILE4  (P_2)
#define PROFILE5  (P_2 | P_0)
#define PROFILE6  (P_2 | P_1)
#define PROFILE7  (P_2 | P_1 | P_0)

typedef struct logic_t {
	uint32_t hold_ns;
	uint8_t state;
} logic_t;

// Терминатор должен быть именно такой
// Первый элемент - парковочный профиль
// Второй элемент - оттянуть момент NDTR == 0 до конца излучения
// Третий элемент - терминатор
#define TERMINATOR 	{ .hold_ns = 1*1000, .state = 0 }, \
					{ .hold_ns = 1*1000, .state = 0 }, \
					{ 0, 0 }

// Спуск последовательности состояний в машинные коды
// Выделит и вернёт три массива для DMA
//
// Пример использования:
//
// ```
// seq_entry_t pulse = {
//     .logic_level_sequence = lower_logic_sequence((logic_t[]){
//         { .hold_ns = 900*1000, .state = P_0 }, // Излучение
//         TERMINATOR // Парковка + терминатор
//     })
// };
// ```
static logic_level_sequence_t lower_logic_sequence(logic_t states[]) {
	uint32_t timer_mhz = HAL_RCC_GetSysClockFreq() / 1000 / 1000;
	int slots_used = 0;

	// Проход 1: определить необходимый запас слотов времени
	// Триалим количество разбивок
	for (size_t i = 0; states[i].hold_ns; i++) {
		uint32_t timer_mu = states[i].hold_ns * timer_mhz / 1000;

		// Не можем точно представить время?
		// TODO: сообщение об ошибке
		assert(states[i].hold_ns * timer_mhz % 1000 == 0);

		int slots = 1;

		while (timer_mu / slots > 0xFFFF || timer_mu % slots) {
			slots++;

			// TODO: сообщение об ошибке
			assert(slots < 100);
		}

		slots_used += slots;
	}

	// TODO: использовать uint16_t - верхние биты бесполезны
	uint32_t* slave_a_stream = malloc(2 * sizeof(uint32_t) * slots_used);
	uint32_t* slave_b_stream = malloc(2 * sizeof(uint32_t) * slots_used);
	uint16_t* hold_time = malloc(sizeof(uint16_t) * slots_used);

	// см. "Output compare 1 mode"
	// https://www.st.com/resource/en/reference_manual/rm0385-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
	const uint8_t set_hi = 0b00010000;
	const uint8_t set_lo = 0b00100000;

	// Проход 2: делаем машинные коды
	for (size_t i = 0, j = 0; states[i].hold_ns; i++) {
		uint8_t state = states[i].state;

		uint8_t a1 = state & DR_CTL ? set_hi : set_lo;		// PC6
		uint8_t a2 = state & DRHOLD ? set_hi : set_lo;		// PC7
		uint8_t a3 = state & TXE    ? set_hi : set_lo;		// PC8
		uint8_t a4 = state & OSK    ? set_hi : set_lo;		// PC9
		uint8_t b1 = state & P_1    ? set_hi : set_lo;		// PD12
		uint8_t b2 = state & P_0    ? set_hi : set_lo;		// PD13
		uint8_t b3 = state & P_2    ? set_hi : set_lo;		// PD14
		uint8_t b4 = state & IO_UPDATE ? set_hi : set_lo;	// PD15

		// Заново найти количество разбивок
		uint32_t timer_mu = states[i].hold_ns * timer_mhz / 1000;

		int slots = 1;

		while (timer_mu / slots > 0xFFFF || timer_mu % slots) {
			slots++;
		}

		// Размножить
		for (int k = 0; k < slots; k++) {
			slave_a_stream[2*j + 0] = (a2 << 8) + a1;
			slave_a_stream[2*j + 1] = (a4 << 8) + a3;

			slave_b_stream[2*j + 0] = (b2 << 8) + b1;
			slave_b_stream[2*j + 1] = (b4 << 8) + b3;

			hold_time[j] = timer_mu / slots;

			j++;
		}
	}

	return (logic_level_sequence_t) {
		.slave_a_stream = (void*)slave_a_stream,
		.slave_b_stream = (void*)slave_b_stream,
		.hold_time = hold_time,
		.count = slots_used
	};
}

// =============================================================================
// CLI COMMANDS
// =============================================================================

// 10^(-.4 dBm/10)		matches PR100 (158 MHz tone)
// #define OUTPUT_RATIO_CAL 0.9120108394

// 10^(-.2 dBm/10)		should match G4-218 (158 MHz tone)
#define OUTPUT_RATIO_CAL 1.0

void set_level_cmd(const char* str) {
	char unit[4] = {0};
	double voltage;

	int rc = sscanf(str, "%*s %lf %3s", &voltage, unit);

	if (rc != 2) {
		printf("Invalid arguments\n");
		printf("Usage: set_level voltage unit\n");
		printf("Example: set_level 200 mV\n");
		return;
	}

	double voltage_vrms = parse_volts(voltage, unit);

	if (voltage_vrms == 0) {
		return;
	}

	uint16_t asf;
	uint8_t fsc;

	int success = best_asf_fsc(voltage_vrms / OUTPUT_RATIO_CAL, &asf, &fsc);

	if (!success) {
		printf("Unable; try a voltage below 270 mV\n");
		return;
	}

	if (asf < 2) {
		printf("Unable; try a voltage above 10 uV\n");
		return;
	}

	double backconv_voltage = ad_vrms(asf, fsc) * OUTPUT_RATIO_CAL;

	// Assume 50 ohms load
	double watts = backconv_voltage * backconv_voltage / 50.0;
	double dbm = 10.0*log10(watts / .001);

	char* verif_voltage = volts_unit( backconv_voltage );

	state.asf = asf;
	state.fsc = fsc;

	printf("Set ASF=%u FSC=%u which is %s rms or %.1lf dBm\n", asf, fsc, verif_voltage, dbm);

	free(verif_voltage);
}

void dbg_level_cmd(const char* str) {
	uint16_t asf;
	uint8_t fsc;

	int rc = sscanf(str, "%*s %hu %hhu", &asf, &fsc);

	if (rc != 2) {
		printf("Invalid arguments\n");
		printf("Usage: dbg_level asf fsc\n");
		printf("Example: dbg_level 16383 127\n");
		return;
	}

	double backconv_voltage = ad_vrms(asf, fsc) * OUTPUT_RATIO_CAL;

	// Assume 50 ohms load
	double watts = backconv_voltage * backconv_voltage / 50.0;
	double dbm = 10.0*log10(watts / .001);

	char* verif_voltage = volts_unit( backconv_voltage );

	printf("Set ASF=%u FSC=%u which is %s rms or %.1lf dBm\n", asf, fsc, verif_voltage, dbm);

	state.asf = asf;
	state.fsc = fsc;

	free(verif_voltage);
}

void test_tone_cmd(const char* str) {
	char unit[4] = {0};
	double freq;

	int rc = sscanf(str, "%*s %lf %3s", &freq, unit);

	if (rc != 2) {
		printf("Invalid arguments\n");
		printf("Usage: test_tone freq unit\n");
		printf("Example: test_tone 150 MHz\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, unit);

	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);

	printf("Test tone at %s\n", verif_freq);

	free(verif_freq);

	enter_test_tone_mode(freq_hz);
}

void basic_pulse_cmd(const char* str) {
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f_unit[4] = {0};
	double offset;
	double duration;
	double freq;

	int rc = sscanf(str, "%*s %lf %3s %lf %3s %lf %3s", &offset, o_unit, &duration, d_unit, &freq, f_unit);

	if (rc != 6) {
		printf("Invalid arguments\n");
		printf("Usage: basic_pulse delay unit duration unit freq unit\n");
		printf("Example: basic_pulse 100 us 250 us 150 MHz\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t duration_ns = parse_time(duration, d_unit);

	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);
	char* verif_t1 = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_t2 = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic pulse at %s, offset %s, duration %s\n", verif_freq, verif_t1, verif_t2);

	free(verif_freq);
	free(verif_t1);
	free(verif_t2);

	uint16_t rate = fit_time(duration_ns);

	if (rate == 0)
		return;

	uint16_t element_count = (duration_ns / 4) / rate;

	uint8_t* ram = malloc(4*element_count + 4);

	for (size_t i = 0; i < element_count; i++) {
		ram[4*i + 0] = 0x00;
		ram[4*i + 1] = 0x00;
		ram[4*i + 2] = (state.asf >> 6);
		ram[4*i + 3] = (state.asf << 2) & 0xFF;
	}

	memset(ram + element_count*4, 0, 4);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.ram_profiles[0] = {
			.start = element_count,
			.end = element_count,
			.rate = 0,
			.mode = AD_RAM_PROFILE_MODE_DIRECTSWITCH
		},
		.ram_profiles[1] = {
			.start = 0,
			.end = element_count,
			.rate = rate,
			.mode = AD_RAM_PROFILE_MODE_RAMPUP
		},
		.ram_image = { .buffer = (uint32_t*)ram, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ad_calc_ftw(freq_hz) },
		.logic_level_sequence = lower_logic_sequence(
			offset_ns ? (logic_t[]){
				{ .hold_ns = offset_ns, .state = 0 },
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			} : (logic_t[]){
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			}
		)
	};

	sequencer_reset();
	sequencer_add(pulse);
	sequencer_run();
}

static void sequencer_add_sweep_internal(const char* str, const char* fstr, const char* caller, int run) {
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char fc_unit[4] = {0};
	double offset;
	double duration;
	double fc;
	int a;
	int b;

	int rc = sscanf(str, fstr, &offset, o_unit, &duration, d_unit, &fc, fc_unit, &a, &b);

	if (rc != 8) {
		printf("Invalid arguments\n");
		printf("Usage: %s delay unit duration unit center_freq unit a b\n", caller);
		printf("Sweep width is determined by: (duration / 4 ns) * (1 GHz / 2^32) * (a/b)\n");
		printf("When a is positive, sweep goes from fc - width/2 to fc + width/2\n");
		printf("When a is negative, sweep goes from fc + width/2 to fc - width/2\n");
		printf("Example: %s 100 us 250 us 140 MHz 1 1\n", caller);
		return;
	}

	uint32_t fc_hz = parse_freq(fc, fc_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t duration_ns = parse_time(duration, d_unit);

	if (b < 1) {
		printf("Invalid b; must be greater than 0\n");
		return;
	}

	// FIXME: Assuming 1 GHz ad_system_clock
	double fstep = ad_system_clock / ((1ull << 32) + 0.0);
	uint32_t steps = duration_ns / (4 * b);

	if (duration_ns % (4 * b) != 0) {
		printf("Invalid combination of duration and b; duration must be a factor of b * 4 ns\n");
		return;
	}

	double span_hz = a * fstep * (steps - 1);
	double f1_hz = fc_hz - span_hz/2;
	double f2_hz = f1_hz + span_hz;

	char* verif_f1 = freq_unit(f1_hz);
	char* verif_f2 = freq_unit(f2_hz);
	char* verif_fc = freq_unit(fc_hz);
	char* verif_span = freq_unit( ABS(span_hz) );
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Sweep center frequency %s span %s, offset %s, duration %s\n", verif_fc, verif_span, verif_offset, verif_duration);
	printf("f1: %s, f2: %s\n", verif_f1, verif_f2);

	free(verif_f1);
	free(verif_f2);
	free(verif_fc);
	free(verif_span);
	free(verif_offset);
	free(verif_duration);

	uint16_t rate = fit_time(duration_ns);

	if (rate == 0)
		return;

	uint16_t element_count = (duration_ns / 4) / rate;
	uint32_t ftw = ad_calc_ftw(f1_hz);

	// We are using RAM and DRG at the same time and that causes a problem:
	// Even with matched latency enabled, they have different pipeline delays
	// DRG Frequency arrives at the DDS core faster than RAM Phase + Amplitude
	// Thus we get a signal with an unexpected start phase
	// 
	// Datasheet says 12 clocks of delay but through measurements it seems it is actually 20
	// Without compensation:
	// 200 MHz	0 deg start phase
	// 100 MHz	0 deg start phase
	// 50 MHz	0 deg start phase
	// 25 MHz	180 deg start phase => 20 clocks of delay
	//
	// Knowing it's 20 clocks of delay, we can compensate for it
	//
	uint32_t start_phase = ftw * 20;
	uint16_t start_phase_16bit = start_phase >> 16;
	uint16_t compensation = 0xFFFF - start_phase_16bit;

	if (f1_hz > f2_hz) {
		compensation = start_phase_16bit + 0x7FFF;
	}

	printf("Start POW: %d\n", start_phase_16bit);

	uint8_t* ram = malloc(4*element_count + 4);

	for (size_t i = 0; i < element_count; i++) {
		ram[4*i + 0] = compensation >> 8;
		ram[4*i + 1] = compensation & 0xFF;
		ram[4*i + 2] = (state.asf >> 6);
		ram[4*i + 3] = (state.asf << 2) & 0xFF;
	}

	memset(ram + element_count*4, 0, 4);

	uint32_t lower_ftw = ftw;
	uint32_t upper_ftw = ftw + a * (steps - 1);

	// We can only reset DRG to lower_ftw, therefore lower_ftw must always be lower than upper_ftw
	// Use mirrored FTWs for f1 > f2 sweeps
	if (a < 0) {
		lower_ftw = (1ull << 32) - lower_ftw;
		upper_ftw = (1ull << 32) - upper_ftw;
	}

	seq_entry_t pulse = {
		.sweep = {
			.fstep_ftw = ABS(a),
			.tstep = b,
			.lower_ftw = lower_ftw,
			.upper_ftw = upper_ftw
		},
		.fsc = state.fsc,
		.ram_profiles[0] = {
			.start = element_count,
			.end = element_count,
			.rate = 0,
			.mode = AD_RAM_PROFILE_MODE_DIRECTSWITCH
		},
		.ram_profiles[1] = {
			.start = 0,
			.end = element_count,
			.rate = rate,
			.mode = AD_RAM_PROFILE_MODE_RAMPUP
		},
		.ram_image = { .buffer = (uint32_t*)ram, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.logic_level_sequence = lower_logic_sequence(
			offset_ns ? (logic_t[]){
				{ .hold_ns = offset_ns, .state = 0 },
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			} : (logic_t[]){
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			}
		)
	};

	if (run) {
		sequencer_reset();
		sequencer_add(pulse);
		sequencer_run();
	} else {
		sequencer_add(pulse);
	}
}

void sequencer_add_sweep_cmd(const char* str) {
	sequencer_add_sweep_internal(str, "%*s %*s %lf %3s %lf %3s %lf %3s %d %d", "seq sweep", 0);
}

void basic_sweep_cmd(const char* str) {
	sequencer_add_sweep_internal(str, "%*s %lf %3s %lf %3s %lf %3s %d %d", "basic_sweep", 1);
}

// The compiler can't tell that two different vec_t(uint8_t)* are actually the same type
// Declare this function as void pointer to silence type warning
void* scan_uint8_data(const char* str) {
	vec_t(uint8_t)* vec = init_vec(uint8_t);

	char* values = (char*)str;
	char* endptr;

	while (1) {
		unsigned int v = strtoul(values, &endptr, 0);

		if (endptr == values) {
			break;
		} else {
			vec_push(vec, v);
			values = endptr;
		}
	}

	printf("Loaded %u values\n", vec->size);

	return vec;
}

void xmitdata_fsk_cmd(const char* str) {
	char o_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	char ts_unit[4] = {0};
	double offset;
	double f1;
	double f2;
	double tstep;
	int data_offset;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s %lf %3s %n", &offset, o_unit, &f1, f1_unit, &f2, f2_unit, &tstep, ts_unit, &data_offset);

	if (rc != 8) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata fsk offset unit f1 unit f2 unit tstep unit data data data ...\n");
		printf("Example: basic_xmitdata fsk 0 us 151.10 MHz 151.11 MHz 10 us 1 1 0 1 ...\n");
		return;
	}

	uint32_t f1_hz = parse_freq(f1, f1_unit);
	uint32_t f2_hz = parse_freq(f2, f2_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	vec_t(logic_t)* v2 = init_vec(logic_t);

	if (offset_ns)
		vec_push(v2, (logic_t){ .hold_ns = offset_ns, .state = 0 } );

	for (size_t i = 0; i < vec->size; i++) {
		vec_push(v2, (logic_t){
			.hold_ns = tstep_ns,
			.state = vec->elements[i] ? PROFILE2 : PROFILE3
		});
	}

	vec_push(v2, (logic_t){ .hold_ns = 1*1000, .state = PROFILE0 } );
	vec_push(v2, (logic_t){ 0 });

	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (f1_hz == 0 || f2_hz == 0) {
		return;
	}

	char* verif_f1 = freq_unit(f1_hz);
	char* verif_f2 = freq_unit(f2_hz);
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_tstep = time_unit(tstep_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration_ns = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic FSK at %s + %s, offset %s, %u * %s elements = %s total duration\n", verif_f1, verif_f2, verif_offset, vec->size, verif_tstep, verif_duration_ns);

	free(verif_f1);
	free(verif_f2);
	free(verif_offset);
	free(verif_tstep);
	free(verif_duration_ns);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[2] = { .ftw = ad_calc_ftw(f1_hz), .asf = state.asf },
		.profiles[3] = { .ftw = ad_calc_ftw(f2_hz), .asf = state.asf },
		.logic_level_sequence = lower_logic_sequence(v2->elements)
	};

	sequencer_reset();
	sequencer_add(pulse);
	sequencer_run();

	free_vec(vec);
	free_vec(v2);
}

void xmitdata_psk_cmd(const char* str) {
	char o_unit[4] = {0};
	char f_unit[4] = {0};
	char ts_unit[4] = {0};
	double offset;
	double freq;
	double tstep;
	int data_offset;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s %n", &offset, o_unit, &freq, f_unit, &tstep, ts_unit, &data_offset);

	if (rc != 6) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata psk offset unit freq unit tstep unit data data data ...\n");
		printf("Example: basic_xmitdata psk 0 us 151.10 MHz 10 us 1 1 0 1 ...\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	vec_t(logic_t)* v2 = init_vec(logic_t);

	if (offset_ns)
		vec_push(v2, (logic_t){ .hold_ns = offset_ns, .state = 0 } );

	for (size_t i = 0; i < vec->size; i++) {
		vec_push(v2, (logic_t){
			.hold_ns = tstep_ns,
			.state = vec->elements[i] ? PROFILE2 : PROFILE3
		});
	}

	vec_push(v2, (logic_t){ .hold_ns = 1*1000, .state = 0 } );
	vec_push(v2, (logic_t){ 0 });

	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_tstep = time_unit(tstep_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic PSK at %s, offset %s, %u * %s elements = %s total duration\n", verif_freq, verif_offset, vec->size, verif_tstep, verif_duration);

	free(verif_freq);
	free(verif_offset);
	free(verif_tstep);
	free(verif_duration);

	uint32_t ftw = ad_calc_ftw(freq_hz);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[2] = { .ftw = ftw, .pow = 0x0000, .asf = state.asf },
		.profiles[3] = { .ftw = ftw, .pow = 0x7FFF, .asf = state.asf },
		.logic_level_sequence = lower_logic_sequence(v2->elements)
	};

	sequencer_reset();
	sequencer_add(pulse);
	sequencer_run();

	free_vec(vec);
	free_vec(v2);
}

void xmitdata_zc_psk_cmd(const char* str) {
	char o_unit[4] = {0};
	char f_unit[4] = {0};
	char ts_unit[4] = {0};
	double offset;
	double freq;
	double tstep;
	int data_offset;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s %n", &offset, o_unit, &freq, f_unit, &tstep, ts_unit, &data_offset);

	if (rc != 6) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata zc_psk offset unit freq unit tstep unit data data data ...\n");
		printf("Example: basic_xmitdata zc_psk 0 us 151.10 MHz 10 us 1 1 0 1 ...\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	vec_t(logic_t)* v2 = init_vec(logic_t);

	if (offset_ns)
		vec_push(v2, (logic_t){ .hold_ns = offset_ns, .state = 0 } );

	for (size_t i = 0; i < vec->size; i++) {
		vec_push(v2, (logic_t){
			.hold_ns = tstep_ns,
			.state = vec->elements[i] ? PROFILE2 : PROFILE3
		});
	}

	vec_push(v2, (logic_t){ .hold_ns = 1*1000, .state = 0 } );
	vec_push(v2, (logic_t){ 0 });

	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_tstep = time_unit(tstep_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic PSK at %s, offset %s, %u * %s elements = %s total duration\n", verif_freq, verif_offset, vec->size, verif_tstep, verif_duration);

	free(verif_freq);
	free(verif_offset);
	free(verif_tstep);
	free(verif_duration);

	uint32_t ftw = ad_calc_ftw(freq_hz) & 0xFFFC0000;

	uint8_t bpsk_ram_image[12] = {
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, (state.asf >> 6), (state.asf << 2) & 0xFF,
		0x7F, 0xFF, (state.asf >> 6), (state.asf << 2) & 0xFF
	};

	uint8_t* ram = malloc(12);

	memcpy(ram, bpsk_ram_image, 12);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.ram_profiles[0] = { .start = 0, .end = 0, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.ram_profiles[2] = { .start = 1, .end = 1, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.ram_profiles[3] = { .start = 2, .end = 2, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.logic_level_sequence = lower_logic_sequence(v2->elements),
		.ram_image = { .buffer = (uint32_t*)ram, .size = 3 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ftw }
	};

	sequencer_reset();
	sequencer_add(pulse);
	sequencer_run();

	free_vec(vec);
	free_vec(v2);
}

void xmitdata_ram_psk_cmd(const char* str) {
	char o_unit[4] = {0};
	char f_unit[4] = {0};
	char ts_unit[4] = {0};
	double offset;
	double freq;
	double tstep;
	int data_offset;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s %n", &offset, o_unit, &freq, f_unit, &tstep, ts_unit, &data_offset);

	if (rc != 6) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata ram_psk offset unit freq unit tstep unit data data data ...\n");
		printf("Example: basic_xmitdata ram_psk 0 us 151.10 MHz 10 us 1 1 0 1 ...\n");
		return;
	}

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	size_t element_count = vec->size;

	uint8_t* ram = malloc(4*element_count + 4);

	for (size_t i = 0; i < element_count; i++) {
		if (vec->elements[i] == 0) {
			ram[4*i + 0] = 0x00;
			ram[4*i + 1] = 0x00;
			ram[4*i + 2] = (state.asf >> 6);
			ram[4*i + 3] = (state.asf << 2) & 0xFF;
		} else {
			ram[4*i + 0] = 0x7F;
			ram[4*i + 1] = 0xFF;
			ram[4*i + 2] = (state.asf >> 6);
			ram[4*i + 3] = (state.asf << 2) & 0xFF;
		}
	}

	memset(ram + element_count*4, 0, 4);

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);
	uint32_t duration_ns = tstep_ns * element_count;

	if (freq_hz == 0) {
		return;
	}

	assert(tstep_ns % 4 == 0);

	char* verif_freq = freq_unit(freq_hz);
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_tstep = time_unit(tstep_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic PSK at %s, offset %s, %u * %s elements = %s total duration\n", verif_freq, verif_offset, element_count, verif_tstep, verif_duration);

	free(verif_freq);
	free(verif_offset);
	free(verif_tstep);
	free(verif_duration);

	uint32_t ftw = ad_calc_ftw(freq_hz);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.ram_profiles[0] = {
			.start = element_count,
			.end = element_count,
			.rate = 0,
			.mode = AD_RAM_PROFILE_MODE_DIRECTSWITCH
		},
		.ram_profiles[1] = {
			.start = 0,
			.end = element_count,
			.rate = tstep_ns / 4,
			.mode = AD_RAM_PROFILE_MODE_RAMPUP
		},
		.ram_image = { .buffer = (uint32_t*)ram, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ftw },
		.logic_level_sequence = lower_logic_sequence(
			offset_ns ? (logic_t[]){
				{ .hold_ns = offset_ns, .state = 0 },
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			} : (logic_t[]){
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			}
		)
	};

	sequencer_reset();
	sequencer_add(pulse);
	sequencer_run();

	free_vec(vec);
}

void basic_xmitdata_cmd(const char* str) {
	char cmd[32] = {0};

	int rc = sscanf(str, "%*s %31s", cmd);

	if (rc != 1) {
		printf("Invalid arguments\n");
		printf("Usage: basic_xmitdata mode ...\n");
		return;
	}

	if (strcmp(cmd, "fsk") == 0) return xmitdata_fsk_cmd(str);
	if (strcmp(cmd, "psk") == 0) return xmitdata_psk_cmd(str);
	if (strcmp(cmd, "zc_psk") == 0) return xmitdata_zc_psk_cmd(str);
	if (strcmp(cmd, "ram_psk") == 0) return xmitdata_ram_psk_cmd(str);

	printf("Unknown xmitdata mode: [%s]\n", cmd);
}

void sequencer_add_pulse_cmd(const char* str) {
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f_unit[4] = {0};
	double offset;
	double duration;
	double freq;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s", &offset, o_unit, &duration, d_unit, &freq, f_unit);

	if (rc != 6) {
		printf("Invalid arguments\n");
		printf("Usage: seq pulse delay unit duration unit freq unit\n");
		printf("Example: seq pulse 100 us 250 us 150 MHz\n");
		return;
	}

	uint32_t freq_hz 		= parse_freq(freq, f_unit);
	uint32_t offset_ns 		= parse_time(offset, o_unit);
	uint32_t duration_ns 	= parse_time(duration, d_unit);

	if (freq_hz == 0) {
		return;
	}

	char* verif_freq 		= freq_unit(freq_hz);
	char* verif_offset 		= time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration 	= time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Sequence basic pulse at %s, offset %s, duration %s\n", verif_freq, verif_offset, verif_duration);

	seq_entry_t pulse = {
		.fsc = state.fsc,
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[1] = { .ftw = ad_calc_ftw(freq_hz), .asf = state.asf },
		.logic_level_sequence = lower_logic_sequence(
			offset_ns ? (logic_t[]){
				{ .hold_ns = offset_ns, .state = 0 },
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			} : (logic_t[]){
				{ .hold_ns = duration_ns, .state = PROFILE1 },
				TERMINATOR
			}
		)
	};

	sequencer_add(pulse);

	free(verif_freq);
	free(verif_offset);
	free(verif_duration);
}

static void _json_load_ram_profiles(const char* json, seq_entry_t* pulse) {
	int offset = json_query_location( json, (const char* []){"v2", "ram_profiles", NULL} );

	assert(offset != - 1);

	int count = json_query_count( json+offset, (const char* []){NULL} );

	assert(count > 0);
	assert(count < 8);

	for (int i = 0; i < count; i++) {
		const char idx[2] = {'0' + i, 0};

		pulse->ram_profiles[i] = (ram_profile_t) {
			.start = json_query_i32(json+offset, (const char* []){idx, "start", NULL} ),
			.end = json_query_i32(json+offset, (const char* []){idx, "end", NULL} ),
			.mode = json_query_i32(json+offset, (const char* []){idx, "mode", NULL} ),
			.rate = json_query_i32(json+offset, (const char* []){idx, "rate", NULL} )
		};
	}
}

static void _json_load_tone_profiles(const char* json, seq_entry_t* pulse) {
	int offset = json_query_location( json, (const char* []){"v2", "profiles", NULL} );

	assert(offset != -1);

	int count = json_query_count( json+offset, (const char* []){NULL} );

	assert(count > 0);
	assert(count <= 8);

	for (int i = 0; i < count; i++) {
		const char idx[2] = {'0' + i, 0};

		pulse->profiles[i] = (profile_t) {
			.asf = json_query_i32(json+offset, (const char* []){idx, "asf", NULL} ),
			.ftw = json_query_i32(json+offset, (const char* []){idx, "ftw", NULL} ),
			.pow = json_query_i32(json+offset, (const char* []){idx, "pow", NULL} )
		};
	}
}

static void _json_load_logic_level_sequence(const char* json, seq_entry_t* pulse) {
	int offset = json_query_location( json, (const char* []) {"v2", "logic_level_sequence", NULL} );

	assert(offset != -1);

	int count = json_query_count( json+offset, (const char* []){NULL} );

	assert(count > 0);
	assert(count < 16383 - 2);

	vec_t(logic_t)* vec = init_vec(logic_t);

	for (int i = 0; i < count; i++) {
		char idx[6] = {0};

		sprintf(idx, "%d", i);

		vec_push(vec, (logic_t){
			.hold_ns = json_query_i32(json+offset, (const char* []){idx, "hold_ns", NULL} ),
			.state = json_query_i32(json+offset, (const char* []){idx, "state", NULL} )
		});
	}

	// FIXME: Нужно обновить терминатор в других местах с vec_push
	vec_push(vec, (logic_t){ .hold_ns = 1*1000, .state = 0 } );
	vec_push(vec, (logic_t){ .hold_ns = 1*1000, .state = 0 } );
	vec_push(vec, (logic_t){ 0 });

	pulse->logic_level_sequence = lower_logic_sequence(vec->elements);

	free_vec(vec);
}

void sequencer_add_json_cmd(const char* str) {
	const char* json = str + 8;

	int ram = json_query_count( json, (const char* []) {"v2", "ram", NULL} );

	seq_entry_t pulse = {
		.fsc = state.fsc
	};

	if (ram > 0) {
		_json_load_ram_profiles(json, &pulse);
	} else {
		_json_load_tone_profiles(json, &pulse);
	}

	_json_load_logic_level_sequence(json, &pulse);

	sequencer_add(pulse);
}

void sequencer_cmd(const char* str) {
	char cmd[32] = {0};

	int rc = sscanf(str, "%*s %31s", cmd);

	if (rc != 1) {
		printf("Invalid arguments\n");
		printf("Usage: seq reset\n");
		printf("Usage: seq show\n");
		printf("Usage: seq pulse ...\n");
		printf("Usage: seq sweep ...\n");
		printf("Usage: seq json ...\n");
		printf("Usage: seq run\n");
		printf("Usage: seq stop\n");
		return;
	}

	if (strcmp(cmd, "reset") == 0) return sequencer_reset();
	if (strcmp(cmd, "show") == 0) return sequencer_show();
	if (strcmp(cmd, "pulse") == 0) return sequencer_add_pulse_cmd(str);
	if (strcmp(cmd, "sweep") == 0) return sequencer_add_sweep_cmd(str);
	if (strcmp(cmd, "json") == 0) return sequencer_add_json_cmd(str);
	if (strcmp(cmd, "run") == 0) return sequencer_run();
	if (strcmp(cmd, "stop") == 0) return sequencer_stop();

	printf("Unknown sequencer command: [%s]\n", cmd);
}

void radar_emulator_cmd(const char* str) {
	char d_unit[4] = {0};
	char f_unit[4] = {0};
	double duration;
	double freq;
	int limit;

	int rc = sscanf(str, "%*s %lf %s %lf %s %d", &freq, f_unit, &duration, d_unit, &limit);

	if (rc != 4 && rc != 5) {
		printf("Invalid arguments\n");
		printf("Usage: radar_emulator freq unit duration unit [count]\n");
		printf("Example - infinite pulses: radar_emulator 25 Hz 12 us\n");
		printf("Example - just one pulse: radar_emulator 25 Hz 12 us 1\n");
		return;
	}

	if (rc == 4) {
		limit = 0;
	} else {
		if (!limit) {
			radar_emulator_stop();
			return;
		}

		if (limit > 65535) {
			printf("Pulse count must be no greater than 65535\n");
			return;
		}
	}

	if (freq == 0.0 || duration == 0.0) {
		radar_emulator_stop();
		return;
	}

	double freq_hz = parse_freq(freq, f_unit);
	double duration_ns = parse_time(duration, d_unit);

	radar_emulator_start(freq_hz, duration_ns / 1000.0 / 1000.0 / 1000.0, limit);
}

void trig_cmd(const char* str) {
	char mode[5] = {0};

	int rc = sscanf(str, "%*s %4s", mode);

	if (rc != 1) {
		printf("Invalid arguments\n");
		printf("Usage: trig mode\n");
		printf("Modes are: rise, fall\n");
		return;
	}

	if (strcmp(mode, "rise") == 0) {
		state.trigger = TRIG_RISE;
		printf("Trigger set to rising\n");
	} else if (strcmp(mode, "fall") == 0) {
		state.trigger = TRIG_FALL;
		printf("Trigger set to falling\n");
	} else {
		printf("Invalid mode; valid are: rise, fall\n");
	}
}

#include "FreeRTOS.h"
#include "queue.h"

#include "events.h"
#include "ack.h"

// Режим следования за DDC
// Получение UDP пакетов было вынесено в ack.c внутрь демона событий DDC
//
// Принцип действия:
// - [РАНЬШЕ] Запуск при первом пакете от DDC, остановка при остутствии пакетов в течение 1000 мс
// - [СЕЙЧАС] Запуск при первом пакете, остановка если два непризнанных триггера - или если нет событий в течение 1000 мс
//
// Любой пакет (пакетов за раз много) от DDC считается признанием всех триггеров
//
void wait_mcast_packet() {
	uint32_t triggers_unacknowledged = 99;
	event_t evt;

	extern QueueHandle_t event_queue;

	// Опустошить очередь
	xQueueReset(event_queue);

	// Ждём первый Acknowledgement
	do {
		int rc = printf("Waiting...\n");

		if (rc < 0) // Не можем отправить "waiting"? значит TCP клиент разорвал сессию
			return;

		while (xQueueReceive(event_queue, &evt, 1000) == pdPASS) {
			if (evt.origin == DDC_ACK_EVENT) {
				triggers_unacknowledged = 0;
				break; // Чтобы не застрять - события пойдут плотным потоком
			}
		}
	} while (triggers_unacknowledged);

	// Повторно опустошить
	xQueueReset(event_queue);

	// Запуск
	sequencer_run();

	printf("Running\n");

	// Разбираем события
	while (xQueueReceive(event_queue, &evt, 1000) == pdPASS) {
		int rc = 0;

		switch (evt.origin) {
			case TRIGGER_EVENT:
				rc = printf("%llu\tTRIG\n", evt.timestamp);
				triggers_unacknowledged++;
				break;
			case READY_EVENT:
				rc = printf("%llu\tREADY\n", evt.timestamp);
				break;
			case DDC_ACK_EVENT:
				if (triggers_unacknowledged)
					rc = printf("%llu\tDDC ACK\n", evt.timestamp);
				triggers_unacknowledged = 0;
				break;
		
			default:
				assert(0);
		}

		if (triggers_unacknowledged >= 2)
			break;

		if (rc < 0)
			break;
	}

	// Здесь делаем сброс чтобы противодействовать фрагментации
	sequencer_reset();

	printf("Stop\n");
}

void run(const char* str) {
	char cmd[32] = {0};

	int rc = sscanf(str, "%31s", cmd);

	if (rc == 0) {
		printf("Invalid str\n");
		return;
	}

	if (strcmp(cmd, "isr") == 0) return print_it();
	if (strcmp(cmd, "mem") == 0) return print_mem();
	if (strcmp(cmd, "seq") == 0) return sequencer_cmd(str);
	if (strcmp(cmd, "perf") == 0) return print_perf();
	if (strcmp(cmd, "write") == 0) return ad_write_all();
	if (strcmp(cmd, "verify") == 0) return ad_readback_all();
	if (strcmp(cmd, "ram_test") == 0) return ad_ram_test();
	if (strcmp(cmd, "rfkill") == 0) return enter_rfkill_mode();
	if (strcmp(cmd, "set_level") == 0) return set_level_cmd(str);
	if (strcmp(cmd, "dbg_level") == 0) return dbg_level_cmd(str);
	if (strcmp(cmd, "test_tone") == 0) return test_tone_cmd(str);
	if (strcmp(cmd, "basic_pulse") == 0) return basic_pulse_cmd(str);
	if (strcmp(cmd, "basic_sweep") == 0) return basic_sweep_cmd(str);
	if (strcmp(cmd, "basic_xmitdata") == 0) return basic_xmitdata_cmd(str);
	if (strcmp(cmd, "radar_emulator") == 0) return radar_emulator_cmd(str);
	if (strcmp(cmd, "wait") == 0) return wait_mcast_packet();
	if (strcmp(cmd, "trig") == 0) return trig_cmd(str);
	if (strcmp(cmd, "reboot") == 0) return NVIC_SystemReset();

	printf("Unknown command: [%s]\n", cmd);
}
