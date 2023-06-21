#include <stdio.h>
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "tasks.h"
#include "isr.h"
#include "syscalls.h"
#include "performance.h"
#include "ad9910.h"
#include "units.h"
#include "timer.h"
#include "sequencer.h"
#include "vec.h"

// =============================================================================
// CLI COMMANDS
// =============================================================================
void test_tone_cmd(const char* str) {
	char cmd[32] = {0};
	char unit[4] = {0};
	double freq;

	int rc = sscanf(str, "%31s %lf %3s", cmd, &freq, unit);

	if (rc != 3) {
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
	char cmd[32] = {0};
	char t1_unit[4] = {0};
	char t2_unit[4] = {0};
	char f_unit[4] = {0};
	double t1;
	double t2;
	double freq;

	int rc = sscanf(str, "%31s %lf %3s %lf %3s %lf %3s", cmd, &t1, t1_unit, &t2, t2_unit, &freq, f_unit);

	if (rc != 7) {
		printf("Invalid arguments\n");
		printf("Usage: basic_pulse delay unit duration unit freq unit\n");
		printf("Example: basic_pulse 100 us 250 us 150 MHz\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t t1_ns = parse_time(t1, t1_unit);
	uint32_t t2_ns = parse_time(t2, t2_unit);

	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);
	char* verif_t1 = time_unit(t1_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_t2 = time_unit(t2_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic pulse at %s, offset %s, duration %s\n", verif_freq, verif_t1, verif_t2);

	free(verif_freq);
	free(verif_t1);
	free(verif_t2);

	enter_basic_pulse_mode(t1_ns, t2_ns, freq_hz);
}

void basic_sweep_cmd(const char* str) {
	char cmd[32] = {0};
	char t1_unit[4] = {0};
	char t2_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	double t1;
	double t2;
	double f1;
	double f2;

	int rc = sscanf(str, "%31s %lf %3s %lf %3s %lf %3s %lf %3s", cmd, &t1, t1_unit, &t2, t2_unit, &f1, f1_unit, &f2, f2_unit);

	if (rc != 9) {
		printf("Invalid arguments\n");
		printf("Usage: basic_sweep delay unit duration unit freq_start unit freq_end unit\n");
		printf("Example: basic_sweep 100 us 250 us 140 MHz 160 MHz\n");
		return;
	}

	uint32_t f1_hz = parse_freq(f1, f1_unit);
	uint32_t f2_hz = parse_freq(f2, f2_unit);
	uint32_t t1_ns = parse_time(t1, t1_unit);
	uint32_t t2_ns = parse_time(t2, t2_unit);

	if (f1_hz == 0 || f2_hz == 0) {
		return;
	}

	char* verif_f1 = freq_unit(f1_hz);
	char* verif_f2 = freq_unit(f2_hz);
	char* verif_t1 = time_unit(t1_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_t2 = time_unit(t2_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic sweep from %s to %s, offset %s, duration %s\n", verif_f1, verif_f2, verif_t1, verif_t2);

	free(verif_f1);
	free(verif_f2);
	free(verif_t1);
	free(verif_t2);

	enter_basic_sweep_mode(t1_ns, t2_ns, f1_hz, f2_hz);
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
	char basic_xmitdata[15] = {0};
	char cmd[32] = {0};
	char t1_unit[4] = {0};
	char t2_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	double t1;
	double t2;
	double f1;
	double f2;
	double rate;
	size_t data_offset;

	int rc = sscanf(str, "%14s %31s %lf %3s %lf %3s %lf %3s %lf %3s %lf %n", basic_xmitdata, cmd, &t1, t1_unit, &t2, t2_unit, &f1, f1_unit, &f2, f2_unit, &rate, &data_offset);

	if (rc != 11) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata fsk offset unit duration unit f1 unit f2 unit rate data data data ...\n");
		printf("Example: basic_xmitdata fsk 0 us 1 ms 151.10 MHz 151.11 MHz 9600 1 1 0 1 ...\n");
		return;
	}

	uint32_t f1_hz = parse_freq(f1, f1_unit);
	uint32_t f2_hz = parse_freq(f2, f2_unit);
	uint32_t t1_ns = parse_time(t1, t1_unit);
	uint32_t t2_ns = parse_time(t2, t2_unit);
	
	if (f1_hz == 0 || f2_hz == 0) {
		return;
	}

	char* verif_f1 = freq_unit(f1_hz);
	char* verif_f2 = freq_unit(f2_hz);
	char* verif_t1 = time_unit(t1_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_t2 = time_unit(t2_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic FSK at %s + %s, offset %s, duration %s, rate %.1lf baud\n", verif_f1, verif_f2, verif_t1, verif_t2, rate);

	free(verif_f1);
	free(verif_f2);
	free(verif_t1);
	free(verif_t2);

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	for_every_entry(vec, prepare_for_dma);

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(t1_ns),
		.t2 = timer_mu(t1_ns + t2_ns),
		.profiles[0] = { .freq_hz = 0, .amplitude = 0 },
		.profiles[2] = { .freq_hz = f1_hz, .amplitude = 0x3FFF },
		.profiles[3] = { .freq_hz = f2_hz, .amplitude = 0x3FFF },
		.profile_modulation = { .buffer = vec->elements, .size = vec->size }
	};

	sequencer_add(pulse);
	sequencer_run();
}

void xmitdata_psk_cmd(const char* str) {
	char basic_xmitdata[15] = {0};
	char cmd[32] = {0};
	char t1_unit[4] = {0};
	char t2_unit[4] = {0};
	char f_unit[4] = {0};
	double t1;
	double t2;
	double freq;
	double rate;
	size_t data_offset;

	int rc = sscanf(str, "%14s %31s %lf %3s %lf %3s %lf %3s %lf %n", basic_xmitdata, cmd, &t1, t1_unit, &t2, t2_unit, &freq, f_unit, &rate, &data_offset);

	if (rc != 9) {
		printf("Invalid arguments: %d\n", rc);
		printf("Usage: basic_xmitdata psk offset unit duration unit freq unit rate data data data ...\n");
		printf("Example: basic_xmitdata psk 0 us 1 ms 151.10 MHz 9600 1 1 0 1 ...\n");
		return;
	}

	uint32_t freq_hz = parse_freq(freq, f_unit);
	uint32_t t1_ns = parse_time(t1, t1_unit);
	uint32_t t2_ns = parse_time(t2, t2_unit);
	
	if (freq_hz == 0) {
		return;
	}

	char* verif_freq = freq_unit(freq_hz);
	char* verif_t1 = time_unit(t1_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_t2 = time_unit(t2_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Basic PSK at %s, offset %s, duration %s, rate %.1lf baud\n", verif_freq, verif_t1, verif_t2, rate);

	free(verif_freq);
	free(verif_t1);
	free(verif_t2);

	vec_t(uint8_t)* vec = scan_uint8_data(str + data_offset);
	for_every_entry(vec, prepare_for_dma);

	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.t1 = timer_mu(t1_ns),
		.t2 = timer_mu(t1_ns + t2_ns),
		.profiles[0] = { .freq_hz = 0, .amplitude = 0 },
		.profiles[2] = { .freq_hz = freq_hz, .amplitude = 0x3FFF, .phase = 0x0 },
		.profiles[3] = { .freq_hz = freq_hz, .amplitude = 0x3FFF, .phase = 0xFFFF/2 },
		.profile_modulation = { .buffer = vec->elements, .size = vec->size }
	};

	sequencer_add(pulse);
	sequencer_run();
}

void basic_xmitdata_cmd(const char* str) {
	char basic_xmitdata[15] = {0};
	char cmd[32] = {0};

	int rc = sscanf(str, "%14s %31s", basic_xmitdata, cmd);

	if (rc != 2) {
		printf("Invalid arguments\n");
		printf("Usage: basic_xmitdata mode ...\n");
		return;
	}

	if (strcmp(cmd, "fsk") == 0) return xmitdata_fsk_cmd(str);
	if (strcmp(cmd, "psk") == 0) return xmitdata_psk_cmd(str);

	printf("Unknown xmitdata mode: [%s]\n", cmd);
}

void sequencer_add_pulse_cmd(const char* str) {
	char seq[4] = {0};
	char cmd[32] = {0};
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f_unit[4] = {0};
	double offset;
	double duration;
	double freq;

	int rc = sscanf(str, "%3s %31s %lf %3s %lf %3s %lf %3s", seq, cmd, &offset, o_unit, &duration, d_unit, &freq, f_unit);

	if (rc != 8) {
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
		.profiles[0] = { .freq_hz = 0, .amplitude = 0 },
		.profiles[1] = { .freq_hz = freq_hz, .amplitude = 0x3FFF }
	};

	sequencer_add(pulse);

	free(verif_freq);
	free(verif_offset);
	free(verif_duration);
}

void sequencer_add_sweep_cmd(const char* str) {
	char seq[4] = {0};
	char cmd[32] = {0};
	char o_unit[4] = {0};
	char d_unit[4] = {0};
	char f1_unit[4] = {0};
	char f2_unit[4] = {0};
	char fstep_unit[4] = {0};
	double offset;
	double duration;
	double f1;
	double f2;
	double fstep;

	int rc = sscanf(str, "%3s %31s %lf %3s %lf %3s %lf %3s %lf %3s %lf %3s", seq, cmd, &offset, o_unit, &duration, d_unit, &f1, f1_unit, &f2, f2_unit, &fstep, fstep_unit);

	if (rc != 10 && rc != 12) {
		printf("Invalid arguments\n");
		printf("Usage: seq sweep delay unit duration unit f1 unit f2 unit [fstep unit]\n");
		printf("Example: seq sweep 100 us 250 us 150 MHz 50 MHz\n");
		return;
	}

	uint32_t f1_hz 			= parse_freq(f1, f1_unit);
	uint32_t f2_hz 			= parse_freq(f2, f2_unit);
	uint32_t offset_ns 		= parse_time(offset, o_unit);
	uint32_t duration_ns 	= parse_time(duration, d_unit);
	uint32_t fstep_hz 		= 0;

	if (rc == 12)
		fstep_hz = parse_freq(fstep, fstep_unit);
	
	char* verif_f1 			= freq_unit(f1_hz);
	char* verif_f2 			= freq_unit(f2_hz);
	char* verif_fstep 		= freq_unit(fstep_hz);
	char* verif_offset 		= time_unit(offset_ns / 1000.0 / 1000.0 / 1000.0);
	char* verif_duration 	= time_unit(duration_ns / 1000.0 / 1000.0 / 1000.0);

	printf("Sequence sweep at %s -> %s step %s, offset %s, duration %s\n", verif_f1, verif_f2, rc == 12 ? verif_fstep : "AUTO", verif_offset, verif_duration);

	ad_ramp_t ramp;

	if (rc == 12) {
		ramp = (ad_ramp_t){
			.fstep_ftw = ad_calc_ftw(fstep_hz),
			.tstep_mul = 1
		};
	} else {
		ramp = ad_calc_ramp(f1_hz, f2_hz, duration_ns);
	}

	seq_entry_t pulse = {
		.sweep = {
			.f1 = f1_hz,
			.f2 = f2_hz,
			.fstep = ramp.fstep_ftw,
			.tstep = ramp.tstep_mul
		},
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns),
		.profiles[0] = { .amplitude = 0 },
		.profiles[1] = { .amplitude = 0x3FFF }
	};

	sequencer_add(pulse);

	free(verif_f1);
	free(verif_f2);
	free(verif_fstep);
	free(verif_fstep);
	free(verif_duration);
}

void sequencer_cmd(const char* str) {
	char seq[4] = {0};
	char cmd[32] = {0};

	int rc = sscanf(str, "%3s %31s", seq, cmd);

	if (rc != 2) {
		printf("Invalid arguments\n");
		printf("Usage: seq reset\n");
		printf("Usage: seq show\n");
		printf("Usage: seq pulse delay unit duration unit freq unit\n");
		printf("Usage: seq sweep delay unit duration unit f1 unit f2 unit {fstep unit | auto}\n");
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

void test_cmd() {
	sequencer_stop();
	sequencer_reset();

	uint8_t* x = malloc(6);

	x[0] = profile_to_gpio_states(3) >> 8;
	x[1] = profile_to_gpio_states(3) >> 8;
	x[2] = profile_to_gpio_states(3) >> 8;
	x[3] = profile_to_gpio_states(2) >> 8;
	x[4] = profile_to_gpio_states(3) >> 8;
	x[5] = profile_to_gpio_states(0) >> 8;

	uint32_t* ram = malloc(4*4);

	memcpy(ram, (uint8_t[]) {
		0x7F, 0xFF, 0xFF, 0xFC,
		0x00, 0x00, 0xFF, 0xFC,
		0x00, 0x00, 0x00, 0x00,
		0x00, 0x00, 0x00, 0x00
	}, 4*4);

	seq_entry_t pulse = {
		.t1 = timer_mu(0),
		.t2 = timer_mu(0 + 125*7),
		.ram_profiles[0] = { .start = 0, .end = 0, .rate = 0, .mode = 0 },
		.ram_profiles[2] = { .start = 2, .end = 2, .rate = 0, .mode = 0 },
		.ram_profiles[3] = { .start = 3, .end = 3, .rate = 0, .mode = 0 },
		.profile_modulation = { .buffer = x, .size = 6 },
		.ram_image = { .buffer = ram, .size = 4 }
	};

	ad_set_ram_freq(12*1000*1000);
	ad_ram_destination_polar();

	sequencer_add(pulse);
	sequencer_run();
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
	if (strcmp(cmd, "test") == 0) return test_cmd();

	printf("Unknown command: [%s]\n", cmd);
}

// =============================================================================
// INPUT HANDLING
// =============================================================================
extern UART_HandleTypeDef usart3;

#define INPUT_BUFFER_SIZE 1024
uint8_t input_buffer[INPUT_BUFFER_SIZE] = {0};

void restart_rx() {
	memset(input_buffer, 0, INPUT_BUFFER_SIZE);
	_write(0, "\r> ", 3);
	assert(HAL_UARTEx_ReceiveToIdle_DMA(&usart3, input_buffer, INPUT_BUFFER_SIZE - 1) == HAL_OK);
}

void uart_cli_init() {
	printf("Entering CLI\n");
	restart_rx();
}

const char* get_next_str(const char* buf) {
	while (*buf) {
		if (*buf == '\n' || *buf == '\r') return buf + 1;
		buf++;
	}

	return NULL;
}

void parse() {
	const char* buf = (const char*)input_buffer;

	do {
		run(buf);
		buf = get_next_str(buf);
	} while (*buf != 0);

	restart_rx();
}

void input_overrun_error() {
	printf("\nInput overrun error\n");
	while (1) {};
}

// This function is called when the line goes idle
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	uint16_t space_remains = huart->RxXferCount;
	uint8_t* base = huart->pRxBuffPtr;
	uint8_t* last = base - 1 + Size;

	// Performance
	extern uint32_t perf_usart3_bytes_rx;
	perf_usart3_bytes_rx += Size;

	// Echo keypresses
	_write(0, (char*)base, Size);

	// Backspace handling
	// Different terminals may send different backspace codes
	// Handle both types
	if (*last == 127) {
		_write(0, (char[]){8, ' ', 8}, 3);
		*last = 8;
	}

	if (*last == 8) {
		if (last == input_buffer) {
			return restart_rx();
		}

		*last = 0;
		last -= 2;

		space_remains++;
		base = last;
	}

	if (space_remains == 0) {
		return add_task(input_overrun_error);
	}

	// Ensure that memory will remain untouched while parsing
	if (*last == '\n' || *last == '\r') {
		_write(0, "\n", 1);
		add_task(parse);
	} else {
		HAL_UARTEx_ReceiveToIdle_DMA(huart, base + Size, space_remains);
	}	
}
