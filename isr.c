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

void USART3_IRQHandler() {
	HAL_UART_IRQHandler(&usart3);
	
	RECORD_INTERRUPT();
}

void DMA1_Stream1_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_usart3_rx);

	RECORD_INTERRUPT();
}

extern TIM_HandleTypeDef timer2;
extern TIM_HandleTypeDef timer5;
extern uint8_t* profile_mod_buffer;
extern size_t profile_mod_size;
extern size_t profile_mod_idx;
extern void pulse_complete_callback();

static void modulation_step() {
	set_profile(profile_mod_buffer[profile_mod_idx]);
	profile_mod_idx++;

	// Модуляция будет идти по кругу
	if (profile_mod_idx == profile_mod_size)
		profile_mod_idx = 0;
}

void TIM2_IRQHandler() {
	static int pulse_t1_pass;

	if (pulse_t1_pass == 0) {
		profile_mod_idx = 0;
		timer5.Instance->CNT = 0; // Принудительно закинуть в таймер 0 для синхронизации
		set_profile(profile_mod_buffer[0]);
		HAL_NVIC_EnableIRQ(TIM5_IRQn);
		pulse_t1_pass = 1;
	} else {
		HAL_NVIC_DisableIRQ(TIM5_IRQn); // Может ли произойти такое, что если прерывание было pending, то обработчик прерывания будет вызван после отключения прерывания?
		set_profile(0);
		pulse_t1_pass = 0;
		add_task(pulse_complete_callback);
	}

	HAL_TIM_IRQHandler(&timer2);
	RECORD_INTERRUPT();
}

void TIM5_IRQHandler() {
	modulation_step();

	HAL_TIM_IRQHandler(&timer5);
	RECORD_INTERRUPT();
}
