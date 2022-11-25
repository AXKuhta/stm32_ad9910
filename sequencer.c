#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "timer.h"
#include "ad9910.h"

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	io_update_software_controlled();
	timer2_stop();

	ad_set_profile_freq(0, 0);
	ad_set_profile_amplitude(0, 0);
	ad_write_all();
	//ad_pulse_io_update();

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
pulse_t null_pulse = {0};

void enter_basic_pulse_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t freq_hz) {
	enter_rfkill_mode();

	ad_set_profile_freq(1, freq_hz);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_write_all();

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = malloc(sizeof(pulse_t) * 2);

	timer_pulse_sequence[0] = timer_pulse(t0_ns, t1_ns);
	timer_pulse_sequence[1] = null_pulse;

	timer2_restart();
}
