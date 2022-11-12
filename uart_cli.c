#include <stdio.h>
#include <assert.h>
#include <string.h>

#include "stm32f7xx_hal.h"
#include "tasks.h"
#include "isr.h"
#include "syscalls.h"
#include "performance.h"
#include "ad9910.h"
#include "sequencer.h"

// =============================================================================
// CLI COMMANDS
// =============================================================================
void run(const char* str) {
	char cmd[128] = {0};

	int rc = sscanf(str, "%127s", cmd);

	if (rc == 0) {
		printf("Invalid str\n");
		return;
	}

	if (strcmp(cmd, "isr") == 0) return print_it();
	if (strcmp(cmd, "perf") == 0) return print_perf();
	if (strcmp(cmd, "verify") == 0) return ad_readback_all();
	if (strcmp(cmd, "write") == 0) return ad_write_all();
	if (strcmp(cmd, "rfkill") == 0) return enter_rfkill_mode();
	if (strcmp(cmd, "test_tone") == 0) return enter_test_tone_mode(15*1000*1000);

	printf("Unknown command: [%s]\n", cmd);
}

// =============================================================================
// INPUT HANDLING
// =============================================================================
extern UART_HandleTypeDef usart3;

#define INPUT_BUFFER_SIZE 128
uint8_t input_buffer[INPUT_BUFFER_SIZE] = {0};

void restart_rx() {
	memset(input_buffer, 0, INPUT_BUFFER_SIZE);
	_write(0, "\r> ", 3);
	assert(HAL_UARTEx_ReceiveToIdle_IT(&usart3, input_buffer, INPUT_BUFFER_SIZE - 1) == HAL_OK);
}

void uart_cli_init() {
	printf("Entering CLI\n");
	restart_rx();
}

const char* get_next_str(const char* buf) {
	while (*buf) {
		if (*buf == '\n' || *buf == '\r') return buf + 1;
		buf++;
	}

	return NULL;
}

void parse() {
	const char* buf = input_buffer;

	do {
		run(buf);
		buf = get_next_str(buf);
	} while (*buf != 0);

	restart_rx();
}

void input_overrun_error() {
	printf("\nInput overrun error\n");
	while (1) {};
}

void HAL_UARTEx_RxEventCallback(UART_HandleTypeDef *huart, uint16_t Size) {
	uint8_t* offset_buffer = huart->pRxBuffPtr;
	uint16_t space_remains = huart->RxXferCount;
	uint8_t* last = offset_buffer - 1;

	// Echo keypresses
	_write(0, (char*)offset_buffer - Size, Size);

	// Performance
	extern uint32_t perf_usart3_bytes_rx;
	perf_usart3_bytes_rx += Size;

	// Different terminals may send different backspace codes
	// Handle both types
	if (*last == 8 || *last == 127) {
		*last = 0;
		last--;

		if (last >= input_buffer) {
			*last = 0;

			offset_buffer--;
			space_remains++;
		} else {
			return restart_rx();
		}

		offset_buffer--;
		space_remains++;
	}

	if (space_remains == 0) {
		return add_task(input_overrun_error);
	}

	// Ensure that memory will remain untouched while parsing
	if (*last == '\n' || *last == '\r') {
		_write(0, "\n", 1);
		add_task(parse);
	} else {
		HAL_UARTEx_ReceiveToIdle_IT(huart, offset_buffer, space_remains);
	}	
}

// Restart RX continuously
void HAL_UART_RxCpltCallback(UART_HandleTypeDef *huart) {
	HAL_UARTEx_ReceiveToIdle_IT(huart, input_buffer, INPUT_BUFFER_SIZE - 1);
}
