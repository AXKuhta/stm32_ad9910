#include <assert.h>
#include <stdio.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "server.h"
#include "init.h"
#include "isr.h"

#include "FreeRTOS.h"
#include "FreeRTOS_IP.h"
#include "events.h"
#include "task.h"
#include "ack.h"

static void init_task(void* params) {
	(void)params;

	system_init();
	event_queue_init();
	init_deferred_daemon();

	xTaskCreate( server_task, "srv", configMINIMAL_STACK_SIZE*4, NULL, 1, NULL);
	xTaskCreate( ack_damenon_task, "ack_daemon", configMINIMAL_STACK_SIZE*4, NULL, 1, NULL);

	while (1) {
		isr_recorder_sync();
		deferred_daemon_run_all();
	}
}

int main(void) {
	init_allocator();

	xTaskCreate( init_task, "init", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);
	vTaskStartScheduler();

	printf("Kernel exited\n");

	while (1) {
	}
}
