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

static int pulses_remain = 0;

// Поддерживающая функция когда внутренний генератор работает в режиме конечного количества импульсов
void timer14_schedule_stop() {
	if (pulses_remain > 1) {
		pulses_remain--;
	} else { // Остался один импульс?
		// Остановить таймер при следующем переполнении
		// Новый ARR будет лежать в preload регистре
		// затем применен в момент переполнения
		TIM14->ARR = 0;

		// Удостовериться, что прерывание не сработает на последнем переполнении
		__HAL_TIM_DISABLE_IT(&(TIM_HandleTypeDef){ .Instance = TIM14 }, TIM_IT_UPDATE);
	}
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

	// - Очистить CNT
	// - Отключить генерацию прерываний
	__HAL_RCC_TIM14_FORCE_RESET();
	__HAL_RCC_TIM14_RELEASE_RESET();

	// Примечание:
	// В рамках HAL_TIM_PWM_Init произойдёт update event, так что значение из preload попадёт в shadow без дополнительных действий
	HAL_TIM_PWM_Init(&timer14_defaults);
	HAL_TIM_PWM_ConfigChannel(&timer14_defaults, &oc_config, TIM_CHANNEL_1);

	// TODO: Вынести конфигурацию всех прерываний в isr.c
	// Проще будет рассуждать о приоритетах обработчиков
	HAL_NVIC_EnableIRQ(TIM8_TRG_COM_TIM14_IRQn);
	HAL_NVIC_SetPriority(TIM8_TRG_COM_TIM14_IRQn, 0, 0);

	if (limit) {
		pulses_remain = limit;

		// - Включить прерывание по переполнению (CNT == ARR)
		__HAL_TIM_CLEAR_IT(&timer14_defaults, TIM_IT_UPDATE); // у нас выставлен UIF после update event - нужно его снять
		__HAL_TIM_ENABLE_IT(&timer14_defaults, TIM_IT_UPDATE); // <--- иначе прямо с этой строки попадём в прерывание

		timer14_schedule_stop();
	}
	
	HAL_TIM_PWM_Start(&timer14_defaults, TIM_CHANNEL_1);
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
