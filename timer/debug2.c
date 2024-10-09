#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

#define TRIGGER_OUT   GPIOE, GPIO_PIN_5
#define SIGNAL_DRAIN  GPIOE, GPIO_PIN_6

// Таймер 9

static void timer9_gpio_init() {
	PIN_AF_Init(TRIGGER_OUT,  GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF3_TIM9); // TIM9_CH1
	PIN_AF_Init(SIGNAL_DRAIN, GPIO_MODE_AF_OD, GPIO_NOPULL, GPIO_AF3_TIM9); // TIM9_CH2
}

static void timer9_init(uint32_t prescaler, uint32_t period, uint32_t pulse) {
	__HAL_RCC_TIM9_CLK_ENABLE();
	timer9_gpio_init();

	TIM_HandleTypeDef timer9_defaults = {
		.Instance = TIM9,
		.Init = {
			.Prescaler = prescaler,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = period,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1
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
	
	HAL_TIM_PWM_Init(&timer9_defaults);
	HAL_TIM_PWM_ConfigChannel(&timer9_defaults, &oc_config, TIM_CHANNEL_1);
	HAL_TIM_PWM_ConfigChannel(&timer9_defaults, &oc_config, TIM_CHANNEL_2);

	HAL_TIM_PWM_Start(&timer9_defaults, TIM_CHANNEL_1);
	HAL_TIM_PWM_Start(&timer9_defaults, TIM_CHANNEL_2);
}

static void timer9_stop() {
	TIM_HandleTypeDef timer9_defaults = {
		.Instance = TIM9
	};

	HAL_TIM_PWM_Stop(&timer9_defaults, TIM_CHANNEL_1);
	HAL_TIM_PWM_Stop(&timer9_defaults, TIM_CHANNEL_2);
}

void debug2_start(double target_hz, double target_t) {
	double desired_freq = 65536 * target_hz;
	uint32_t prescaler = M_216MHz / desired_freq + 0.5;
	double scaled_freq = M_216MHz / (prescaler + 1);
	uint32_t period = scaled_freq / target_hz + 0.5;
	uint32_t pulse = scaled_freq * target_t + 0.5;

	timer9_init(prescaler, period, period - pulse);

	char* tstr = time_unit(pulse / scaled_freq);

	printf("Debug2 enabled: %s pulses at %lf Hz\n", tstr, scaled_freq / period);

	free(tstr);
}
