#include <stdint.h>
#include <assert.h>

#include "stm32f7xx_ll_gpio.h"
#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "ad9910/pins.h"

// Профили
#define P_0 	GPIOD, GPIO_PIN_13
#define P_1 	GPIOD, GPIO_PIN_12
#define P_2 	GPIOD, GPIO_PIN_11

// Управляющие сигналы
#define IO_UPDATE 	GPIOB, GPIO_PIN_0
#define IO_RESET 	GPIOG, GPIO_PIN_9
#define DR_CTL 		GPIOB, GPIO_PIN_11
#define DR_HOLD 	GPIOB, GPIO_PIN_10

// Перевести номер профиля в значение, пригодное для записи в регистр ODR
uint16_t profile_to_gpio_states(uint8_t profile_id) {
	return 	(profile_id & 0b001 ? GPIO_PIN_13 : 0) +
			(profile_id & 0b010 ? GPIO_PIN_12 : 0) +
			(profile_id & 0b100 ? GPIO_PIN_11 : 0);
}

// Установить профиль
void set_profile(uint8_t profile_id) {
	assert(profile_id < 8);

	LL_GPIO_WriteOutputPort(GPIOD, profile_to_gpio_states(profile_id));
}

static void init_profile_gpio() {
	PIN_Init(P_0);
	PIN_Init(P_1);
	PIN_Init(P_2);
	
	HAL_GPIO_WritePin(P_0, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_1, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(P_2, GPIO_PIN_RESET);
}

static void init_control_gpio() {
	PIN_Init(IO_UPDATE);
	PIN_Init(IO_RESET);
	
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_RESET);
}

void drctl_software_controlled() {
	HAL_GPIO_DeInit(DR_CTL);
	PIN_Init(DR_CTL);
}

void drhold_software_controlled() {
	HAL_GPIO_DeInit(DR_HOLD);
	PIN_Init(DR_HOLD);
}

void drctl_timer_controlled() {
	HAL_GPIO_DeInit(DR_CTL);
	PIN_AF_Init(DR_CTL, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH4
}

void drhold_timer_controlled() {
	HAL_GPIO_DeInit(DR_HOLD);
	PIN_AF_Init(DR_HOLD, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // TIM2_CH3
}

void ad_init_gpio() {
	init_profile_gpio();
	init_control_gpio();

	drctl_software_controlled();
	drhold_software_controlled();
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

// Хоть какая-то задержка должна присутствовать
// Без неё импульса вообще не возникнет
void ad_pulse_io_reset() {
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_SET);
	//HAL_Delay(1);
	HAL_GPIO_WritePin(IO_RESET, GPIO_PIN_RESET);
	//HAL_Delay(1);
}	

void ad_pulse_io_update() {
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_SET);
	//HAL_Delay(1);
	HAL_GPIO_WritePin(IO_UPDATE, GPIO_PIN_RESET);
}
