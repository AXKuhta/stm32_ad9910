#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "uart.h"

/**
	* @brief  Configure the MPU attributes
	* @param  None
	* @retval None
	*/
void MPU_Config(void){
	MPU_Region_InitTypeDef MPU_InitStruct;

	/* Disable the MPU */
	HAL_MPU_Disable();

	/* Configure the MPU as Strongly ordered for not defined regions */
	MPU_InitStruct.Enable = MPU_REGION_ENABLE;
	MPU_InitStruct.BaseAddress = 0x00;
	MPU_InitStruct.Size = MPU_REGION_SIZE_4GB;
	MPU_InitStruct.AccessPermission = MPU_REGION_NO_ACCESS;
	MPU_InitStruct.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE;
	MPU_InitStruct.IsCacheable = MPU_ACCESS_NOT_CACHEABLE;
	MPU_InitStruct.IsShareable = MPU_ACCESS_SHAREABLE;
	MPU_InitStruct.Number = MPU_REGION_NUMBER0;
	MPU_InitStruct.TypeExtField = MPU_TEX_LEVEL0;
	MPU_InitStruct.SubRegionDisable = 0x87;
	MPU_InitStruct.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE;

	HAL_MPU_ConfigRegion(&MPU_InitStruct);

	/* Enable the MPU */
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

/**
	* @brief  CPU L1-Cache enable.
	* @param  None
	* @retval None
	*/
void CPU_CACHE_Enable(void)
{
	/* Enable I-Cache */
	SCB_EnableICache();

	/* Enable D-Cache */
	SCB_EnableDCache();
}

/**
	* @brief  System Clock Configuration
	*         The system Clock is configured as follow : 
	*            System Clock source            = PLL (HSE)
	*            SYSCLK(Hz)                     = 216000000
	*            HCLK(Hz)                       = 216000000
	*            AHB Prescaler                  = 1
	*            APB1 Prescaler                 = 4
	*            APB2 Prescaler                 = 2
	*            HSE Frequency(Hz)              = 8000000
	*            PLL_M                          = 8
	*            PLL_N                          = 432
	*            PLL_P                          = 2
	*            PLL_Q                          = 9
	*            VDD(V)                         = 3.3
	*            Main regulator output voltage  = Scale1 mode
	*            Flash Latency(WS)              = 7
	* @param  None
	* @retval None
	*/
void SystemClock_Config(void)
{
	RCC_ClkInitTypeDef RCC_ClkInitStruct;
	RCC_OscInitTypeDef RCC_OscInitStruct;
	
	/* Enable HSE Oscillator and activate PLL with HSE as source */
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.HSIState = RCC_HSI_OFF;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
	RCC_OscInitStruct.PLL.PLLM = 8;
	RCC_OscInitStruct.PLL.PLLN = 432;
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
	RCC_OscInitStruct.PLL.PLLQ = 9; 
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		while(1) {};
	}
	
	/* Activate the OverDrive to reach the 216 Mhz Frequency */
	if(HAL_PWREx_EnableOverDrive() != HAL_OK)
	{
		while(1) {};
	}

	/* Select PLL as system clock source and configure the HCLK, PCLK1 and PCLK2 
		 clocks dividers */
	RCC_ClkInitStruct.ClockType = (RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2);
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;  
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;  
	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		while(1) {};
	}
}

void print_startup_info() {
	printf("\f\r");
	printf("System startup\n");

	uint32_t tmp = RCC->CFGR & RCC_CFGR_SWS;
	char* clocksource = "UNKNOWN";
	
	switch (tmp) {
		case 0x00:
			clocksource = "HSI";
			break;
		case 0x04:
			clocksource = "HSE";
			break;
		case 0x08:
			clocksource = "PLL";
			break;
		default:
			break;
	}

	uint32_t sysclockfreq = HAL_RCC_GetSysClockFreq();

	printf("System clock source: %s\n", clocksource);
	printf("System clock: %ld MHz\n", sysclockfreq / 1000 / 1000);
}

void system_init() {
	/* Configure the MPU attributes */
	MPU_Config();

	/* Enable the CPU Cache */
	CPU_CACHE_Enable();
	
	/* STM32F7xx HAL library initialization:
			 - Configure the Flash ART accelerator
			 - Systick timer is configured by default as source of time base, but user 
				 can eventually implement his proper time base source (a general purpose 
				 timer for example or other time source), keeping in mind that Time base 
				 duration should be kept 1ms since PPP_TIMEOUT_VALUEs are defined and 
				 handled in milliseconds basis.
			 - Set NVIC Group Priority to 4
			 - Low Level Initialization
		 */
	HAL_Init();

	/* Configure the system clock to 216 MHz */
	SystemClock_Config();

	usart3_init();

	print_startup_info();
}
