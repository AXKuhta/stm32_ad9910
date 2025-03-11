#include <assert.h>
#include <stdio.h>
#include <stdlib.h>

#include "stm32f7xx_ll_tim.h"
#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "timer.h"
#include "units.h"

// В данной файле находится реализация вывода произвольной последовательности логических уровней с привязкой ко времени
// Доступно восемь выходов:
//
// TIM3 CH1		PC6		DR_CTL
// TIM3 CH2		PC7		DRHOLD
// TIM3 CH3		PC8		TXE
// TIM3 CH4		PC9		OSK
// TIM4 CH1		PD12	P_0
// TIM4 CH2		PD13	P_1
// TIM4 CH3		PD14	P_2
// TIM4 CH4		PD15	IO_UPDATE
//
// Триггерный вход:
// TIM2 CH1		A0		"EXT_TRIG"
//
// см. картинки в
// https://os.mbed.com/platforms/ST-Nucleo-F746ZG/
//
// см. таблицу "STM32F745xx and STM32F746xx alternate function mapping"
// https://www.st.com/resource/en/datasheet/stm32f745ie.pdf

#define PC6		GPIOC, GPIO_PIN_6
#define PC7		GPIOC, GPIO_PIN_7
#define PC8		GPIOC, GPIO_PIN_8
#define PC9		GPIOC, GPIO_PIN_9

#define PD12	GPIOD, GPIO_PIN_12
#define PD13	GPIOD, GPIO_PIN_13
#define PD14	GPIOD, GPIO_PIN_14
#define PD15	GPIOD, GPIO_PIN_15

#define A0 		GPIOA, GPIO_PIN_0

TIM_HandleTypeDef master_timer;  // TIM2
TIM_HandleTypeDef slave_timer_a; // TIM3
TIM_HandleTypeDef slave_timer_b; // TIM4

extern DMA_HandleTypeDef dma_slave_timer_a_up;  // Slave A, Перезапись ARR
extern DMA_HandleTypeDef dma_slave_timer_a_cc1; // Slave A, Перезапись CCMR1 и CCMR2

extern DMA_HandleTypeDef dma_slave_timer_b_up;  // Slave B, Перезапись ARR
extern DMA_HandleTypeDef dma_slave_timer_b_cc1; // Slave B, Перезапись CCMR1 и CCMR2

void logic_blaster_init_gpio() {
	PIN_AF_Init(A0, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF1_TIM2); // FIXME - нет необходимости переинициализировать A0

	PIN_AF_Init(PC6, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM3);
	PIN_AF_Init(PC7, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM3);
	PIN_AF_Init(PC8, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM3);
	PIN_AF_Init(PC9, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM3);

	PIN_AF_Init(PD12, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM4);
	PIN_AF_Init(PD13, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM4);
	PIN_AF_Init(PD14, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM4);
	PIN_AF_Init(PD15, GPIO_MODE_AF_PP, GPIO_PULLDOWN, GPIO_AF2_TIM4);
}

void timer_init() {
	__HAL_RCC_TIM2_CLK_ENABLE();
	__HAL_RCC_TIM3_CLK_ENABLE();
	__HAL_RCC_TIM4_CLK_ENABLE();

	// ====================================================================================
	// TIM2 - Master
	// ====================================================================================
	master_timer = (TIM_HandleTypeDef) {
		.Instance = TIM2,
		.Init = {
			.Prescaler = 0,
			.CounterMode = TIM_COUNTERMODE_UP,
			.Period = 0xFFFFFFFF,
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.RepetitionCounter = 0
		}
	};

	// Первый канал ловит триггер
	// Запоминает время получения
	HAL_TIM_IC_Init(&master_timer);
	HAL_TIM_IC_ConfigChannel(&master_timer, &(TIM_IC_InitTypeDef){
		.ICPolarity = TIM_ICPOLARITY_FALLING,
		.ICSelection = TIM_ICSELECTION_DIRECTTI
	}, TIM_CHANNEL_1 );

	// ====================================================================================
	// TIM3 - Slave A
	// ====================================================================================
	slave_timer_a = (TIM_HandleTypeDef) {
		.Instance = TIM3,
		.Init = {
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.CounterMode = TIM_COUNTERMODE_DOWN,
			.Prescaler = 0,
			.Period = 0xFFFF,
			.RepetitionCounter = 0
		}
	};

	// Для TIM3, TIM_TS_ITR1 означает триггер от TIM2
	// см. таблицу "TIMx internal trigger connection" в STM32F746 reference manual (страница 783)
	// https://www.st.com/resource/en/reference_manual/rm0385-stm32f75xxx-and-stm32f74xxx-advanced-armbased-32bit-mcus-stmicroelectronics.pdf
	HAL_TIM_SlaveConfigSynchronization(&slave_timer_a, &(TIM_SlaveConfigTypeDef){
		.SlaveMode = TIM_SLAVEMODE_TRIGGER,
		.InputTrigger = TIM_TS_ITR1,
		.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING
	});

	// ====================================================================================
	// TIM4 - Slave B
	// ====================================================================================
	slave_timer_b = (TIM_HandleTypeDef) {
		.Instance = TIM4,
		.Init = {
			.ClockDivision = TIM_CLOCKDIVISION_DIV1,
			.CounterMode = TIM_COUNTERMODE_DOWN,
			.Prescaler = 0,
			.Period = 0xFFFF,
			.RepetitionCounter = 0
		}
	};

	// Тоже ITR1
	HAL_TIM_SlaveConfigSynchronization(&slave_timer_b, &(TIM_SlaveConfigTypeDef){
		.SlaveMode = TIM_SLAVEMODE_TRIGGER,
		.InputTrigger = TIM_TS_ITR1,
		.TriggerPolarity = TIM_TRIGGERPOLARITY_RISING
	});

	// ====================================================================================
	// Выходы
	// ====================================================================================
	HAL_TIM_OC_Init(&slave_timer_a);
	HAL_TIM_OC_Init(&slave_timer_b);

	// У каналов сравнения есть парамеры:
	// OCMode		Режим
	// Pulse		Значение
	// OCPolarity	Инверсия - не нужно
	// OCFastMode	Пропуск ступеней конвейера - не нужно
	HAL_TIM_OC_ConfigChannel(&slave_timer_a, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_1);
	HAL_TIM_OC_ConfigChannel(&slave_timer_a, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_2);
	HAL_TIM_OC_ConfigChannel(&slave_timer_a, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_3);
	HAL_TIM_OC_ConfigChannel(&slave_timer_a, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_4);

	HAL_TIM_OC_ConfigChannel(&slave_timer_b, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_1);
	HAL_TIM_OC_ConfigChannel(&slave_timer_b, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_2);
	HAL_TIM_OC_ConfigChannel(&slave_timer_b, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_3);
	HAL_TIM_OC_ConfigChannel(&slave_timer_b, &(TIM_OC_InitTypeDef){ .OCMode = TIM_OCMODE_FORCED_INACTIVE, .Pulse = 0 }, TIM_CHANNEL_4);

	// Взаимодействует с очередями FreeRTOS
	// Приоритет должен быть больше или равен configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY
	// FIXME: уже не взаимодействует
	// TODO: вынести все включения прерываний в isr.c
	HAL_NVIC_SetPriority(TIM2_IRQn, 7, 0);
	HAL_NVIC_EnableIRQ(TIM2_IRQn);

	// HAL_NVIC_SetPriority(TIM3_IRQn, 0, 0);
	// HAL_NVIC_EnableIRQ(TIM3_IRQn);

	// Не нужно
	// HAL_NVIC_SetPriority(TIM4_IRQn, 0, 1);
	// HAL_NVIC_EnableIRQ(TIM4_IRQn);
	
	// Запуск требуется даже при настроенном триггере
	//HAL_TIM_OC_Start(&master_timer, TIM_CHANNEL_3);
	//HAL_TIM_OC_Start_IT(&master_timer, TIM_CHANNEL_4);

	// Мастер не настроен - таймер сразу побежит
	// Запуск немного неуклюжий:
	// - IC Start включит канал 1 и enable таймера
	// - Base Start IT включит прерывание по переполнению, повторно enable, выставит статус BUSY
	HAL_TIM_IC_Start(&master_timer, DMA_CHANNEL_1);
	HAL_TIM_Base_Start_IT(&master_timer);

	HAL_TIM_Base_Init(&slave_timer_a);
	HAL_TIM_Base_Init(&slave_timer_b);

	// Не спровоцирует запуск, поскольку сконфигурирован мастер
	HAL_TIM_Base_Start_IT(&slave_timer_a);
	//HAL_TIM_Base_Start_IT(&slave_timer_b);

	// Включить выход на всех каналах
	HAL_TIM_OC_Start(&slave_timer_a, TIM_CHANNEL_1);
	HAL_TIM_OC_Start(&slave_timer_a, TIM_CHANNEL_2);
	HAL_TIM_OC_Start(&slave_timer_a, TIM_CHANNEL_3);
	HAL_TIM_OC_Start(&slave_timer_a, TIM_CHANNEL_4);

	HAL_TIM_OC_Start(&slave_timer_b, TIM_CHANNEL_1);
	HAL_TIM_OC_Start(&slave_timer_b, TIM_CHANNEL_2);
	HAL_TIM_OC_Start(&slave_timer_b, TIM_CHANNEL_3);
	HAL_TIM_OC_Start(&slave_timer_b, TIM_CHANNEL_4);

	__HAL_LINKDMA(&slave_timer_a, hdma[TIM_DMA_ID_UPDATE], dma_slave_timer_a_up);
	__HAL_LINKDMA(&slave_timer_a, hdma[TIM_DMA_ID_CC1], dma_slave_timer_a_cc1);

	__HAL_LINKDMA(&slave_timer_b, hdma[TIM_DMA_ID_UPDATE], dma_slave_timer_b_up);
	__HAL_LINKDMA(&slave_timer_b, hdma[TIM_DMA_ID_CC1], dma_slave_timer_b_cc1);
}

// Разрешить старт Источника Логической Последовательности
void logic_blaster_arm() {
	assert( LL_TIM_IsEnabledCounter(slave_timer_a.Instance) == 0 );
	assert( LL_TIM_IsEnabledCounter(slave_timer_b.Instance) == 0 );

	// Канал выбран как источник триггерного выхода
	// ...в чём разница между TIM_TRGO_OC1 и TIM_TRGO_OC1REF?
	HAL_TIMEx_MasterConfigSynchronization(&master_timer, &(TIM_MasterConfigTypeDef){ .MasterOutputTrigger = TIM_TRGO_OC1 });
}

// Приостановить + запретить старт Источника Логической Последовательности
void logic_blaster_disarm() {
	LL_TIM_DisableCounter(slave_timer_a.Instance);
	LL_TIM_DisableCounter(slave_timer_b.Instance);

	HAL_TIMEx_MasterConfigSynchronization(&master_timer, &(TIM_MasterConfigTypeDef){ .MasterOutputTrigger = TIM_TRGO_RESET });
}

void timer_stop() {	
	__HAL_RCC_TIM2_FORCE_RESET();
	__HAL_RCC_TIM3_FORCE_RESET();
	__HAL_RCC_TIM4_FORCE_RESET();
}

void timer_restart() {
	__HAL_RCC_TIM2_RELEASE_RESET();
	__HAL_RCC_TIM3_RELEASE_RESET();
	__HAL_RCC_TIM4_RELEASE_RESET();
	timer_init();
}
