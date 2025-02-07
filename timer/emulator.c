#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

#define RADAR_EMULATOR  GPIOF, GPIO_PIN_9

// Таймер 1
// TIM14_CH1
// Эмулятор радара
static void timer14_gpio_init() {
	PIN_AF_Init(RADAR_EMULATOR, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF9_TIM14);
}

int pulses_remain = 0;

void timer14_schedule_stop() {
	// Остановить таймер по истечению RepetitionCounter
	// Будет лежать в preload регистре, пока
	// RepetitionCounter > 0
	if (pulses_remain == 1)
		TIM14->ARR = 0;

	pulses_remain--;
}

// Инициализировать и запустить внутренний генератор импульсов
// Когда limit = 0 импульсы следуют бесконечно
// Когда limit = x, следует x импульсов
static void timer14_init(uint32_t prescaler, uint32_t period, uint32_t pulse, int limit) {
	__HAL_RCC_TIM14_CLK_ENABLE();
	timer14_gpio_init();

	// Не используем RepetitionCounter т.к. у General Purpose таймеров у него слишком низкое максимальное значение
	// Вместо этого через прерывания считаем, сколько было подано импульсов
	TIM_HandleTypeDef timer14_defaults = {
		.Instance = TIM14,
		.Init = {
			.Prescaler = prescaler,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = period,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_ENABLE
		}
	};
	
	// PWM1 = Начинает на высоком уровне
	// PWM2 = Начинает на низком уровне
	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM2,
		.Pulse = pulse,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_PWM_Init(&timer14_defaults);
	HAL_TIM_PWM_ConfigChannel(&timer14_defaults, &oc_config, TIM_CHANNEL_1);

	// Спровоцировать update event, чтобы значение из preload перенеслось в shadow регистры
	timer14_defaults.Instance->EGR = TIM_EGR_UG;

	pulses_remain = limit;

	timer14_schedule_stop();

	HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);

	if (limit) {
		HAL_TIM_PWM_Start_IT(&timer14_defaults, TIM_CHANNEL_1);
	} else {
		HAL_TIM_PWM_Start(&timer14_defaults, TIM_CHANNEL_1);
	}
	
}

static void timer14_stop() {
	HAL_TIM_PWM_Stop_IT(&(TIM_HandleTypeDef){ .Instance = TIM14 }, TIM_CHANNEL_1);
}

//
// Данная функция настроит TIM1 для имитации внешнего управляющего сигнала от радара (12 мкс, 25 Гц)
// Вычисление параметров таймера выглядит следующим образом:
//
// timer_freq = 216 MHz
// timer_bits = 16
// target_hz = 25 Hz
// target_t = 12 us
//
// desired_freq = 2^timer_bits * target_hz
// prescaler = round(timer_freq / desired_freq)
// scaled_freq = timer_freq / (prescaler + 1)
// period = round(scaled_freq / target_hz)
// pulse = round(scaled_freq * target_t)
//
// actual_hz = scaled_freq / period = 25.00015625097657 Hz
// actual_t = pulse / scaled_freq = 11.699074074074074 us
//
// prescaler = 132
// period = 64962
// pulse = 19
//
// # _pulse_ тиков таймера соответствуют желаемому времени заполнения
// # но это должны быть последние _pulse_ тиков
// # так что:
// pulse = period - pulse
//
void radar_emulator_start(double target_hz, double target_t, uint16_t limit) {
	double desired_freq = 65536 * target_hz;
	uint32_t prescaler = (double)HAL_RCC_GetSysClockFreq() / desired_freq + 0.5;
	double scaled_freq = (double)HAL_RCC_GetSysClockFreq() / (prescaler + 1);
	uint32_t period = scaled_freq / target_hz + 0.5;
	uint32_t pulse = scaled_freq * target_t + 0.5;

	timer14_init(prescaler, period, period - pulse, limit);

	char* tstr = time_unit(pulse / scaled_freq);

	printf("Radar emulator enabled: %s pulses at %lf Hz\n", tstr, scaled_freq / period);

	free(tstr);
}

void radar_emulator_stop() {
	timer14_stop();

	printf("Radar emulator stopped\n");
}
