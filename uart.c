#include "stm32f7xx_hal.h"
#include "definitions.h"

UART_HandleTypeDef huart;

//
// USART3
// PD8  TX
// PD9  RX
// PD10 CK   ???
// PD11 CTS  ???
// PD12 RTS  ???
//
void usart3_gpio_init() {
	GPIO_InitTypeDef TX = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_8, .Alternate = GPIO_AF7_USART3 };
	GPIO_InitTypeDef RX = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_9, .Alternate = GPIO_AF7_USART3 };
	
	// Уже сделано где-нибудь?
	__HAL_RCC_GPIOD_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOD, &TX);
	HAL_GPIO_Init(GPIOD, &RX);
}

void HAL_UART_MspInit(UART_HandleTypeDef *husart) {
	// Просто выйти при попытке инициализировать что-то кроме USART3
	if (husart->Instance != USART3)
		return;
	
	__HAL_RCC_USART3_CLK_ENABLE();
	
	usart3_gpio_init();
}

void uart_init() {
	huart.Instance = USART3;
	
	// Привычные параметры UART
	huart.Init.BaudRate = 9600;
	huart.Init.WordLength = UART_WORDLENGTH_8B;
	huart.Init.StopBits = UART_STOPBITS_1;
	huart.Init.Parity = UART_PARITY_NONE;
	huart.Init.HwFlowCtl = UART_HWCONTROL_NONE;
	huart.Init.Mode = UART_MODE_TX_RX;
	
	// Непривычные параметры UART
	// Эти свойства нулевые
	//huart.Init.OneBitSampling = UART_ONEBIT_SAMPLING_DISABLED;
	//huart.Init.OverSampling = UART_OVERSAMPLING_16;

	HAL_UART_Init(&huart);
	
	// Прерывания
	HAL_NVIC_SetPriority(USART3_IRQn, 0, 0); // Максимальный приоритет
	HAL_NVIC_EnableIRQ(USART3_IRQn);
	
	// Когда в буфере приёма что-то есть
	__HAL_UART_ENABLE_IT(&huart, UART_IT_RXNE);
}

void uart_send(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_UART_Transmit(&huart, buffer, size, 100);
	
	if (status != HAL_OK) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	}
}

void print(char* str) {
	uart_send( (uint8_t*)str, (uint16_t)__builtin_strlen(str) );
}

void print_hex(uint32_t value) {
	char* lut = "0123456789ABCDEF";
	
	print("0x");
	
	if (value > 255) {
		uart_send( (uint8_t*)&lut[(value >> 28) & 0xF], 1 );
		uart_send( (uint8_t*)&lut[(value >> 24) & 0xF], 1 );
		uart_send( (uint8_t*)&lut[(value >> 20) & 0xF], 1 );
		uart_send( (uint8_t*)&lut[(value >> 16) & 0xF], 1 );
		uart_send( (uint8_t*)&lut[(value >> 12) & 0xF], 1 );
		uart_send( (uint8_t*)&lut[(value >> 8) & 0xF], 1 );
	}
	uart_send( (uint8_t*)&lut[(value >> 4) & 0xF], 1 );
	uart_send( (uint8_t*)&lut[(value >> 0) & 0xF], 1 );
}

void print_dec(uint32_t value) {
	char* lut = "0123456789";
	
	uint32_t divd = 1000000000;
	uint32_t d = 0;
	
	while (divd > value)
		divd = divd / 10;
	
	while (divd) {
		d = value / divd;
		value = value % divd;
		
		uart_send( (uint8_t*)&lut[d], 1 );
		divd /= 10;
	}
}

