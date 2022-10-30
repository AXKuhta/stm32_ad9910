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
#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"

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
extern uint8_t data[16];
/**
	* @brief  Main program
	* @param  None
	* @retval None
	*/
int main(void)
{
	system_init();

	/* Infinite loop */
	while (1) {
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
		isr_recorder_sync();
		print_it();

		data[15] = 0;
		printf("%s\n", data);
	}
}
