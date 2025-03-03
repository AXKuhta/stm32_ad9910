#include <stdint.h>
#include <assert.h>

#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "ad9910/pins.h"
#include "timer/sequencing.h"

// Профили
#define P_0 	GPIOD, GPIO_PIN_13
#define P_1 	GPIOD, GPIO_PIN_12
#define P_2 	GPIOD, GPIO_PIN_14

// Управляющие сигналы
#define IO_UPDATE 	GPIOB, GPIO_PIN_0
#define IO_RESET 	GPIOG, GPIO_PIN_9
#define DR_CTL 		GPIOB, GPIO_PIN_11
#define DR_HOLD 	GPIOB, GPIO_PIN_10

// Установить профиль
void set_profile(uint8_t profile_id) {
	assert(profile_id < 8);

	HAL_GPIO_WritePin(P_0, profile_id & 0b001 ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_1, profile_id & 0b010 ? GPIO_PIN_SET : GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_2, profile_id & 0b100 ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void ad_init_gpio() {
	PIN_Init(P_0);
	PIN_Init(P_1);
	PIN_Init(P_2);

	PIN_Init(IO_UPDATE);
	PIN_Init(IO_RESET);

	PIN_Init(DR_CTL);
	PIN_Init(DR_HOLD);
}

// Передать управление входами AD9910 процессору (или DMA)
void ad_slave_to_arm() {
	ad_init_gpio();
}

// Передать управление входами AD9910 таймерам
// TODO: пусть AD9910 всегда управляется таймерами
void ad_slave_to_tim() {
	logic_blaster_init_gpio();
}

// Установить направление хода Digital Ramp генератора
// 1 = отсчитывать вверх
// 0 = отсчитывать вниз
// Вероятно, будет использоваться только режим отсчёта вверх
void set_ramp_direction(uint8_t direction) {
	HAL_GPIO_WritePin(DR_CTL, direction ? GPIO_PIN_SET : GPIO_PIN_RESET);
}

void my_delay(uint32_t delay) {
	for (uint32_t i=0; i<delay; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
}

// Подержать IO_RESET высоким 1 мс
void ad_pulse_io_reset() {
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_RESET);
}	

// Подержать IO_UPDATE высоким 1мс
void ad_pulse_io_update() {
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_SET);
	HAL_Delay(1);
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_RESET);
}
