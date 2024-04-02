#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifndef STANDALONE_CLI_APP
#include "stm32f7xx_hal.h"
#endif

#include "timer/sequencing.h"
#include "timer.h"
#include "ad9910.h"
#include "sequencer.h"
#include "units.h"
#include "algos.h"
#include "vec.h"

extern uint16_t ad_default_asf;

// "Приватные" глобальные переменные
static vec_t(seq_entry_t)* sequence;
static int seq_index;

void sequencer_init() {
	sequence = init_vec(seq_entry_t);
}

void sequencer_reset() {
	for (size_t i = 0; i < sequence->size; i++) {
		if (sequence->elements[i].ram_image.buffer) {
			free(sequence->elements[i].ram_image.buffer);
			sequence->elements[i].ram_image.buffer = NULL;
		}

		if (sequence->elements[i].profile_modulation.buffer) {
			free(sequence->elements[i].profile_modulation.buffer);
			sequence->elements[i].profile_modulation.buffer = NULL;
		}
	}

	clear_vec(sequence);
}

static void print_sweep(seq_entry_t* entry) {
	printf(" Sweep:\n");

	char* f1 = freq_unit(ad_backconvert_ftw(entry->sweep.lower_ftw));
	char* f2 = freq_unit(ad_backconvert_ftw(entry->sweep.upper_ftw));
	char* fstep = freq_unit(ad_backconvert_ftw(entry->sweep.fstep_ftw));
	char* tstep = time_unit(ad_backconvert_step_time(entry->sweep.tstep));

	printf("\tf1 ftw 0x%08lX (%s)\n", entry->sweep.lower_ftw, f1);
	printf("\tf2 ftw 0x%08lX (%s)\n", entry->sweep.upper_ftw, f2);
	printf("\tfstep ftw 0x%08lX (%s)\n", entry->sweep.fstep_ftw, fstep);
	printf("\ttstep val 0x%04X (%s)\n", entry->sweep.tstep, tstep);

	free(f1);
	free(f2);
	free(fstep);
	free(tstep);
}

static void print_profile(profile_t profile) {
	char* freq = freq_unit(ad_backconvert_ftw(profile.ftw));

	printf("\tftw 0x%08lX (%s)\n", profile.ftw, freq);
	printf("\tpow 0x%04X     (%.3lf deg)\n", profile.pow, ad_backconvert_pow(profile.pow));
	printf("\tasf 0x%04X     (%.3lfx)\n", profile.asf, ad_backconvert_asf(profile.asf));

	free(freq);
}

static void print_ram_image(uint32_t* buffer, size_t size) {
	uint8_t* byte = (uint8_t*)buffer;

	for (size_t i = 0; i < size; i++)
		printf("\t0x%02X 0x%02X 0x%02X 0x%02X\n", byte[4*i], byte[4*i+1], byte[4*i+2], byte[4*i+3]);
}

static void print_profile_modulation_buffer(uint8_t* buffer, size_t size) {
	printf("\t");

	for (size_t i = 0; i < size; i++) {
		printf("%02X ", buffer[i]);

		if ((i & 0b111) == 0b111)
			printf("\n\t");
	}
}

static void debug_print_entry(seq_entry_t* entry) {
	printf(" === seq_entry_t ===\n");

	char* t1_str = time_unit(timer_ns(entry->t1) / 1000.0 / 1000.0 / 1000.0);
	char* t2_str = time_unit(timer_ns(entry->t2) / 1000.0 / 1000.0 / 1000.0);

	printf(" t1: %lu (%s)\n", entry->t1, t1_str);
	printf(" t2: %lu (%s)\n", entry->t2, t2_str);

	free(t1_str);
	free(t2_str);

	if (entry->sweep.fstep_ftw > 0)
		print_sweep(entry);

	if (entry->ram_image.size > 0) {
		printf(" Profiles [RAM]:\n");

		for (int i = 0; i < 8; i++)
			printf(" %d: start %u end %u rate %u mode %u\n", i, entry->ram_profiles[i].start, entry->ram_profiles[i].end, entry->ram_profiles[i].rate, entry->ram_profiles[i].mode);

		printf(" RAM image:\n");
		print_ram_image(entry->ram_image.buffer, entry->ram_image.size);

		printf(" Secondary params:\n");
		print_profile(entry->ram_secondary_params);

	} else {
		printf(" Profiles:\n");

		for (int i = 0; i < 8; i++) {
			printf(" %d:", i);
			print_profile(entry->profiles[i]);
			printf("\n");
		}
	}

	if (entry->profile_modulation.size) {
		printf(" Profile modulation:\n");
		print_profile_modulation_buffer(entry->profile_modulation.buffer, entry->profile_modulation.size);
		printf("\n");
	}

	printf(" ===============\n");
}

void sequencer_show() {
	printf("Sequencer entries: %u\n", sequence->size);
	for_every_entry(sequence, debug_print_entry);
}

void sequencer_add(seq_entry_t entry) {
	vec_push(sequence, entry);
}

#ifndef STANDALONE_CLI_APP

// Глобальные переменные
uint8_t parking_profile = 0;
uint8_t tone_profile = GPIO_PIN_13 >> 8;

extern DMA_HandleTypeDef dma_timer8_up;
extern TIM_HandleTypeDef timer8;
extern TIM_HandleTypeDef timer2;

void pulse_complete_callback() {
	seq_entry_t entry = sequence->elements[seq_index++ % sequence->size];
	uint8_t* profile_mod_buffer;
	size_t profile_mod_size;

	if (entry.profile_modulation.buffer) {
		profile_mod_buffer = entry.profile_modulation.buffer;
		profile_mod_size = entry.profile_modulation.size;

		timer8_reconfigure(entry.profile_modulation.tstep);
	} else {
		profile_mod_buffer = &tone_profile;
		profile_mod_size = 1;
	}

	HAL_DMA_Abort(&dma_timer8_up);

	spi_write_entry(entry);
	ad_safety_off(entry.ram_image.size > 0);

	// Если t1 == 0, то TIM2 CH3 не сможет сгенерировать событие для запуска таймера модуляции
	// Пересесть на TIM2 RESET в таких ситуациях
	if (entry.t1 > 0) {
		timer2_trgo_on_ch3();
	} else {
		timer2_trgo_on_reset();
	}

	// Можно производить запись только в верхнюю часть регистра ODR, сдвинув адрес на 1
	// Если DMA был настроен в режим DMA_CIRCULAR, то модуляция будет идти по кругу
	HAL_DMA_Start(&dma_timer8_up, (uint32_t)profile_mod_buffer, (uint32_t)&GPIOD->ODR + 1, profile_mod_size);
	__HAL_TIM_ENABLE_DMA(&timer8, TIM_DMA_UPDATE);

	// Принудительно закинуть в таймер очень большое значение, чтобы он случайно не пересёк те точки, которые мы вот вот запишем
	timer2.Instance->CNT = 0x7FFFFFFF;
	timer2.Instance->CCR3 = entry.t1;
	timer2.Instance->CCR4 = entry.t2;
}

void sequencer_run() {
	enter_rfkill_mode();

	seq_index = 0;

	// Полный сброс + активация таймеров
	timer2_restart();
	timer8_restart();

	// А что произойдёт, если внешний триггер придёт между timer2_restart() и pulse_complete_callback()?
	pulse_complete_callback();
}

void spi_write_entry(seq_entry_t entry) {
	if (entry.sweep.fstep_ftw > 0) {
		ad_enable_ramp();

		ad_set_ramp_limits(entry.sweep.lower_ftw, entry.sweep.upper_ftw);
		ad_set_ramp_step(0, entry.sweep.fstep_ftw);
		ad_set_ramp_rate(entry.sweep.tstep, entry.sweep.tstep);
	} else {
		ad_disable_ramp();
	}

	// Выбор формата профилей происходит на основе присутствия/отсутствия образа оперативной памяти
	if (entry.ram_image.size > 0) {

		// Портит профили, должно быть выполнено до записи профилей
		ad_write_ram(entry.ram_image.buffer, entry.ram_image.size); 

		for (int i = 0; i < 8; i++) {
			ad_set_ram_profile(i, entry.ram_profiles[i].rate, entry.ram_profiles[i].start, entry.ram_profiles[i].end, entry.ram_profiles[i].mode);
		}

		ad_set_ram_destination(entry.ram_destination);
		ad_set_secondary_freq(entry.ram_secondary_params.ftw);
		ad_set_secondary_amplitude(entry.ram_secondary_params.asf);
		ad_set_secondary_phase(entry.ram_secondary_params.pow);
	} else {
		for (int i = 0; i < 8; i++) {
			ad_set_profile_freq(i, entry.profiles[i].ftw);
			ad_set_profile_amplitude(i, entry.profiles[i].asf);
			ad_set_profile_phase(i, entry.profiles[i].pow);
		}
	}

	ad_write_all();
	set_ramp_direction(0);
	ad_pulse_io_update(); // !! Большая задержка
	set_ramp_direction(1);
}

void sequencer_stop() {
	enter_rfkill_mode();
}

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	timer2_stop();
	timer8_stop();

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

	ad_set_profile_freq(1, ad_calc_ftw(freq_hz));
	ad_set_profile_amplitude(1, ad_default_asf);
	ad_write_all();
	ad_safety_off(0);
	set_profile(1);
}

#endif
