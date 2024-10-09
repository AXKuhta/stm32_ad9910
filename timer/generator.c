#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

// Выбрано исходя из таблицы "STM32F745xx and STM32F746xx alternate function mapping"
#define MODULATION_DBG	GPIOC, GPIO_PIN_6

TIM_HandleTypeDef timer8;

// Таймер 5
// Модуляция + отладочный выход
void timer8_gpio_init() {
	PIN_AF_Init(MODULATION_DBG, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF3_TIM8); // TIM8_CH1
	//PIN_AF_Init(BREAK_INPUT, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF3_TIM8);

	HAL_GPIO_Init(GPIOC, &((GPIO_InitTypeDef) {
		.Mode = GPIO_MODE_AF_PP,
		.Pull = GPIO_NOPULL,
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH,
		.Pin = GPIO_PIN_6,
		.Alternate = GPIO_AF3_TIM8
	}));
}

//
// Таймер модуляции
//
void timer8_init() {
	__HAL_RCC_TIM8_CLK_ENABLE();
	timer8_gpio_init();
	
	TIM_HandleTypeDef timer8_defaults = {
		.Instance = TIM8,
		.Init = {
			.Prescaler = 0,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = 7,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};
	
	timer8 = timer8_defaults;

	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM2,
		.Pulse = 4,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE,
		.OCIdleState = TIM_OCIDLESTATE_RESET,
		.OCNIdleState = TIM_OCIDLESTATE_RESET,
	};
	
	HAL_TIM_OC_Init(&timer8);
	HAL_TIM_OC_ConfigChannel(&timer8, &oc_config, TIM_CHANNEL_1);

	HAL_TIM_Base_Init(&timer8);
	HAL_TIM_Base_Start(&timer8);
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
