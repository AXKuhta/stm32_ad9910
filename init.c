#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "uart.h"
#include "uart_cli.h"
#include "dma.h"
#include "spi.h"
#include "timer/emulator.h"
#include "timer.h"
#include "ad9910.h"
#include "ethernet.h"
#include "sequencer.h"


static void MPU_Config(void) {
	MPU_Region_InitTypeDef MPU_InitStruct = {
		.Enable = MPU_REGION_ENABLE,
		.BaseAddress = 0x00,
		.Size = MPU_REGION_SIZE_4GB,
		.AccessPermission = MPU_REGION_NO_ACCESS,
		.IsBufferable = MPU_ACCESS_NOT_BUFFERABLE,
		.IsCacheable = MPU_ACCESS_NOT_CACHEABLE,
		.IsShareable = MPU_ACCESS_NOT_SHAREABLE,
		.Number = MPU_REGION_NUMBER0,
		.TypeExtField = MPU_TEX_LEVEL0,
		.SubRegionDisable = 0x87,
		.DisableExec = MPU_INSTRUCTION_ACCESS_DISABLE
	};

	HAL_MPU_Disable();
	HAL_MPU_ConfigRegion(&MPU_InitStruct);
	HAL_MPU_Enable(MPU_PRIVILEGED_DEFAULT);
}

// Enabling dcache causes Ethernet to be unreliable
// Thus cache left disabled
// static void CPU_CACHE_Enable(void) {
// 	SCB_EnableICache();
// 	SCB_EnableDCache();
// }

// Select HSI as STM32's SYSCLK source
// Used when PLL needs to be reset
// HSI must be on
static void source_sysclk_from_hsi() {
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { // ARM and bus clock dividers
		.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
		.SYSCLKSource = RCC_SYSCLKSOURCE_HSI,
		.AHBCLKDivider = RCC_SYSCLK_DIV1,
		.APB1CLKDivider = RCC_HCLK_DIV4,
		.APB2CLKDivider = RCC_HCLK_DIV2
	};

	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
		while(1) {};
	}
}

// Select PLL as STM32's SYSCLK source
// PLL must be on
static void source_sysclk_from_pll() {
	RCC_ClkInitTypeDef RCC_ClkInitStruct = { // ARM and bus clock dividers
		.ClockType = RCC_CLOCKTYPE_SYSCLK | RCC_CLOCKTYPE_HCLK | RCC_CLOCKTYPE_PCLK1 | RCC_CLOCKTYPE_PCLK2,
		.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK,
		.AHBCLKDivider = RCC_SYSCLK_DIV1,
		.APB1CLKDivider = RCC_HCLK_DIV4,
		.APB2CLKDivider = RCC_HCLK_DIV2
	};

	if(HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK) {
		while(1) {};
	}
}

// Run from 16 MHz internal oscillator
static void run_from_hsi(void) {
	// HSI Enabled
	// HSE Disabled
	// HSI 16 MHZ is divided by 16 to form 1 MHz
	// The PLL then produces 432 MHz locked to 1 MHz
	// 432 MHz is divided by 2 to form 216 MHz SYSCLK
	RCC_OscInitTypeDef RCC_OscInitStruct = {
		.OscillatorType = RCC_OSCILLATORTYPE_HSI,
		.HSEState = RCC_HSE_OFF,
		.HSIState = RCC_HSI_ON,
		.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
		.PLL = {
			.PLLState = RCC_PLL_ON,
			.PLLSource = RCC_PLLSOURCE_HSI,
			.PLLM = 16,
			.PLLN = 216*2,
			.PLLP = RCC_PLLP_DIV2,
			.PLLQ = 9
		}
	};
	
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		while(1) {};
	}
	
	if(HAL_PWREx_EnableOverDrive() != HAL_OK) {
		while(1) {};
	}

	source_sysclk_from_pll();
}

// Run from 25 MHz external clock
static void run_from_hse(void) {
	source_sysclk_from_hsi();

	// HSI Disabled
	// HSE Enabled (Bypass means no crystal)
	// HSE 25 MHZ is divided by 25 to form 1 MHz
	// The PLL then produces 432 MHz locked to 1 MHz
	// 432 MHz is divided by 2 to form 216 MHz SYSCLK
	RCC_OscInitTypeDef RCC_OscInitStruct = {
		.OscillatorType = RCC_OSCILLATORTYPE_HSE,
		.HSEState = RCC_HSE_BYPASS,
		.HSIState = RCC_HSI_OFF,
		.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT,
		.PLL = {
			.PLLState = RCC_PLL_ON,
			.PLLSource = RCC_PLLSOURCE_HSE,
			.PLLM = 25,
			.PLLN = 216*2,
			.PLLP = RCC_PLLP_DIV2,
			.PLLQ = 9
		}
	};
	
	if(HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK) {
		while(1) {};
	}
	
	if(HAL_PWREx_EnableOverDrive() != HAL_OK) {
		while(1) {};
	}

	source_sysclk_from_pll();
}

// Run timers at full clock
void enable_fast_timers() {
	RCC_PeriphCLKInitTypeDef RCC_PeriphCLKInitStruct = {
		.PeriphClockSelection = RCC_PERIPHCLK_TIM,
		.TIMPresSelection = RCC_TIMPRES_ACTIVATED
	};

	HAL_RCCEx_PeriphCLKConfig(&RCC_PeriphCLKInitStruct);
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

	// Leave the cache disabled
	//CPU_CACHE_Enable();
	
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

	run_from_hsi();
	enable_fast_timers();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOE_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	usart3_init();

	print_startup_info();

	radar_emulator_start(25, 12.0 / 1000.0 / 1000.0, 0);
	spi4_init();
	ad_init();

	run_from_hsi();

	dma_init();

	sequencer_init();
	enter_rfkill_mode();
	uart_cli_init();
	network_init();
}
