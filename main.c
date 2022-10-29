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
#include <stdio.h>
#include <assert.h>
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


/* Private functions ---------------------------------------------------------*/

/**
	* @brief  Main program
	* @param  None
	* @retval None
	*/
int main(void)
{
	system_init();

	printf("** Newlib-nano printf() works! ** \r\n");

	/* Infinite loop */
	while (1)
	{
	}
}
