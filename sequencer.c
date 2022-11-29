#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pulse.h"
#include "timer.h"
#include "ad9910.h"
#include "vec.h"

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	io_update_software_controlled();
	timer2_stop();

	ad_set_profile_freq(0, 0);
	ad_set_profile_amplitude(0, 0);
	ad_disable_ramp();
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

extern TIM_HandleTypeDef timer2;

void enter_basic_pulse_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t freq_hz) {
	enter_rfkill_mode();

	ad_set_profile_freq(1, freq_hz);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_write_all();

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = malloc(sizeof(pulse_t) * 2);

	pulse_set_timing(timer_pulse_sequence + 0, t0_ns, t1_ns);
	pulse_set_timing(timer_pulse_sequence + 1, 0, 0);

	timer2_restart();

	// Можно перезаписывать регистры на лету
	timer2.Instance->CCR3 = timer_pulse_sequence[0].t1;
	timer2.Instance->CCR4 = timer_pulse_sequence[0].t2;
}

void enter_basic_sweep_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t f1_hz, uint32_t f2_hz) {
	enter_rfkill_mode();

	ad_enable_ramp();
	ad_set_ramp_limits(f1_hz, f2_hz);
	ad_set_ramp_step(1, 1);
	ad_set_ramp_rate(1, 1);
	ad_write_all();
	ad_pulse_io_update();

	if (timer_pulse_sequence != NULL)
		free(timer_pulse_sequence);

	timer_pulse_sequence = malloc(sizeof(pulse_t) * 2);

	pulse_set_timing(timer_pulse_sequence + 0, t0_ns, t1_ns);
	pulse_set_timing(timer_pulse_sequence + 1, 0, 0);

	timer2_restart();

	timer2.Instance->CCR3 = timer_pulse_sequence[0].t1;
	timer2.Instance->CCR4 = timer_pulse_sequence[0].t2;
}


void sequencer_reset() {

}

void sequencer_show() {

}

void sequencer_run() {

}

void sequencer_stop() {

}
