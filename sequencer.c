#include <stdlib.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "timer.h"
#include "pulse.h"
#include "ad9910.h"
#include "vec.h"

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	timer2_stop();

	ad_set_profile_freq(0, 0);
	ad_set_profile_amplitude(0, 0);
	ad_disable_ramp();
	ad_write_all();
	ad_pulse_io_update();

	set_profile(0);
}

// Подавать непрерывный сигнал
void enter_test_tone_mode(uint32_t freq_hz) {
	enter_rfkill_mode();

	ad_set_profile_freq(1, freq_hz);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_write_all();
	//ad_pulse_io_update();

	set_profile(1);
}

pulse_t* timer_pulse_sequence = NULL;

extern TIM_HandleTypeDef timer2;

void enter_basic_pulse_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t freq_hz) {
	enter_rfkill_mode();

	ad_set_profile_freq(1, freq_hz);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_write_all();

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = malloc(sizeof(pulse_t) * 2);

	pulse_t null_pulse = {0};
	pulse_t pulse;

	pulse.timing = timer_points(t0_ns, t1_ns);

	timer_pulse_sequence[0] = pulse;
	timer_pulse_sequence[1] = null_pulse;

	timer2_restart();

	// Можно перезаписывать регистры на лету
	timer2.Instance->CCR3 = timer_pulse_sequence[0].timing.t1;
	timer2.Instance->CCR4 = timer_pulse_sequence[0].timing.t2;
}

void enter_basic_sweep_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t f1_hz, uint32_t f2_hz) {
	enter_rfkill_mode();

	set_ramp_direction(0);
	ad_enable_ramp();

	uint32_t step_ftw = ad_calc_ramp_step_ftw(f1_hz, f2_hz, t1_ns);

	ad_set_ramp_limits(f1_hz, f2_hz);
	ad_set_ramp_step(0, step_ftw);
	ad_set_ramp_rate(1, 1);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_write_all();
	ad_pulse_io_update();
	set_ramp_direction(1);

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = malloc(sizeof(pulse_t) * 2);

	pulse_t null_pulse = {0};
	pulse_t pulse;

	pulse.timing = timer_points(t0_ns, t1_ns);

	timer_pulse_sequence[0] = pulse;
	timer_pulse_sequence[1] = null_pulse;

	timer2_restart();

	timer2.Instance->CCR3 = timer_pulse_sequence[0].timing.t1;
	timer2.Instance->CCR4 = timer_pulse_sequence[0].timing.t2;
}


static void debug_print_pulse(pulse_t* pulse) {
	printf(" === pulse_t ===\n");
	printf(" t1: %d\n", pulse->timing.t1);
	printf(" t2: %d\n", pulse->timing.t2);
	printf(" ===============\n");
}

vec_t* sequence;

void sequencer_init() {
	sequence = init_vec();
}

void sequencer_reset() {
	clear_vec(sequence);
}

void sequencer_show() {
	for_every_entry(sequence, debug_print_pulse);
}

void sequencer_add(pulse_t pulse) {
	vec_push(sequence, pulse);
}

void sequencer_run() {
	enter_rfkill_mode();

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = sequence->elements;

	timer2_restart();

	// Можно перезаписывать регистры на лету
	timer2.Instance->CCR3 = timer_pulse_sequence[0].timing.t1;
	timer2.Instance->CCR4 = timer_pulse_sequence[0].timing.t2;
}

void sequencer_stop() {
	enter_rfkill_mode();
}

void pulse_complete_callback() {
	// Здесь также будет код для перезаписи регистров на лету
}
