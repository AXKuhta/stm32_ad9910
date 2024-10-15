#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

// Выбрано исходя из таблицы "STM32F745xx and STM32F746xx alternate function mapping"
#define EXT_TRIG  		GPIOA, GPIO_PIN_0
#define MODULATION_DBG	GPIOC, GPIO_PIN_6

TIM_HandleTypeDef timer2;
TIM_HandleTypeDef timer8;

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
	
	HAL_NVIC_SetPriority(TIM2_IRQn, 7, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	
	// Запуск требуется даже при настроенном триггере
	HAL_TIM_OC_Start(&timer2, TIM_CHANNEL_3);
	HAL_TIM_OC_Start_IT(&timer2, TIM_CHANNEL_4);
}

// Обычный режим, отступ > 0
void timer2_trgo_on_ch3() {
	HAL_TIMEx_MasterConfigSynchronization(&timer2, &(TIM_MasterConfigTypeDef){ .MasterOutputTrigger = TIM_TRGO_OC3REF });
}

// Особый режим для ситуаций, когда отступ = 0
void timer2_trgo_on_reset() {
	HAL_TIMEx_MasterConfigSynchronization(&timer2, &(TIM_MasterConfigTypeDef){ .MasterOutputTrigger = TIM_TRGO_RESET });
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

	uint32_t period = 1*1000*1000 * ns_to_machine_units_factor() + 0.5;
	
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
		.Pulse = period - (20.0 * ns_to_machine_units_factor() + 0.5),
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
	TIM8->CCR1 = period - (20.0 * ns_to_machine_units_factor() + 0.5);
}

void timer8_stop() {
	__HAL_RCC_TIM8_FORCE_RESET();
}

void timer8_restart() {
	__HAL_RCC_TIM8_RELEASE_RESET();
	timer8_init();
}
