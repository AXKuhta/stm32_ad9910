#include "stm32f7xx_hal.h"
#include "definitions.h"

extern UART_HandleTypeDef huart;

void Error_Handler(void) {
	
}

void SysTick_Handler(void) {
	HAL_IncTick();
}

void EXTI15_10_IRQHandler() {
	HAL_GPIO_EXTI_IRQHandler(GPIO_PIN_13); // Занять прерывание EXTI15_10 экслюзивно под кнопку на GPIO C13
}

void tap(void);

static char buf[256] = {0};
static int i = 0;

void tap() {		
	buf[i] = 0;
	print(buf);
	i = 0;
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
}

void USART3_IRQHandler() {
	//HAL_UART_IRQHandler(&husart);
	HAL_NVIC_ClearPendingIRQ(USART3_IRQn);
	
	// RDR является FIFO на 4 байта?
	// Читать его таким образом не совсем правильно
	// При большом количестве данных, некоторые байты могут быть утеряны
	char symbol = (char)huart.Instance->RDR;
	
	buf[i] = symbol;
	i++;
	
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
	
	if (i > 255) {
		HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
		i = 0;
	}
	
	switch (symbol) {
		case 'v':
			set_next_task(ad_readback_all);
			break;
		case 'r':
			set_next_task(ad_write_all);
			break;
		
		default:
			break;
	}
	
	//set_next_task(tap);
	
}
