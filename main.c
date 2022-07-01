#include "stm32f7xx_hal.h"
#include "definitions.h"

static void (*next_task)(void);

void set_next_task(void fn(void)) {
	if (next_task == NULL)
		next_task = fn;
}

void run_next_task() {
	if (next_task != NULL) {
		next_task();
		next_task = NULL;
	}
}

void print_startup_info() {
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
	
	print("\f\r");
	print("System startup\r\n");
	
	print("System clock source: "); print(clocksource); print("\r\n");
	print("System clock: "); print_dec(SystemCoreClock / 1000 / 1000); print(" MHz\r\n");
	
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	
	HAL_RCC_GetOscConfig(&RCC_OscInitStruct);
	
	print("PLLM: "); print_dec(RCC_OscInitStruct.PLL.PLLM); print("\r\n");
	print("PLLN: "); print_dec(RCC_OscInitStruct.PLL.PLLN); print("\r\n");
	print("PLLP: "); print_dec(RCC_OscInitStruct.PLL.PLLP); print("\r\n");
	print("PLLQ: "); print_dec(RCC_OscInitStruct.PLL.PLLQ); print("\r\n");
}


int main() {
	SCB_EnableICache();

	// Настроит системный таймер на интервал в 1 мс
	HAL_Init();

	SystemClock_Config();
	SystemCoreClockUpdate();

	led_init();
	button_init();
	spi_init();
	uart_init();
	ad_init_gpio();
	ad_init();
	print_startup_info();
	
	// Пока что мы не выключили делитель на 2
	ad_external_clock(200*1000*1000);
	ad_set_profile_freq(0, 25*1000*1000);
	print("Press r to reset ad9910, v to validate registers\r\n");
	
	// LD1
	HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
	
	while (1) {
		run_next_task();
		
		HAL_PWR_EnterSLEEPMode(PWR_MAINREGULATOR_ON, PWR_SLEEPENTRY_WFI);
	}

}

void SystemClock_Config(void)
{
	RCC_OscInitTypeDef RCC_OscInitStruct = {0};
	RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
	RCC_PeriphCLKInitTypeDef PeriphClkInitStruct = {0};

	/** Configure LSE Drive Capability
	*/
	HAL_PWR_EnableBkUpAccess();
	/** Configure the main internal regulator output voltage
	*/
	__HAL_RCC_PWR_CLK_ENABLE();
	__HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);
	/** Initializes the RCC Oscillators according to the specified parameters
	* in the RCC_OscInitTypeDef structure.
	*/
	RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
	RCC_OscInitStruct.HSEState = RCC_HSE_BYPASS;
	RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
	RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE; 	// HSE имеет частоту 8 МГц, поступает с отладчика
	RCC_OscInitStruct.PLL.PLLM = 4;							// PLL поддерживает входные частоты от 0.95 до 2.10 МГц; делитель нужно выбирать соответствующим образом
	RCC_OscInitStruct.PLL.PLLN = 216;						// PLL может умножать на значения от 24 до 216
	RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;				// Здесь нужно 216 МГц
	RCC_OscInitStruct.PLL.PLLQ = 9;							// Здесь нужно 48 МГц, но также доступны 54 МГц и 72 МГц
	if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
	/** Activate the Over-Drive mode
	*/
	if (HAL_PWREx_EnableOverDrive() != HAL_OK)
	{
		Error_Handler();
	}
	/** Initializes the CPU, AHB and APB buses clocks
	*/
	RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
							  |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
	RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
	RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
	RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
	RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

	if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_7) != HAL_OK)
	{
		Error_Handler();
	}
	PeriphClkInitStruct.PeriphClockSelection = RCC_PERIPHCLK_USART3|RCC_PERIPHCLK_CLK48;
	PeriphClkInitStruct.Usart3ClockSelection = RCC_USART3CLKSOURCE_PCLK1;
	PeriphClkInitStruct.Clk48ClockSelection = RCC_CLK48SOURCE_PLL;
	if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInitStruct) != HAL_OK)
	{
		Error_Handler();
	}
}
