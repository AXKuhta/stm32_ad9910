#include "stm32f7xx_hal.h"

extern UART_HandleTypeDef usart3;

int _write(int handle, char* data, int size) {
	HAL_UART_Transmit(&usart3, (uint8_t*)data, size, 0xFFFF);

	// Performance
	extern uint32_t perf_usart3_bytes_tx;
	perf_usart3_bytes_tx += size;

	(void)handle;

	return size;
}
