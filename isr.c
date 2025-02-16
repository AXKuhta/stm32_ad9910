#include <stdio.h>
#include <assert.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "ad9910.h"

// =============================================================================
// INTERRUPT PROFILER
// =============================================================================
#define LIST_SIZE 256

typedef struct interrupt_t {
	const char* title;
	uint32_t count;
} interrupt_t;

interrupt_t it_list[LIST_SIZE] = {0};
int isr_recorder_collision = 0;

static void record_it(const char* title) {
	size_t ptr = (size_t)title;
	size_t idx = (ptr >> 2) % LIST_SIZE;

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
		printf("Interrupt recorder index collision\n");
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
//void vPortSVCHandler();
//void xPortPendSVHandler();
void xPortSysTickHandler();

// void SVC_Handler(void) {
// 	vPortSVCHandler();

// 	RECORD_INTERRUPT();
// }

void DebugMon_Handler(void) {
}

// void PendSV_Handler(void) {
// 	xPortPendSVHandler();

// 	RECORD_INTERRUPT();
// }

void SysTick_Handler(void) {
	HAL_IncTick();
	xPortSysTickHandler();

	RECORD_INTERRUPT();
}

// =============================================================================
// DEVICE INTERRUPTS
// =============================================================================

extern UART_HandleTypeDef usart3;

extern TIM_HandleTypeDef master_timer;
extern TIM_HandleTypeDef slave_timer_a;
extern TIM_HandleTypeDef slave_timer_b;

extern DMA_HandleTypeDef dma_usart3_rx;

extern DMA_HandleTypeDef dma_slave_timer_a_up;
extern DMA_HandleTypeDef dma_slave_timer_a_cc1;

extern DMA_HandleTypeDef dma_slave_timer_b_up;
extern DMA_HandleTypeDef dma_slave_timer_b_cc1;

void USART3_IRQHandler() {
	HAL_UART_IRQHandler(&usart3);
	RECORD_INTERRUPT();
}

void DMA1_Stream1_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_usart3_rx);
	RECORD_INTERRUPT();
}

void DMA1_Stream2_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_slave_timer_a_up);
	RECORD_INTERRUPT();
}

void DMA1_Stream4_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_slave_timer_a_cc1);
	RECORD_INTERRUPT();
}

void DMA1_Stream6_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_slave_timer_b_up);
	RECORD_INTERRUPT();
}

void DMA1_Stream0_IRQHandler() {
	HAL_DMA_IRQHandler(&dma_slave_timer_b_cc1);
	RECORD_INTERRUPT();
}

extern uint8_t parking_profile;
extern void pulse_complete_callback();

int idx = 0;

// Конец излучения
void TIM2_IRQHandler() {
	set_profile(parking_profile);						// 4. Выставить нулевой профиль принудительно
	run_later(pulse_complete_callback);					// 5. Запланировать запись параметров следующего импульса

	HAL_TIM_IRQHandler(&master_timer);

	// В __HAL_TIM_DISABLE проверка на отключение каналов, только нам это зачем??
	slave_timer_a.Instance->CR1 &= ~(TIM_CR1_CEN);
	slave_timer_b.Instance->CR1 &= ~(TIM_CR1_CEN);

	idx = 0;

	RECORD_INTERRUPT();
}

void ETH_IRQHandler_real(void);

void ETH_IRQHandler() {
	ETH_IRQHandler_real();
	RECORD_INTERRUPT();
}

extern void timer14_schedule_stop();

void TIM8_TRG_COM_TIM14_IRQHandler() {
	timer14_schedule_stop();

	HAL_TIM_IRQHandler(&(TIM_HandleTypeDef){ .Instance = TIM14 });

	RECORD_INTERRUPT();
}

// ccmr1 bit fields
// 0000000v 0000000u 0vvv0000 0uuu0000
// ch2+     ch1+     ch2      ch1
//
// ccmr2 bit fields
// 0000000y 0000000x 0yyy0000 0xxx0000
// ch4+     ch3+     ch4      ch3
//
// uuuu
// 0000 frozen
// 0001 set active on match
// 0010 set inactive on match
// 0011 toggle
// 0100 force inactive
// 0101 force active
// 0110 active while CNT < OC (PWM1)
// 0111 active while CNT > OC (PWM2)
// 1... extra modes, see reference manual pg. 799
