#include <stdlib.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "sequencer.h"
#include "timer.h"
#include "ad9910.h"
#include "vec.h"

extern TIM_HandleTypeDef timer2;

static vec_t* sequence;
static int seq_index;

void sequencer_init() {
	sequence = init_vec();
}

void sequencer_reset() {
	clear_vec(sequence);
}

static void debug_print_entry(seq_entry_t* entry) {
	printf(" === seq_entry_t ===\n");
	printf(" t1: %lu\n", entry->t1);
	printf(" t2: %lu\n", entry->t2);

	for (int i = 0; i < 8; i++) {
		printf(" profile %d: %lu hz\n", i, entry->profiles.freq_hz[i]);
	}

	printf(" ===============\n");
}

void sequencer_show() {
	printf("Sequencer entries: %lu\n", sequence->size);
	for_every_entry(sequence, debug_print_entry);
}

void sequencer_add(seq_entry_t entry) {
	vec_push(sequence, entry);
}

void sequencer_run() {
	enter_rfkill_mode();
	spi_write_entry(sequence->elements[0]);
	ad_write_all();

	seq_index = 0;

	timer2_restart();
	// Можно перезаписывать регистры на лету
	timer2.Instance->CCR3 = sequence->elements[0].t1;
	timer2.Instance->CCR4 = sequence->elements[0].t2;
}

void spi_write_entry(seq_entry_t entry) {
	ad_set_profile_freq(0, entry.profiles.freq_hz[0]);
	ad_set_profile_freq(1, entry.profiles.freq_hz[1]);
	ad_set_profile_freq(2, entry.profiles.freq_hz[2]);
	ad_set_profile_freq(3, entry.profiles.freq_hz[3]);
	ad_set_profile_freq(4, entry.profiles.freq_hz[4]);
	ad_set_profile_freq(5, entry.profiles.freq_hz[5]);
	ad_set_profile_freq(6, entry.profiles.freq_hz[6]);
	ad_set_profile_freq(7, entry.profiles.freq_hz[7]);

	ad_set_profile_amplitude(0, 0x0);
	ad_set_profile_amplitude(1, 0x3FFF);
	ad_set_profile_amplitude(2, 0x3FFF);
	ad_set_profile_amplitude(3, 0x3FFF);
	ad_set_profile_amplitude(4, 0x3FFF);
	ad_set_profile_amplitude(5, 0x3FFF);
	ad_set_profile_amplitude(6, 0x3FFF);
	ad_set_profile_amplitude(7, 0x3FFF);
}

void sequencer_stop() {
	enter_rfkill_mode();
}

void pulse_complete_callback() {
	int next_idx = ++seq_index % sequence->size;

	spi_write_entry(sequence->elements[next_idx]);

	// Принудительно закинуть в таймер очень большое значение, чтобы он случайно не пересёк те точки, которые мы вот вот запишем
	timer2.Instance->CNT = 0x7FFFFFFF;
	timer2.Instance->CCR3 = sequence->elements[next_idx].t1;
	timer2.Instance->CCR4 = sequence->elements[next_idx].t2;
}

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

void enter_basic_pulse_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t freq_hz) {
	sequencer_stop();
	sequencer_reset();

	seq_entry_t pulse = {
		.profiles.freq_hz[0] = 0,
		.profiles.freq_hz[1] = freq_hz,
		.t1 = timer_mu(offset_ns),
		.t2 = timer_mu(offset_ns + duration_ns)
	};

	sequencer_add(pulse);
	sequencer_run();
}

void enter_basic_sweep_mode(uint32_t offset_ns, uint32_t duration_ns, uint32_t f1_hz, uint32_t f2_hz) {

}
