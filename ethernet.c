#include "stm32f7xx_hal.h"
#include "FreeRTOSConfig.h"

#include "pin_init.h"

//
// For NUCLEO-F746ZG board (RMII)
//
// PA1     ------> RMII_REF_CLK
// PA2     ------> RMII_MDIO
// PC1     ------> RMII_MDC
// PA7     ------> RMII_RxDataValid
// PC4     ------> RMII_RXD0
// PC5     ------> RMII_RXD1
// PG11    ------> RMII_TXEnable
// PG13    ------> RMII_TXD0
// PB13    ------> RMII_TXD1
//

#define RMII_REF_CLK		GPIOA, GPIO_PIN_1
#define RMII_MDIO			GPIOA, GPIO_PIN_2
#define RMII_MDC			GPIOC, GPIO_PIN_1
#define RMII_RxDataValid	GPIOA, GPIO_PIN_7
#define RMII_RXD0			GPIOC, GPIO_PIN_4
#define RMII_RXD1			GPIOC, GPIO_PIN_5
#define RMII_TXEnable		GPIOG, GPIO_PIN_11
#define RMII_TXD0			GPIOG, GPIO_PIN_13
#define RMII_TXD1			GPIOB, GPIO_PIN_13

static void ethernet_init() {
	__HAL_RCC_ETHMAC_CLK_ENABLE();
	__HAL_RCC_ETHMACTX_CLK_ENABLE();
	__HAL_RCC_ETHMACRX_CLK_ENABLE();

	__HAL_RCC_GPIOA_CLK_ENABLE();
	__HAL_RCC_GPIOB_CLK_ENABLE();
	__HAL_RCC_GPIOC_CLK_ENABLE();
	__HAL_RCC_GPIOG_CLK_ENABLE();

	PIN_AF_Init(RMII_REF_CLK, 		GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_MDIO, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_MDC, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_RxDataValid, 	GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_RXD0, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_RXD1, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_TXEnable, 		GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_TXD0, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);
	PIN_AF_Init(RMII_TXD1, 			GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF11_ETH);

	HAL_NVIC_SetPriority(ETH_IRQn, configLIBRARY_MAX_SYSCALL_INTERRUPT_PRIORITY, 0);
	HAL_NVIC_EnableIRQ(ETH_IRQn);
}

void HAL_ETH_MspInit(ETH_HandleTypeDef* xETH_Handle) {
	if (xETH_Handle->Instance == ETH) {
		ethernet_init();
	}
}

