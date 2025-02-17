#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"

// TODO: Вынести __HAL_LINKDMA в usart.c
extern UART_HandleTypeDef usart3;

DMA_HandleTypeDef dma_usart3_rx;

DMA_HandleTypeDef dma_slave_timer_a_up;  // Slave A, Перезапись ARR
DMA_HandleTypeDef dma_slave_timer_a_cc1; // Slave A, Перезапись CCMR1 и CCMR2

DMA_HandleTypeDef dma_slave_timer_b_up;  // Slave B, Перезапись ARR
DMA_HandleTypeDef dma_slave_timer_b_cc1; // Slave B, Перезапись CCMR1 и CCMR2

// На DMA1 занято 5 из 8 движков:
// USART3 = DMA1, Channel 4, Stream 1
// TIM3 UP  DMA1, Channel 5, Stream 2
// TIM3 CH1 DMA1, Channel 5, Stream 4
// TIM4 UP  DMA1, Channel 2, Stream 6
// TIM4 CH1 DMA1, Channel 2, Stream 0
//
// Движки 3, 5, 7 свободны и доступны для доп. действий по событиям от TIM3/TIM4
//
// Если отказаться от перестройки hold time, то можно освободить движки 2, 6
//
// Взято из таблицы "DMA1 request mapping" в STM32F746 reference manual
//
static void usart3_rx_dma_init() {
	DMA_HandleTypeDef dma_usart3_rx_defaults = {
		.Instance = DMA1_Stream1,
		.Init = {
			.Channel = DMA_CHANNEL_4,
			.Direction = DMA_PERIPH_TO_MEMORY,
			.PeriphInc = DMA_PINC_DISABLE,
			.MemInc = DMA_MINC_ENABLE,
			.PeriphDataAlignment = DMA_PDATAALIGN_BYTE,
			.MemDataAlignment = DMA_MDATAALIGN_BYTE,
			.Mode = DMA_NORMAL,
			.Priority = DMA_PRIORITY_LOW,
			.FIFOMode = DMA_FIFOMODE_DISABLE,
			.FIFOThreshold = DMA_FIFO_THRESHOLD_1QUARTERFULL,
			.MemBurst = DMA_MBURST_SINGLE,
			.PeriphBurst = DMA_PBURST_SINGLE
		}
	};

	dma_usart3_rx = dma_usart3_rx_defaults;

	assert(HAL_DMA_Init(&dma_usart3_rx) == HAL_OK);

	__HAL_LINKDMA(&usart3, hdmarx, dma_usart3_rx);

	// Это прерывание может вызывать HAL_UART_ErrorCallback, который, в свою очередь, может взаимодействовать с очередями FreeRTOS
	// Так что оставить здесь низкий приоритет
	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 7, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

static void logic_level_blaster_dma_init() {
	dma_slave_timer_a_up = (DMA_HandleTypeDef) {
		.Instance = DMA1_Stream2,
		.Init = {
			.Channel 				= DMA_CHANNEL_5,
			.Direction 				= DMA_MEMORY_TO_PERIPH,
			.PeriphInc 				= DMA_PINC_DISABLE,
			.MemInc 				= DMA_MINC_ENABLE,
			.PeriphDataAlignment 	= DMA_PDATAALIGN_HALFWORD,
			.MemDataAlignment 		= DMA_MDATAALIGN_HALFWORD,
			.Mode 					= DMA_NORMAL,
			.Priority 				= DMA_PRIORITY_LOW,
			.FIFOMode 				= DMA_FIFOMODE_DISABLE,
			.FIFOThreshold 			= DMA_FIFO_THRESHOLD_1QUARTERFULL,
			.MemBurst 				= DMA_MBURST_SINGLE,
			.PeriphBurst 			= DMA_PBURST_SINGLE
		}
	};

	dma_slave_timer_a_cc1 = (DMA_HandleTypeDef) {
		.Instance = DMA1_Stream4,
		.Init = {
			.Channel 				= DMA_CHANNEL_5,
			.Direction 				= DMA_MEMORY_TO_PERIPH,
			.PeriphInc 				= DMA_PINC_DISABLE,
			.MemInc 				= DMA_MINC_ENABLE,
			.PeriphDataAlignment 	= DMA_PDATAALIGN_WORD,
			.MemDataAlignment 		= DMA_MDATAALIGN_WORD,
			.Mode 					= DMA_NORMAL,
			.Priority 				= DMA_PRIORITY_LOW,
			.FIFOMode 				= DMA_FIFOMODE_DISABLE,
			.FIFOThreshold 			= DMA_FIFO_THRESHOLD_1QUARTERFULL,
			.MemBurst 				= DMA_MBURST_SINGLE,
			.PeriphBurst 			= DMA_PBURST_SINGLE
		}
	};

	dma_slave_timer_b_up = (DMA_HandleTypeDef) {
		.Instance = DMA1_Stream6,
		.Init = {
			.Channel 				= DMA_CHANNEL_2,
			.Direction 				= DMA_MEMORY_TO_PERIPH,
			.PeriphInc 				= DMA_PINC_DISABLE,
			.MemInc 				= DMA_MINC_ENABLE,
			.PeriphDataAlignment 	= DMA_PDATAALIGN_HALFWORD,
			.MemDataAlignment 		= DMA_MDATAALIGN_HALFWORD,
			.Mode 					= DMA_NORMAL,
			.Priority 				= DMA_PRIORITY_LOW,
			.FIFOMode 				= DMA_FIFOMODE_DISABLE,
			.FIFOThreshold 			= DMA_FIFO_THRESHOLD_1QUARTERFULL,
			.MemBurst 				= DMA_MBURST_SINGLE,
			.PeriphBurst 			= DMA_PBURST_SINGLE
		}
	};

	dma_slave_timer_b_cc1 = (DMA_HandleTypeDef) {
		.Instance = DMA1_Stream0,
		.Init = {
			.Channel 				= DMA_CHANNEL_2,
			.Direction 				= DMA_MEMORY_TO_PERIPH,
			.PeriphInc 				= DMA_PINC_DISABLE,
			.MemInc 				= DMA_MINC_ENABLE,
			.PeriphDataAlignment 	= DMA_PDATAALIGN_WORD,
			.MemDataAlignment 		= DMA_MDATAALIGN_WORD,
			.Mode 					= DMA_NORMAL,
			.Priority 				= DMA_PRIORITY_LOW,
			.FIFOMode 				= DMA_FIFOMODE_DISABLE,
			.FIFOThreshold 			= DMA_FIFO_THRESHOLD_1QUARTERFULL,
			.MemBurst 				= DMA_MBURST_SINGLE,
			.PeriphBurst 			= DMA_PBURST_SINGLE
		}
	};

	assert(HAL_DMA_Init(&dma_slave_timer_a_up) == HAL_OK);
	assert(HAL_DMA_Init(&dma_slave_timer_a_cc1) == HAL_OK);

	assert(HAL_DMA_Init(&dma_slave_timer_b_up) == HAL_OK);
	assert(HAL_DMA_Init(&dma_slave_timer_b_cc1) == HAL_OK);

	// FIXME: вынести в isr.c
	HAL_NVIC_SetPriority(DMA1_Stream2_IRQn, 7, 0);
	HAL_NVIC_SetPriority(DMA1_Stream4_IRQn, 7, 0);
	HAL_NVIC_SetPriority(DMA1_Stream6_IRQn, 7, 0);
	HAL_NVIC_SetPriority(DMA1_Stream0_IRQn, 7, 0);

	HAL_NVIC_EnableIRQ(DMA1_Stream2_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Stream4_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Stream6_IRQn);
	HAL_NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void dma_init() {
	printf("DMA Init\n");

	__HAL_RCC_DMA1_CLK_ENABLE();

	usart3_rx_dma_init();
	logic_level_blaster_dma_init();
}
