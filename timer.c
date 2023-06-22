#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

// Выбрано исходя из таблицы "STM32F745xx and STM32F746xx alternate function mapping"
#define EXT_TRIG  		GPIOA, GPIO_PIN_0
#define RADAR_EMULATOR  GPIOE, GPIO_PIN_9
#define MODULATION_DBG	GPIOC, GPIO_PIN_6

TIM_HandleTypeDef timer1;
TIM_HandleTypeDef timer2;
TIM_HandleTypeDef timer8;

// Таймер 1
// Эмулятор радара
void timer1_gpio_init() {
	PIN_AF_Init(RADAR_EMULATOR, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF1_TIM1); // TIM1_CH1
}

// Таймер 2
// EXT_TRIG
void timer2_gpio_init() {
	PIN_AF_Init(EXT_TRIG, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH1
}

// Таймер 5
// Модуляция + отладочный выход
void timer8_gpio_init() {
	PIN_AF_Init(MODULATION_DBG, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF3_TIM8); // TIM8_CH1
}

void timer1_init(uint32_t prescaler, uint32_t period, uint32_t pulse) {
	__HAL_RCC_TIM1_CLK_ENABLE();
	timer1_gpio_init();

	TIM_HandleTypeDef timer1_defaults = {
		.Instance = TIM1,
		.Init = {
			.Prescaler = prescaler,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = period,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};

	timer1 = timer1_defaults;
	
	// PWM1 = Начинает на высоком уровне
	// PWM2 = Начинает на низком уровне
	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM1,
		.Pulse = pulse,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_PWM_Init(&timer1);
	HAL_TIM_PWM_ConfigChannel(&timer1, &oc_config, TIM_CHANNEL_1);
	
	HAL_TIM_PWM_Start(&timer1, TIM_CHANNEL_1);
}

// TIM_TS_TI1FP1
void timer2_init() {
	__HAL_RCC_TIM2_CLK_ENABLE();
	timer2_gpio_init();
	
	TIM_HandleTypeDef timer2_defaults = {
		.Instance = TIM2,
		.Init = {
			.Prescaler = 0,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = 0xFFFFFFFF,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};
	
	timer2 = timer2_defaults;

	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM2,
		.Pulse = 0,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_OC_Init(&timer2);
	HAL_TIM_OC_ConfigChannel(&timer2, &oc_config, TIM_CHANNEL_4);
	HAL_TIM_OC_ConfigChannel(&timer2, &oc_config, TIM_CHANNEL_3);
	
	TIM_SlaveConfigTypeDef slave_config = {
		.SlaveMode = TIM_SLAVEMODE_COMBINED_RESETTRIGGER,
		.InputTrigger = TIM_TS_TI1FP1,
		.TriggerPolarity = TIM_TRIGGERPOLARITY_FALLING
	};
	
	HAL_TIM_SlaveConfigSynchronization(&timer2, &slave_config);

	TIM_MasterConfigTypeDef master_config = {
		.MasterOutputTrigger = TIM_TRGO_OC3REF,
		.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE
	};

	HAL_TIMEx_MasterConfigSynchronization(&timer2, &master_config);
	
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	
	// Запуск требуется даже при настроенном триггере
	HAL_TIM_OC_Start(&timer2, TIM_CHANNEL_3);
	HAL_TIM_OC_Start_IT(&timer2, TIM_CHANNEL_4);
}

void timer2_stop() {	
	__HAL_RCC_TIM2_FORCE_RESET();
}

void timer2_restart() {
	__HAL_RCC_TIM2_RELEASE_RESET();
	timer2_init();
}

//
// Таймер модуляции
//
void timer8_init() {
	__HAL_RCC_TIM8_CLK_ENABLE();
	timer8_gpio_init();

	uint32_t period = 1*1000*1000 * NS_TO_216MHZ_MU + 0.5;
	
	TIM_HandleTypeDef timer8_defaults = {
		.Instance = TIM8,
		.Init = {
			.Prescaler = 0,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = period - 1,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};
	
	timer8 = timer8_defaults;

	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM2,
		.Pulse = period - (20.0 * NS_TO_216MHZ_MU + 0.5),
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_OC_Init(&timer8);
	HAL_TIM_OC_ConfigChannel(&timer8, &oc_config, TIM_CHANNEL_1);

	// Для TIM8, TIM_TS_ITR1 означает триггер от TIM2
	// Это можно найти в таблице "TIMx internal trigger connection" в STM32F746 reference manual
	TIM_SlaveConfigTypeDef slave_config = {
		.SlaveMode = TIM_SLAVEMODE_COMBINED_RESETTRIGGER,
		.InputTrigger = TIM_TS_ITR1,
		.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING
	};
	
	HAL_TIM_SlaveConfigSynchronization(&timer8, &slave_config);
	
	HAL_NVIC_SetPriority(TIM8_UP_TIM13_IRQn, 0, 1);
	HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);

	HAL_TIM_Base_Init(&timer8);
	HAL_TIM_Base_Start(&timer8); // Не спровоцирует запуск, поскольку сконфигурирован мастер
	HAL_TIM_OC_Start(&timer8, TIM_CHANNEL_1);
}

// Изменить период на лету
void timer8_reconfigure(uint32_t period) {
	TIM_Base_InitTypeDef base_config = {
		.Prescaler = 0,
		.CounterMode = TIM_COUNTERMODE_UP,
		.Period = period - 1,
		.ClockDivision = TIM_CLOCKDIVISION_DIV1,
		.RepetitionCounter = 0
	};

	TIM_Base_SetConfig(TIM8, &base_config);
	TIM8->CCR1 = period - (20.0 * NS_TO_216MHZ_MU + 0.5);
}

void timer8_stop() {
	__HAL_RCC_TIM8_FORCE_RESET();
}

void timer8_restart() {
	__HAL_RCC_TIM8_RELEASE_RESET();
	timer8_init();
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
void radar_emulator_start() {
	uint32_t target_hz = 25;
	double target_t = 12.0 / 1000.0 / 1000.0;

	uint32_t timer_freq = 216*1000*1000;
	uint32_t desired_freq = 65536 * target_hz;
	uint32_t prescaler = timer_freq / (double)desired_freq + 0.5;
	double scaled_freq = timer_freq / (prescaler + 1);
	uint32_t period = scaled_freq / (double)target_hz + 0.5;
	uint32_t pulse = scaled_freq * target_t + 0.5;

	timer1_init(prescaler, period, pulse);

	char* tstr = time_unit(target_t);

	printf("Radar emulator enabled: %s pulses at %ld Hz\n", tstr, target_hz);

	free(tstr);
}

uint32_t timer_mu(uint32_t time_ns) {
	return time_ns * NS_TO_216MHZ_MU + 0.5;
}
