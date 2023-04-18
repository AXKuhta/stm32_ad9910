#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"

extern UART_HandleTypeDef usart3;
DMA_HandleTypeDef dma_usart3_rx;

void usart3_rx_dma_init() {
	__HAL_RCC_DMA1_CLK_ENABLE();

	// USART3 = DMA 1, Channel 4, Stream 1
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

	printf("DMA Init\n");

	dma_usart3_rx = dma_usart3_rx_defaults;

	assert(HAL_DMA_Init(&dma_usart3_rx) == HAL_OK);

	__HAL_LINKDMA(&usart3, hdmarx, dma_usart3_rx);

	HAL_NVIC_SetPriority(DMA1_Stream1_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}
