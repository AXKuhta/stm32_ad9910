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

uint16_t dma_buf[20] = {0};

void DMA2_Stream1_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_timer8_up);

	//printf("DMA2 State: %u Error %u NDTR %u ODR %u\n", HAL_DMA_GetState(&dma_timer8_up), HAL_DMA_GetError(&dma_timer8_up), DMA2_Stream1->NDTR, GPIOD->ODR);

	RECORD_INTERRUPT();
}

extern uint8_t parking_profile;
extern uint8_t* profile_mod_buffer;
extern size_t profile_mod_size;
extern size_t profile_mod_idx;
extern void pulse_complete_callback();

static void modulation_step() {
	uint8_t profile_id = profile_mod_buffer[profile_mod_idx];
	uint16_t value = (profile_id & 0b001 ? GPIO_PIN_13 : 0) +
					 (profile_id & 0b010 ? GPIO_PIN_12 : 0) +
					 (profile_id & 0b100 ? GPIO_PIN_11 : 0);

	dma_buf[profile_mod_idx] = value;
	profile_mod_idx++;

	// Модуляция будет идти по кругу
	if (profile_mod_idx == profile_mod_size)
		profile_mod_idx = 0;
}

void first_modulation_step() {
	profile_mod_idx = 0;
	modulation_step();
}

void TIM2_IRQHandler() {
	static int pulse_t1_pass;

	if (pulse_t1_pass == 0) {
		HAL_NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);				// 1. Включить прерывание модуляции
		pulse_t1_pass = 1;									// 2. Выставить t1_pass
		TIM8->CCR1 = TIM8->ARR - 20;						// 3. Включить отладочный выход таймера модуляции
	} else {
		__HAL_TIM_DISABLE_DMA(&timer8, TIM_DMA_UPDATE);		// 1. Убрать источник DMA request-ов
		TIM8->CR1 &= ~(TIM_CR1_CEN);						// 2. Поставить таймер на паузу -- почему-то просто __HAL_TIM_DISABLE(&timer8) не рабоает
		HAL_DMA_Abort_IT(&dma_timer8_up); 					// 3. Сброс DMA [Можно убрать куда-нибудь?]
		HAL_NVIC_DisableIRQ(TIM8_UP_TIM13_IRQn);			// 4. Выключить прерывание (Правда это не поможет, если оно уже pending => можно и не выключать?)
		pulse_t1_pass = 0;									// 6. Сбросить t1_pass
		set_profile(parking_profile);						// 7. Выставить нулевой профиль принудительно
		add_task(pulse_complete_callback);					// 8. Запланировать запись параметров следующего импульса
		TIM8->CCR1 = 0xFFFF;								// 9. Заглушить отладочный выход таймера модуляции
	}

	HAL_TIM_IRQHandler(&timer2);
	RECORD_INTERRUPT();
}

void TIM8_UP_TIM13_IRQHandler() {
	modulation_step();

	HAL_TIM_IRQHandler(&timer8);
	RECORD_INTERRUPT();
}


