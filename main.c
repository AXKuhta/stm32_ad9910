/**
	******************************************************************************
	* @file    UART/UART_Printf/Src/main.c
	* @author  MCD Application Team
	* @brief   This example shows how to retarget the C library printf function
	*          to the UART.
	******************************************************************************
	* @attention
	*
	* Copyright (c) 2016 STMicroelectronics.
	* All rights reserved.
	*
	* This software is licensed under terms that can be found in the LICENSE file
	* in the root directory of this software component.
	* If no LICENSE file comes with this software, it is provided AS-IS.
	*
	******************************************************************************
	*/

/* Includes ------------------------------------------------------------------*/
#include "main.h"
#include "init.h"

/** @addtogroup STM32F7xx_HAL_Examples
	* @{
	*/

/** @addtogroup UART_Printf
	* @{
	*/

/* Private typedef -----------------------------------------------------------*/
/* Private define ------------------------------------------------------------*/
/* Private macro -------------------------------------------------------------*/
/* Private variables ---------------------------------------------------------*/
/* UART handler declaration */
UART_HandleTypeDef usart3;

/* Private function prototypes -----------------------------------------------*/
static void Error_Handler(void);

/* Private functions ---------------------------------------------------------*/

/**
	* @brief  Main program
	* @param  None
	* @retval None
	*/
int main(void)
{
	/*##-1- Configure the UART peripheral ######################################*/
	/* Put the USART peripheral in the Asynchronous mode (UART Mode) */
	/* UART configured as follows:
			- Word Length = 8 Bits (7 data bit + 1 parity bit) : 
										BE CAREFUL : Program 7 data bits + 1 parity bit in PC HyperTerminal
			- Stop Bit    = One Stop bit
			- Parity      = ODD parity
			- BaudRate    = 9600 baud
			- Hardware flow control disabled (RTS and CTS signals) */
	usart3.Instance        = USARTx;

	usart3.Init.BaudRate   = 9600;
	usart3.Init.WordLength = UART_WORDLENGTH_8B;
	usart3.Init.StopBits   = UART_STOPBITS_1;
	usart3.Init.Parity     = UART_PARITY_NONE;
	usart3.Init.HwFlowCtl  = UART_HWCONTROL_NONE;
	usart3.Init.Mode       = UART_MODE_TX_RX;
	usart3.Init.OverSampling = UART_OVERSAMPLING_16;
	if (HAL_UART_Init(&usart3) != HAL_OK)
	{
		/* Initialization Error */
		Error_Handler();
	}

	printf("** Newlib-nano printf() works! ** \r\n");

	/* Infinite loop */
	while (1)
	{
	}
}

/**
	* @brief  This function is executed in case of error occurrence.
	* @param  None
	* @retval None
	*/
static void Error_Handler(void)
{
	while (1)
	{
	}
}

#ifdef  USE_FULL_ASSERT
/**
	* @brief  Reports the name of the source file and the source line number
	*         where the assert_param error has occurred.
	* @param  file: pointer to the source file name
	* @param  line: assert_param error line source number
	* @retval None
	*/
void assert_failed(uint8_t *file, uint32_t line)
{
	/* User can add his own implementation to report the file name and line number,
		 ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */

	/* Infinite loop */
	while (1)
	{
	}
}
#endif

/**
	* @}
	*/

/**
	* @}
	*/

