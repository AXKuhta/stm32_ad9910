#include <assert.h>
#include <stdio.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "task.h"

// ##########################################################################################
const uint8_t MAC[6] = {0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

/* Define the network addressing.  These parameters will be used if either
ipconfigUDE_DHCP is 0 or if ipconfigUSE_DHCP is 1 but DHCP auto configuration
failed. */
const uint8_t IP[4] = {10, 15, 15, 250};
const uint8_t Mask[4] = {255, 255, 255, 0};
const uint8_t Gateway[4] = {10, 15, 15, 1};
const uint8_t DNSServer[4] = {1, 1, 1, 1};

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

void vApplicationIPNetworkEventHook( eIPCallbackEvent_t eNetworkEvent ) {
	printf("Network event\n");
	(void) eNetworkEvent;
}
// ##########################################################################################

static void init_task(void* params) {
	(void)params;

	system_init();
	init_deferred_daemon();

	FreeRTOS_IPInit(IP, Mask, Gateway, DNSServer, MAC);

	while (1) {
		isr_recorder_sync();
		deferred_daemon_run_all();
		print_it();
	}
}

int main(void) {
	xTaskCreate( init_task, "init", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);
	vTaskStartScheduler();

	printf("Kernel exited\n");

	while (1) {
	}
}
