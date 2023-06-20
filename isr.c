#include <stdio.h>
#include <assert.h>

#include "stm32f7xx_hal.h"
#include "ad9910.h"
#include "tasks.h"

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

void SVC_Handler(void) {
}

void DebugMon_Handler(void) {
}

void PendSV_Handler(void) {
}

void SysTick_Handler(void) {
	HAL_IncTick();
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

static void modulation_step() {

}

void TIM2_IRQHandler() {
	static int pulse_t1_pass;

	if (pulse_t1_pass == 0) {
		pulse_t1_pass = 1;									// 1. Выставить t1_pass
		TIM8->CCR1 = TIM8->ARR - 20;						// 2. Включить отладочный выход таймера модуляции
	} else {
		__HAL_TIM_DISABLE_DMA(&timer8, TIM_DMA_UPDATE);		// 1. Убрать источник DMA request-ов
		TIM8->CR1 &= ~(TIM_CR1_CEN);						// 2. Поставить таймер на паузу -- почему-то просто __HAL_TIM_DISABLE(&timer8) не рабоает
		HAL_DMA_Abort_IT(&dma_timer8_up); 					// 3. Сброс DMA [Можно убрать куда-нибудь?]
		pulse_t1_pass = 0;									// 4. Сбросить t1_pass
		set_profile(parking_profile);						// 5. Выставить нулевой профиль принудительно
		add_task(pulse_complete_callback);					// 6. Запланировать запись параметров следующего импульса
		TIM8->CCR1 = 0xFFFF;								// 7. Заглушить отладочный выход таймера модуляции
	}

	HAL_TIM_IRQHandler(&timer2);
	RECORD_INTERRUPT();
}

void TIM8_UP_TIM13_IRQHandler() {
	modulation_step();

	HAL_TIM_IRQHandler(&timer8);
	RECORD_INTERRUPT();
}


