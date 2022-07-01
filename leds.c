#include "stm32f7xx_hal.h"
#include "definitions.h"

void led_init() {
	GPIO_InitTypeDef LD1 = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_0 };
	GPIO_InitTypeDef LD2 = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_7 };
	GPIO_InitTypeDef LD3 = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_14 };

	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_Init(GPIOB, &LD1);
	HAL_GPIO_Init(GPIOB, &LD2);
	HAL_GPIO_Init(GPIOB, &LD3);
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

