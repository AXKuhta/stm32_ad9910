#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "timer.h"
#include "ad9910.h"

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	timer2_stop();

	set_profile(7);
}

// Подавать непрерывный сигнал
void enter_test_tone_mode(uint32_t freq_hz) {
	enter_rfkill_mode();

	ad_set_profile_freq(0, freq_hz);
	ad_set_profile_amplitude(0, 0x3FFF);
	ad_write_all();

	set_profile(0);
}

pulse_t* pulse_sequence;
int pulse_idx;
int pulse_max;
int pulse_t1_pass;

void enter_basic_pulse_mode() {
	pulse_sequence = malloc(1 * sizeof(pulse_t));

	pulse_sequence[0] = timer_pulse(250*1000, 500*1000);

	pulse_idx = 0;
	pulse_max = 1;
	pulse_t1_pass = 1;

	timer2_restart();
}
