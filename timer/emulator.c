#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

#define RADAR_EMULATOR  GPIOE, GPIO_PIN_9

// Таймер 1
// Эмулятор радара
static void timer1_gpio_init() {
	PIN_AF_Init(RADAR_EMULATOR, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF1_TIM1); // TIM1_CH1
}

static void timer1_init(uint32_t prescaler, uint32_t period, uint32_t pulse) {
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
	
	// PWM1 = Начинает на высоком уровне
	// PWM2 = Начинает на низком уровне
	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM2,
		.Pulse = pulse,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_PWM_Init(&timer1_defaults);
	HAL_TIM_PWM_ConfigChannel(&timer1_defaults, &oc_config, TIM_CHANNEL_1);
	
	HAL_TIM_PWM_Start(&timer1_defaults, TIM_CHANNEL_1);
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
// # _pulse_ тиков таймера соответствуют желаемому времени заполнения
// # но это должны быть последние _pulse_ тиков
// # так что:
// pulse = period - pulse
//
void radar_emulator_start(double target_hz, double target_t) {
	double desired_freq = 65536 * target_hz;
	uint32_t prescaler = M_216MHz / desired_freq + 0.5;
	double scaled_freq = M_216MHz / (prescaler + 1);
	uint32_t period = scaled_freq / target_hz + 0.5;
	uint32_t pulse = scaled_freq * target_t + 0.5;

	timer1_init(prescaler, period, period - pulse);

	char* tstr = time_unit(pulse / scaled_freq);

	printf("Radar emulator enabled: %s pulses at %lf Hz\n", tstr, scaled_freq / period);

	free(tstr);
}
