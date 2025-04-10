
#include "stm32f7xx_hal.h"
#include "pin_init.h"

#define PB8		GPIOB, GPIO_PIN_8 // Arm indicator
#define PA6     GPIOA, GPIO_PIN_6 // Power indicator

#define LED_ARM 	PB8
#define LED_POWER 	PA6

void leds_init() {
	PIN_Init(LED_ARM);
	PIN_Init(LED_POWER);

	HAL_GPIO_WritePin(LED_ARM, 0);
	HAL_GPIO_WritePin(LED_POWER, 0);
}

void led_power_set() {
	HAL_GPIO_WritePin(LED_POWER, 1);
}

void led_arm_set() {
	HAL_GPIO_WritePin(LED_ARM, 1);
}

void led_arm_reset() {
	HAL_GPIO_WritePin(LED_ARM, 0);
}
