#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

// Выбрано исходя из таблицы "STM32F745xx and STM32F746xx alternate function mapping"
#define EXT_TRIG  		GPIOA, GPIO_PIN_0

TIM_HandleTypeDef timer2;

// Таймер 2
// EXT_TRIG
void timer2_gpio_init() {
	PIN_AF_Init(EXT_TRIG, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH1
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
