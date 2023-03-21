#include "stm32f7xx_hal.h"
#include "pin_init.h"

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
	HAL_NVIC_SetPriority(USART3_IRQn, 1, 0);
	HAL_NVIC_EnableIRQ(USART3_IRQn);
}

void usart3_deinit() {
	HAL_NVIC_DisableIRQ(USART3_IRQn);

	__HAL_RCC_USART3_FORCE_RESET();
	__HAL_RCC_USART3_RELEASE_RESET();

	usart3_gpio_deinit();
}
