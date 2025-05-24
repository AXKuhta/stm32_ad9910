#include "stm32f7xx_hal.h"
#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"

#include "pin_init.h"

const uint8_t MAC[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

/* Define the network addressing.  These parameters will be used if either
ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
failed. */
const uint8_t IP[4] = {192, 168, 0, 12};
const uint8_t Mask[4] = {255, 255, 255, 0};
const uint8_t Gateway[4] = {192, 168, 0, 1};
const uint8_t DNSServer[4] = {1, 1, 1, 1};

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

BaseType_t xApplicationGetRandomNumber( uint32_t *pulValue ) {
	printf("Random number event\n");
	*pulValue = HAL_GetTick();
	return pdPASS;
}

BaseType_t xApplicationDNSQueryHook( const char * pcName ) {
	printf("DNS event\n");
	(void) pcName;
	return pdPASS;
}

uint32_t ulApplicationGetNextSequenceNumber( uint32_t ulSourceAddress, uint16_t usSourcePort, uint32_t ulDestinationAddress, uint16_t usDestinationPort ) {
	printf("Get next sequence number\n");
	(void) ulSourceAddress;
	(void) usSourcePort;
	(void) ulDestinationAddress;
	(void) usDestinationPort;
	return HAL_GetTick();
}

eDHCPCallbackAnswer_t xApplicationDHCPHook( eDHCPCallbackPhase_t eDHCPPhase, uint32_t ulIPAddress ) {
	printf("DHCP event\n");
	(void) eDHCPPhase;
	(void) ulIPAddress;
	return eDHCPContinue;
}

static eIPCallbackEvent_t network_state = eNetworkDown;

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent ) {
	switch (eNetworkEvent) {
		case eNetworkDown:
			if (network_state != eNetworkDown)
				printf("Network is down\n");
			
			network_state = eNetworkDown;
			break;
		case eNetworkUp:
			if (network_state != eNetworkUp)
				printf("Network is up\n");
			
			network_state = eNetworkUp;
			break;
		default:
			printf("Unknown network event\n");
			break;
	}
}

void network_init() {
    FreeRTOS_IPInit(IP, Mask, Gateway, DNSServer, MAC);
}
