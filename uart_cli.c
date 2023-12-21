#include <stdint.h>
#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "isr.h"
#include "performance.h"
#include "ad9910.h"
#include "units.h"
#include "timer.h"
#include "sequencer.h"
#include "algos.h"
#include "vec.h"

// =============================================================================
// CLI COMMANDS
// =============================================================================
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

	vec_t(uint8_t)* ram = init_vec(uint8_t);

	for (size_t i = 0; i < element_count; i++) {
		vec_push(ram, 0x00);
		vec_push(ram, 0x00);
		vec_push(ram, 0xFF);
		vec_push(ram, 0xFC);
	}

	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
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
		.ram_image = { .buffer = (uint32_t*)ram->elements, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ad_calc_ftw(freq_hz) }
	};

	sequencer_add(pulse);
	sequencer_run();
}

void basic_sweep_cmd(const char* str) {
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	double offset;
	double duration;
	double f1;
	double f2;

	int rc = sscanf(str, "%*s %lf %3s %lf %3s %lf %3s %lf %3s", &offset, o_unit, &duration, d_unit, &f1, f1_unit, &f2, f2_unit);

	if (rc != 8) {
		printf("Invalid arguments\n");
		printf("Usage: basic_sweep delay unit duration unit freq_start unit freq_end unit\n");
		printf("Example: basic_sweep 100 us 250 us 140 MHz 160 MHz\n");
		return;
	}

	uint32_t f1_hz = parse_freq(f1, f1_unit);
	uint32_t f2_hz = parse_freq(f2, f2_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t duration_ns = parse_time(duration, d_unit);

	if (f1_hz == 0 || f2_hz == 0) {
		return;
	}

	char* verif_f1 = freq_unit(f1_hz);
	char* verif_f2 = freq_unit(f2_hz);
	char* verif_offset = time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration = time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic sweep from %s to %s, offset %s, duration %s\n", verif_f1, verif_f2, verif_offset, verif_duration);

	free(verif_f1);
	free(verif_f2);
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

	vec_t(uint8_t)* ram = init_vec(uint8_t);

	for (size_t i = 0; i < element_count; i++) {
		vec_push(ram, compensation >> 8);
		vec_push(ram, compensation & 0xFF);
		vec_push(ram, 0xFF);
		vec_push(ram, 0xFC);
	}

	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.sweep = calculate_sweep(f1_hz, f2_hz, duration_ns),
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
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
		.ram_image = { .buffer = (uint32_t*)ram->elements, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR
	};

	sequencer_add(pulse);
	sequencer_run();
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

void prepare_for_dma(uint8_t* ptr) {
	*ptr = profile_to_gpio_states(*ptr) >> 8;
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

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	for_every_entry(vec, prepare_for_dma);

	uint32_t f1_hz = parse_freq(f1, f1_unit);
	uint32_t f2_hz = parse_freq(f2, f2_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);
	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (f1_hz == 0 || f2_hz == 0) {
		return;
	}

	if (tstep_ns >= MAX_NS_16BIT_216MHz) {
		printf("Error: tstep too large\n");
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

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[2] = { .ftw = ad_calc_ftw(f1_hz), .asf = 0x3FFF },
		.profiles[3] = { .ftw = ad_calc_ftw(f2_hz), .asf = 0x3FFF },
		.profile_modulation = { .buffer = vec->elements, .size = vec->size, .tstep = timer_mu(tstep_ns) }
	};

	sequencer_add(pulse);
	sequencer_run();
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

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	for_every_entry(vec, prepare_for_dma);

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);
	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (freq_hz == 0) {
		return;
	}

	if (tstep_ns >= MAX_NS_16BIT_216MHz) {
		printf("Error: tstep too large\n");
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

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[2] = { .ftw = ftw, .pow = 0x0000, .asf = 0x3FFF },
		.profiles[3] = { .ftw = ftw, .pow = 0x7FFF, .asf = 0x3FFF },
		.profile_modulation = { .buffer = vec->elements, .size = vec->size, .tstep = timer_mu(tstep_ns) }
	};

	sequencer_add(pulse);
	sequencer_run();
}

uint8_t bpsk_ram_image[] = {
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0x00, 0x00,
	0x00, 0x00, 0xFF, 0xFC,
	0x7F, 0xFF, 0xFF, 0xFC
};

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

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	for_every_entry(vec, prepare_for_dma);

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t offset_ns = parse_time(offset, o_unit);
	uint32_t tstep_ns = parse_time(tstep, ts_unit);
	uint32_t duration_ns = tstep_ns * vec->size;
	
	if (freq_hz == 0) {
		return;
	}

	if (tstep_ns >= MAX_NS_16BIT_216MHz) {
		printf("Error: tstep too large\n");
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

	/*
	int highest_bit = 0;

	while (tstep_ns >> highest_bit)
		highest_bit++;

	highest_bit -= 4;

	uint32_t keep_mask = ~(0xFFFFFFFF >> highest_bit);
	uint32_t ftw = ad_calc_ftw(freq_hz) & keep_mask;

	printf("Keep mask: 0x%08lX\n", keep_mask);
	*/

	uint32_t ftw = ad_calc_ftw(freq_hz) & 0xFFFC0000;

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.ram_profiles[0] = { .start = 0, .end = 0, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.ram_profiles[2] = { .start = 2, .end = 2, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.ram_profiles[3] = { .start = 3, .end = 3, .rate = 0, .mode = AD_RAM_PROFILE_MODE_ZEROCROSSING },
		.profile_modulation = { .buffer = vec->elements, .size = vec->size, .tstep = timer_mu(tstep_ns) },
		.ram_image = { .buffer = (uint32_t*)bpsk_ram_image, .size = 4 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ftw }
	};

	sequencer_add(pulse);
	sequencer_run();
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

	vec_t(uint8_t)* ram = init_vec(uint8_t);
	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	size_t element_count = vec->size;

	for (size_t i = 0; i < element_count; i++) {
		if (vec->elements[i] == 0) {
			vec_push(ram, 0x00);
			vec_push(ram, 0x00);
			vec_push(ram, 0xFF);
			vec_push(ram, 0xFC);
		} else {
			vec_push(ram, 0x7F);
			vec_push(ram, 0xFF);
			vec_push(ram, 0xFF);
			vec_push(ram, 0xFC);
		}
	}

	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);
	vec_push(ram, 0x00);

	free_vec(vec);

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

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
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
		.ram_image = { .buffer = (uint32_t*)ram->elements, .size = element_count + 1 },
		.ram_destination = AD_RAM_DESTINATION_POLAR,
		.ram_secondary_params = { .ftw =  ftw }
	};

	sequencer_add(pulse);
	sequencer_run();
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
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.profiles[0] = { .ftw = 0, .asf = 0 },
		.profiles[1] = { .ftw = ad_calc_ftw(freq_hz), .asf = 0x3FFF }
	};

	sequencer_add(pulse);

	free(verif_freq);
	free(verif_offset);
	free(verif_duration);
}

void sequencer_add_sweep_cmd(const char* str) {
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	double offset;
	double duration;
	double f1;
	double f2;

	int rc = sscanf(str, "%*s %*s %lf %3s %lf %3s %lf %3s %lf %3s", &offset, o_unit, &duration, d_unit, &f1, f1_unit, &f2, f2_unit);

	if (rc != 8) {
		printf("Invalid arguments\n");
		printf("Usage: seq sweep delay unit duration unit f1 unit f2 unit\n");
		printf("Example: seq sweep 100 us 250 us 150 MHz 50 MHz\n");
		return;
	}

	uint32_t f1_hz 			= parse_freq(f1, f1_unit);
	uint32_t f2_hz 			= parse_freq(f2, f2_unit);
	uint32_t offset_ns 		= parse_time(offset, o_unit);
	uint32_t duration_ns 	= parse_time(duration, d_unit);
	
	char* verif_f1 			= freq_unit(f1_hz);
	char* verif_f2 			= freq_unit(f2_hz);
	char* verif_offset 		= time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration 	= time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Sequence sweep at %s -> %s, offset %s, duration %s\n", verif_f1, verif_f2, verif_offset, verif_duration);

	seq_entry_t pulse = {
		.sweep = calculate_sweep(f1_hz, f2_hz, duration_ns),
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.profiles[0] = { .asf = 0 },
		.profiles[1] = { .asf = 0x3FFF }
	};

	sequencer_add(pulse);

	free(verif_f1);
	free(verif_f2);
	free(verif_offset);
	free(verif_duration);
}

void sequencer_cmd(const char* str) {
	char cmd[32] = {0};

	int rc = sscanf(str, "%*s %31s", cmd);

	if (rc != 1) {
		printf("Invalid arguments\n");
		printf("Usage: seq reset\n");
		printf("Usage: seq show\n");
		printf("Usage: seq pulse delay unit duration unit freq unit\n");
		printf("Usage: seq sweep delay unit duration unit f1 unit f2 unit\n");
		printf("Usage: seq run\n");
		printf("Usage: seq stop\n");
		return;
	}

	if (strcmp(cmd, "reset") == 0) return sequencer_reset();
	if (strcmp(cmd, "show") == 0) return sequencer_show();
	if (strcmp(cmd, "pulse") == 0) return sequencer_add_pulse_cmd(str);
	if (strcmp(cmd, "sweep") == 0) return sequencer_add_sweep_cmd(str);
	if (strcmp(cmd, "run") == 0) return sequencer_run();
	if (strcmp(cmd, "stop") == 0) return sequencer_stop();

	printf("Unknown sequencer command: [%s]\n", cmd);
}

void run(const char* str) {
	char cmd[32] = {0};

	int rc = sscanf(str, "%31s", cmd);

	if (rc == 0) {
		printf("Invalid str\n");
		return;
	}

	if (strcmp(cmd, "isr") == 0) return print_it();
	if (strcmp(cmd, "seq") == 0) return sequencer_cmd(str);
	if (strcmp(cmd, "perf") == 0) return print_perf();
	if (strcmp(cmd, "write") == 0) return ad_write_all();
	if (strcmp(cmd, "verify") == 0) return ad_readback_all();
	if (strcmp(cmd, "ram_test") == 0) return ad_ram_test();
	if (strcmp(cmd, "rfkill") == 0) return enter_rfkill_mode();
	if (strcmp(cmd, "test_tone") == 0) return test_tone_cmd(str);
	if (strcmp(cmd, "basic_pulse") == 0) return basic_pulse_cmd(str);
	if (strcmp(cmd, "basic_sweep") == 0) return basic_sweep_cmd(str);
	if (strcmp(cmd, "basic_xmitdata") == 0) return basic_xmitdata_cmd(str);

	printf("Unknown command: [%s]\n", cmd);
}
