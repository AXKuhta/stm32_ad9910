#include <stdio.h>
#include <assert.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "ad9910.h"

// =============================================================================
// INTERRUPT PROFILER
// =============================================================================
#define LIST_SIZE 64
#define REC_SLOTS 16

typedef struct interrupt_t {
	const char* title;
	uint32_t count;
} interrupt_t;

interrupt_t it_list[LIST_SIZE] = {0};
int isr_recorder_collision = 0;

static void record_it(const char* title) {
	size_t ptr = (size_t)title;
	size_t idx = ptr % LIST_SIZE;

	interrupt_t* it = &it_list[idx];

	if (it->title == 0) {
		it->title = title;
	} else if (it->title != title) {
		isr_recorder_collision = 1;
		return;
	}

	it->count++;
}

void print_it() {
	printf("%10s %s\n", "COUNT", "INTERRUPT");

	for (int i = 0; i < LIST_SIZE; i++) {
		interrupt_t* it = &it_list[i];

		if (it->title == 0) {
			continue;
		}

		printf("%10ld %s\n", it->count, it->title);
	}
}

void isr_recorder_sync() {
	if (isr_recorder_collision) {
		printf("Interrupt recorder index collision");
		while (1) {};
	}
}

#define RECORD_INTERRUPT() record_it(__FUNCTION__)

// =============================================================================
// PROCESSOR EXCEPTIONS
// =============================================================================

void NMI_Handler(void) {
}

void HardFault_Handler(void)
{
	/* Go to infinite loop when Hard Fault exception occurs */
	while (1) {
	}
}

void MemManage_Handler(void) {
	/* Go to infinite loop when Memory Manage exception occurs */
	while (1) {
	}
}

void BusFault_Handler(void) {
	/* Go to infinite loop when Bus Fault exception occurs */
	while (1) {
	}
}

void UsageFault_Handler(void) {
	/* Go to infinite loop when Usage Fault exception occurs */
	while (1) {
	}
}

// FreeRTOS scheduler
void vPortSVCHandler();
void xPortPendSVHandler();
void xPortSysTickHandler();

void SVC_Handler(void) {
	vPortSVCHandler();

	RECORD_INTERRUPT();
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
	xPortPendSVHandler();

	RECORD_INTERRUPT();
}

void SysTick_Handler(void) {
	HAL_IncTick();
	xPortSysTickHandler();

	RECORD_INTERRUPT();
}

// =============================================================================
// DEVICE INTERRUPTS
// =============================================================================

extern UART_HandleTypeDef usart3;
extern DMA_HandleTypeDef dma_usart3_rx;
extern DMA_HandleTypeDef dma_timer8_up;
extern TIM_HandleTypeDef timer2;
extern TIM_HandleTypeDef timer8;

void USART3_IRQHandler() {
	HAL_UART_IRQHandler(&usart3);
	
	RECORD_INTERRUPT();
}

void DMA1_Stream1_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_usart3_rx);

	RECORD_INTERRUPT();
}

void DMA2_Stream1_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_timer8_up);

	//printf("DMA2 State: %u Error %u NDTR %u ODR %u\n", HAL_DMA_GetState(&dma_timer8_up), HAL_DMA_GetError(&dma_timer8_up), DMA2_Stream1->NDTR, GPIOD->ODR);

	RECORD_INTERRUPT();
}

extern uint8_t parking_profile;
extern void pulse_complete_callback();

void TIM2_IRQHandler() {
	__HAL_TIM_DISABLE_DMA(&timer8, TIM_DMA_UPDATE);		// 1. Убрать источник DMA request-ов
	TIM8->CR1 &= ~(TIM_CR1_CEN);						// 2. Поставить таймер на паузу -- почему-то просто __HAL_TIM_DISABLE(&timer8) не рабоает
	TIM8->CNT = 0;										// 3. И занулить его, чтобы он случайно не застрял на значении выше CCR1
	set_profile(parking_profile);						// 4. Выставить нулевой профиль принудительно
	run_later(pulse_complete_callback);					// 5. Запланировать запись параметров следующего импульса

	HAL_TIM_IRQHandler(&timer2);
	RECORD_INTERRUPT();
}

void TIM8_UP_TIM13_IRQHandler() {
	HAL_TIM_IRQHandler(&timer8);
	RECORD_INTERRUPT();
}

extern ETH_HandleTypeDef xETH;

void ETH_IRQHandler( void )
{
    HAL_ETH_IRQHandler( &xETH );

	RECORD_INTERRUPT();
}


