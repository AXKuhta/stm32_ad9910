#include "stm32f7xx_hal.h"
#include "definitions.h"

void HAL_GPIO_EXTI_Callback(uint16_t GPIO_Pin) {
	if (GPIO_Pin == GPIO_PIN_13) {
		if (HAL_GPIO_ReadPin(GPIOC, GPIO_PIN_13) == 1) {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_SET);
		} else {
			HAL_GPIO_WritePin(GPIOB, GPIO_PIN_7, GPIO_PIN_RESET);
		}
	}
}

void button_init() {
	GPIO_InitTypeDef B1 = { .Mode = GPIO_MODE_IT_RISING_FALLING, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_13 };

	__HAL_RCC_GPIOC_CLK_ENABLE();

	HAL_GPIO_Init(GPIOC, &B1);
	HAL_NVIC_SetPriority(EXTI15_10_IRQn, 0, 0); // Максимальный приоритет
	HAL_NVIC_EnableIRQ(EXTI15_10_IRQn);
}
