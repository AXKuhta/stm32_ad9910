#include <stdio.h>
#include <assert.h>

#include "stm32f7xx_hal.h"

// =============================================================================
// INPUT HANDLING
// =============================================================================
extern UART_HandleTypeDef usart3;

#define INPUT_BUFFER_SIZE 128
uint8_t input_buffer[INPUT_BUFFER_SIZE] = {0};

void uart_cli_init() {
	printf("Entering CLI\n");

	//assert(HAL_UART_Receive_IT(&usart3, input_buffer, INPUT_BUFFER_SIZE) == HAL_OK);
	assert(HAL_UARTEx_ReceiveToIdle_IT(&usart3, input_buffer, INPUT_BUFFER_SIZE - 1) == HAL_OK);

	printf("UART now running RTI\n");
}

// Echo keypresses
void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	uint8_t* offset_buffer = huart->pRxBuffPtr;
	uint16_t space_remains = huart->RxXferCount;
	uint8_t* last = offset_buffer - 1;

	// Different terminals may send different backspace codes
	// Handle both types
	if (*last == 8 || *last == 127) {
		*last = 0;
		last--;

		if (last >= input_buffer) {
			*last = 0;

			 offset_buffer--;
			 space_remains++;
		}

		offset_buffer--;
		space_remains++;
	}

	if (space_remains == 0) {
		offset_buffer = input_buffer;
		space_remains = INPUT_BUFFER_SIZE - 1;
		memset(input_buffer, 0, INPUT_BUFFER_SIZE);
	}

	HAL_UARTEx_ReceiveToIdle_IT(&usart3, offset_buffer, space_remains);
}

// Restart RX continuously
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_UARTEx_ReceiveToIdle_IT(&usart3, input_buffer, INPUT_BUFFER_SIZE - 1);
}

// =============================================================================
// CLI COMMANDS
// =============================================================================
