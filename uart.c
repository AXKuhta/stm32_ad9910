#include <assert.h>
#include <stdio.h>
#include <string.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "pin_init.h"
#include "syscalls.h"
#include "uart_cli.h"

#define USART3_TX 	GPIOD, GPIO_PIN_8
#define USART3_RX 	GPIOD, GPIO_PIN_9

UART_HandleTypeDef usart3;

static void usart3_gpio_init() {
	PIN_AF_Init(USART3_TX, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART3);
	PIN_AF_Init(USART3_RX, GPIO_MODE_AF_PP, GPIO_PULLUP, GPIO_AF7_USART3);
}

static void usart3_gpio_deinit() {
	HAL_GPIO_DeInit(USART3_TX);
	HAL_GPIO_DeInit(USART3_RX);
}

void usart3_init() {
	__HAL_RCC_USART3_CLK_ENABLE();

	usart3_gpio_init();

	UART_HandleTypeDef usart3_defaults = {
		.Instance = USART3,
		.Init = {
			.BaudRate 		= 115200,
			.WordLength 	= UART_WORDLENGTH_8B,
			.StopBits 		= UART_STOPBITS_1,
			.Parity 		= UART_PARITY_NONE,
			.HwFlowCtl 		= UART_HWCONTROL_NONE,
			.Mode 			= UART_MODE_TX_RX,
			.OverSampling 	= UART_OVERSAMPLING_16
		}
	};

	usart3 = usart3_defaults;
	
	if (HAL_UART_Init(&usart3) != HAL_OK) {
		while (1) {};
	}

	// This interrupt may be used to clear UART error flags
	HAL_NVIC_SetPriority(USART3_IRQn, 8, 0);
	HAL_NVIC_EnableIRQ(USART3_IRQn);
}

void usart3_deinit() {
	HAL_NVIC_DisableIRQ(USART3_IRQn);

	__HAL_RCC_USART3_FORCE_RESET();
	__HAL_RCC_USART3_RELEASE_RESET();

	usart3_gpio_deinit();
}

// =============================================================================
// INPUT HANDLING
// =============================================================================
#define INPUT_BUFFER_SIZE 1024
uint8_t input_buffer[INPUT_BUFFER_SIZE] __ALIGNED(32) = {0};

void restart_rx() {
	memset(input_buffer, 0, INPUT_BUFFER_SIZE);
	_write(1, "\r> ", 3);
	assert(HAL_UARTEx_ReceiveToIdle_DMA(&usart3, input_buffer, INPUT_BUFFER_SIZE - 1) == HAL_OK);
}

void uart_cli_init() {
	printf("Entering CLI\n");
	restart_rx();
}

const char* get_next_str(const char* buf) {
	while (*buf) {
		// CRLF
		if (*buf == '\r' && *(buf + 1) == '\n')
			return buf + 1;

		// LF
		if (*buf == '\n')
			return buf + 1;

		buf++;
	}

	return NULL;
}

void parse() {
	const char* buf = (const char*)input_buffer;

	do {
		run(buf);
		buf = get_next_str(buf);
	} while (buf);

	restart_rx();
}

void input_overrun_error() {
	printf("\nInput overrun error\n");
	while (1) {};
}

void HAL_UART_ErrorCallback(UART_HandleTypeDef *huart) {
	(void)huart;

	run_later(input_overrun_error);
}

// This function is called when the line goes idle
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	uint16_t space_remains = huart->RxXferCount;
	uint8_t* base = huart->pRxBuffPtr;
	uint8_t* last = base - 1 + Size;

	// Must do this or it will look to ARM like the memory is still 0
	// ...Except when input_buffer is in the first 64KB of SRAM which is uncached
	SCB_InvalidateDCache_by_Addr((uint32_t*)input_buffer, INPUT_BUFFER_SIZE);

	// Performance
	extern uint32_t perf_usart3_bytes_rx;
	perf_usart3_bytes_rx += Size;

	// Echo keypresses
	_write(1, (char*)base, Size);

	// Backspace handling
	// Different terminals may send different backspace codes
	// Handle both types
	if (*last == 127) {
		_write(1, (char[]){8, ' ', 8}, 3);
		*last = 8;
	}

	if (*last == 8) {
		if (last == input_buffer) {
			return restart_rx();
		}

		*last = 0;
		last -= 2;

		space_remains++;
		base = last;
	}

	if (space_remains == 0) {
		return run_later(input_overrun_error);
	}

	// Ensure that memory will remain untouched while parsing
	if (*last == '\n' || *last == '\r') {
		_write(1, "\n", 1);
		run_later(parse);
	} else {
		HAL_UARTEx_ReceiveToIdle_DMA(huart, base + Size, space_remains);
	}	
}
