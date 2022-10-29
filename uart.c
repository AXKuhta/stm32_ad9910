#include "stm32f7xx_hal.h"

UART_HandleTypeDef usart3;

static void usart3_gpio_init() {
	GPIO_InitTypeDef TX = { .Pin = GPIO_PIN_8, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Alternate = GPIO_AF7_USART3 };
	GPIO_InitTypeDef RX = { .Pin = GPIO_PIN_9, .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_PULLUP, .Speed = GPIO_SPEED_FREQ_VERY_HIGH, .Alternate = GPIO_AF7_USART3 };

	__HAL_RCC_GPIOD_CLK_ENABLE();

	HAL_GPIO_Init(GPIOD, &TX);
	HAL_GPIO_Init(GPIOD, &RX);
}

static void usart3_gpio_deinit() {
	HAL_GPIO_DeInit(GPIOD, GPIO_PIN_8);
	HAL_GPIO_DeInit(GPIOD, GPIO_PIN_9);
}

void usart3_init() {
	__HAL_RCC_USART3_CLK_ENABLE();

	usart3_gpio_init();

	UART_HandleTypeDef usart3_defaults = {
		.Instance = USART3,
		.Init = {
			.BaudRate 		= 9600,
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
}

void usart3_deinit() {
	__HAL_RCC_USART3_FORCE_RESET();
	__HAL_RCC_USART3_RELEASE_RESET();

	usart3_gpio_deinit();
}
