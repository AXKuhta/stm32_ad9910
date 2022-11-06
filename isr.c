#include <stdio.h>
#include <assert.h>

#include "stm32f7xx_hal.h"

// =============================================================================
// INTERRUPT PROFILER
// =============================================================================
#define LIST_SIZE 8
#define REC_SLOTS 16

typedef struct interrupt_t {
	const char* title;
	uint32_t count;
} interrupt_t;

interrupt_t it_list[LIST_SIZE] = {0};
const char* isr_log[REC_SLOTS] = {0};
int isr_log_next = 0;

void print_it() {
	printf("%10s %s\n", "COUNT", "INTERRUPT");

	for (int i = 0; i < LIST_SIZE; i++) {
		interrupt_t* it = &it_list[i];

		if (it->title == 0) {
			return;
		}

		printf("%10ld %s\n", it->count, it->title);
	}
}

void record_it(const char* title) {
	for (int i = 0; i < LIST_SIZE; i++) {
		interrupt_t* it = &it_list[i];

		if (it->title == title) {
			it->count++;
			return;
		}

		if (it->title == 0) {
			it->title = title;
			it->count = 1;
			return;
		}
	}
}

void isr_recorder_sync() {
	if (isr_log_next > REC_SLOTS) {
		printf("Interrupt recorder overrun: %d interrupts received.\n", isr_log_next);
		while (1) {};
	}

	for (int i = 0; i < isr_log_next; i++) {
		record_it(isr_log[i]);
	}

	isr_log_next = 0;
}

#define RECORD_INTERRUPT() (isr_log[isr_log_next++ % REC_SLOTS] = __FUNCTION__)

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

void USART3_IRQHandler() {
	HAL_UART_IRQHandler(&usart3);
	
	RECORD_INTERRUPT();
}
