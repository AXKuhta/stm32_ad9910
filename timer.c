#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_hal.h"
#include "timer.h"
#include "units.h"

TIM_HandleTypeDef timer1;
TIM_HandleTypeDef timer2;

// Таймер 1
// Эмулятор радара
void timer1_gpio_init() {
	GPIO_InitTypeDef CH1 = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Pin = GPIO_PIN_9, .Alternate = GPIO_AF1_TIM1 };
	
	__HAL_RCC_GPIOE_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOE, &CH1);
}

// Таймер 2
// Поле Pull определит уровень на ножке, когда таймер сброшен
// На входных каналах стоит использовать GPIO_PULLDOWN
void timer2_gpio_init() {
	GPIO_InitTypeDef CH1 = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Pin = GPIO_PIN_0, .Alternate = GPIO_AF1_TIM2 };
	GPIO_InitTypeDef CH4 = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Pin = GPIO_PIN_11, .Alternate = GPIO_AF1_TIM2 };
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOA, &CH1);
	HAL_GPIO_Init(GPIOB, &CH4);
}

void io_update_timer_controlled() {
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

	GPIO_InitTypeDef CH3 = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Pin = GPIO_PIN_10, .Alternate = GPIO_AF1_TIM2 };

	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_Init(GPIOB, &CH3);
}

void io_update_software_controlled() {
	HAL_GPIO_DeInit(GPIOB, GPIO_PIN_10);

	GPIO_InitTypeDef CH3 = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_PULLDOWN, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Pin = GPIO_PIN_10 };

	__HAL_RCC_GPIOB_CLK_ENABLE();

	HAL_GPIO_Init(GPIOB, &CH3);
}

void timer1_init(uint32_t prescaler, uint32_t period, uint32_t pulse) {
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

	timer1 = timer1_defaults;
	
	// PWM1 = Начинает на высоком уровне
	// PWM2 = Начинает на низком уровне
	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_PWM1,
		.Pulse = pulse,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_PWM_Init(&timer1);
	HAL_TIM_PWM_ConfigChannel(&timer1, &oc_config, TIM_CHANNEL_1);
	
	HAL_TIM_PWM_Start(&timer1, TIM_CHANNEL_1);
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
			.Period = 4294967295,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};
	
	timer2 = timer2_defaults;

	TIM_OC_InitTypeDef oc_config = {
		.OCMode = TIM_OCMODE_TOGGLE,
		.Pulse = 50,
		.OCPolarity = TIM_OCPOLARITY_HIGH,
		.OCFastMode = TIM_OCFAST_DISABLE
	};
	
	HAL_TIM_OC_Init(&timer2);
	HAL_TIM_OC_ConfigChannel(&timer2, &oc_config, TIM_CHANNEL_4);
	
	TIM_SlaveConfigTypeDef slave_config = {
		.SlaveMode = TIM_SLAVEMODE_COMBINED_RESETTRIGGER,
		.InputTrigger = TIM_TS_TI1FP1,
		.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING
	};
	
	HAL_TIM_SlaveConfigSynchronization(&timer2, &slave_config);
	
	HAL_NVIC_SetPriority(TIM2_IRQn, 0, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);
	
	// Запуск требуется даже при настроенном триггере
	HAL_TIM_OC_Start_IT(&timer2, TIM_CHANNEL_4);
}

void timer2_stop() {	
	__HAL_RCC_TIM2_FORCE_RESET();
}

void timer2_restart() {
	__HAL_RCC_TIM2_RELEASE_RESET();
	timer2_init();
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
void radar_emulator_start() {
	uint32_t target_hz = 25;
	double target_t = 12.0 / 1000.0 / 1000.0;

	uint32_t timer_freq = 216*1000*1000;
	uint32_t desired_freq = 65536 * target_hz;
	uint32_t prescaler = timer_freq / (double)desired_freq + 0.5;
	double scaled_freq = timer_freq / (prescaler + 1);
	uint32_t period = scaled_freq / (double)target_hz + 0.5;
	uint32_t pulse = scaled_freq * target_t + 0.5;

	timer1_init(prescaler, period, pulse);

	char* tstr = time_unit(target_t);

	printf("Radar emulator enabled: %s pulses at %ld Hz\n", tstr, target_hz);

	free(tstr);
}

pulse_t timer_pulse(uint32_t delay_ns, uint32_t length_ns) {
	pulse_t pulse;

	if (delay_ns + length_ns > 40*1000*1000) {
		printf("Specified pulse doesn't fit into 25 Hz");
		return;
	}
	
	pulse.t1 = (uint32_t)((double)delay_ns / NANOSEC_108MHZ);
	pulse.t2 = (uint32_t)((double)length_ns / NANOSEC_108MHZ) + pulse.t1;

	return pulse;
}
