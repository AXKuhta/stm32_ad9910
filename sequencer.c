#include <stdint.h>
#include <stdlib.h>
#include <assert.h>
#include <string.h>
#include <stdio.h>

#ifndef STANDALONE_CLI_APP
#include "stm32f7xx_hal.h"
#endif

#include "leds.h"
#include "timer/sequencing.h"
#include "timer.h"
#include "ad9910.h"
#include "sequencer.h"
#include "events.h"
#include "units.h"
#include "algos.h"
#include "vec.h"

extern uint16_t ad_default_asf;
extern uint8_t ad_default_fsc;

// "Приватные" глобальные переменные
static vec_t(seq_entry_t)* sequence;
static int seq_index;

void sequencer_init() {
	sequence = init_vec(seq_entry_t);
}

// Остановить и очистить секвенсор
void sequencer_reset() {
	sequencer_stop();

	// Здесь фактически деструктор для seq_entry_t
	// Высвобождение всей памяти
	for (size_t i = 0; i < sequence->size; i++) {
		if (sequence->elements[i].ram_image.buffer) {
			free(sequence->elements[i].ram_image.buffer);
			sequence->elements[i].ram_image.buffer = NULL;
		}

		if (sequence->elements[i].logic_level_sequence.count) {
			free(sequence->elements[i].logic_level_sequence.slave_a_stream);
			free(sequence->elements[i].logic_level_sequence.slave_b_stream);
			free(sequence->elements[i].logic_level_sequence.hold_time);

			sequence->elements[i].logic_level_sequence.slave_a_stream = NULL;
			sequence->elements[i].logic_level_sequence.slave_b_stream = NULL;
			sequence->elements[i].logic_level_sequence.hold_time = NULL;
		}
	}

	// Пока динамический массив наполнялся, он мог быть несколько раз перевыделен
	// следовательно он переместился дальше в памяти и, после высвобождения всего
	// прочего, остался посредине кучи, создавая фрагментацию
	//
	// Чтобы противодействовать фрагментации следует высвободить его, и выделить повторно
	free_vec(sequence);
	sequencer_init();
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

	// TODO: вывод информации о logic level sequence исп. print_profile_modulation_buffer

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

extern TIM_HandleTypeDef master_timer;
extern TIM_HandleTypeDef slave_timer_a;
extern TIM_HandleTypeDef slave_timer_b;

extern DMA_HandleTypeDef dma_slave_timer_a_up;  // Slave A, Перезапись ARR
extern DMA_HandleTypeDef dma_slave_timer_a_cc1;	// Slave A, Перезапись CCMR1 и CCMR2

extern DMA_HandleTypeDef dma_slave_timer_b_up;  // Slave B, Перезапись ARR
extern DMA_HandleTypeDef dma_slave_timer_b_cc1; // Slave B, Перезапись CCMR1 и CCMR2

// Важно для выставления статуса READY в структурках HAL
static void dma_abort() {
	HAL_DMA_Abort(&dma_slave_timer_a_up);
	HAL_DMA_Abort(&dma_slave_timer_b_up);

	// DMA запрос таймера может быть зависшим в поднятом состоянии
	// Приведёт к тому, что сразу после HAL_DMA_Start произойдёт транзакция
	// Отключение запроса снимает запрос
	__HAL_TIM_DISABLE_DMA(&slave_timer_a, TIM_DMA_UPDATE);
	__HAL_TIM_DISABLE_DMA(&slave_timer_b, TIM_DMA_UPDATE);

	HAL_TIM_DMABurst_WriteStop(&slave_timer_a, TIM_DMA_CC1);
	HAL_TIM_DMABurst_WriteStop(&slave_timer_b, TIM_DMA_CC1);
}

void pulse_complete_callback() {
	seq_entry_t entry = sequence->elements[seq_index++ % sequence->size];

	logic_blaster_disarm();

	// FIXME: Logic Level Sequence empty
	dma_abort();

	spi_write_entry(entry);
	ad_safety_off(entry.ram_image.size > 0);

	// Конфигурация генератора последовательностей логических уровней
	assert(HAL_DMA_Start(&dma_slave_timer_a_up, (uint32_t)entry.logic_level_sequence.hold_time, (uint32_t)&slave_timer_a.Instance->ARR, entry.logic_level_sequence.count) == HAL_OK);
	assert(HAL_DMA_Start(&dma_slave_timer_b_up, (uint32_t)entry.logic_level_sequence.hold_time, (uint32_t)&slave_timer_b.Instance->ARR, entry.logic_level_sequence.count) == HAL_OK);

	// FIXME: MultiWriteStart внутри использует HAL_DMA_Start_IT, но нам не нужны прерывания
	assert(HAL_TIM_DMABurst_MultiWriteStart(
		&slave_timer_a,
		TIM_DMABASE_CCMR1,
		TIM_DMA_CC1,
		(void*)entry.logic_level_sequence.slave_a_stream,
		TIM_DMABURSTLENGTH_2TRANSFERS,
		2*entry.logic_level_sequence.count
	) == HAL_OK);

	assert(HAL_TIM_DMABurst_MultiWriteStart(
		&slave_timer_b,
		TIM_DMABASE_CCMR1,
		TIM_DMA_CC1,
		(void*)entry.logic_level_sequence.slave_b_stream,
		TIM_DMABURSTLENGTH_2TRANSFERS,
		2*entry.logic_level_sequence.count
	) == HAL_OK);

	__HAL_TIM_ENABLE_DMA(&slave_timer_a, TIM_DMA_UPDATE);
	__HAL_TIM_ENABLE_DMA(&slave_timer_b, TIM_DMA_UPDATE);

	// Прайминг
	HAL_TIM_GenerateEvent(&slave_timer_a, TIM_EVENTSOURCE_CC1);
	HAL_TIM_GenerateEvent(&slave_timer_b, TIM_EVENTSOURCE_CC1);
	HAL_TIM_GenerateEvent(&slave_timer_a, TIM_EVENTSOURCE_UPDATE);
	HAL_TIM_GenerateEvent(&slave_timer_b, TIM_EVENTSOURCE_UPDATE);

	// Костыль чтобы избежать одного пустого периода
	// От доп. задержки в один такт таймера не избавиться
	slave_timer_a.Instance->CNT = 1;
	slave_timer_b.Instance->CNT = 1;

	ad_slave_to_tim();

	logic_blaster_arm();

	event_queue_push((event_t) { .origin = READY_EVENT, .timestamp = logic_blaster_hrtime() });
}

void sequencer_run() {
	enter_rfkill_mode();

	seq_index = 0;

	if (sequence->size == 0) {
		printf("Sequencer is empty\n");
		return;
	}

	led_arm_set();

	// Полный сброс + активация таймеров
	timer_restart();

	pulse_complete_callback();
}

void spi_write_entry(seq_entry_t entry) {
	ad_slave_to_arm();

	// Настроить DRG (если используется ЛЧМ)
	// Т.к. включен static reset DRG, он начнёт отсчитывать только после:
	// - вызова ad_safety_off() и затем...
	// - ...принятия изменений от смены профиля
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

	ad_set_full_scale_current(entry.fsc);

	// FIXME: Почему ему нужен *фронт* а не уровень DR_CTL? Не понимаю...
	ad_write_all();
	set_ramp_direction(0);
	ad_pulse_io_update();
	set_ramp_direction(1);
}

void sequencer_stop() {
	enter_rfkill_mode();
}

// Прекратить подачу сигналов
void enter_rfkill_mode() {
	timer_stop();

	ad_slave_to_arm();
	ad_set_profile_freq(0, 0);
	ad_set_profile_amplitude(0, 0);
	ad_disable_ramp();
	ad_write_all();
	ad_pulse_io_update();

	set_profile(0);

	led_arm_reset();
}

// Подавать непрерывный сигнал
void enter_test_tone_mode(uint32_t freq_hz) {
	enter_rfkill_mode();

	led_arm_set();

	ad_set_profile_freq(1, ad_calc_ftw(freq_hz));
	ad_set_profile_amplitude(1, ad_default_asf);
	ad_set_full_scale_current(ad_default_fsc);
	ad_write_all();
	ad_safety_off(0);
	set_profile(1);
}

#endif
