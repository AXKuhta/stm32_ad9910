#include <assert.h>
#include <stdio.h>

#include "freertos_extras/deferred.h"
#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"

#include "FreeRTOS.h"
#include "task.h"

static void workloop(void* params) {
	(void)params;

	while (1) {
		isr_recorder_sync();
		deferred_daemon_run_all();
	}
}

static void init_task(void* params) {
	(void)params;

	system_init();
	init_deferred_daemon();

	xTaskCreate( workloop, "work", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);

	while (1) {
		// ... Использование блокирования в этом месте провоцирует зависание??
	}
}

int main(void) {
	xTaskCreate( init_task, "init", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);
	vTaskStartScheduler();

	printf("Kernel exited\n");

	while (1) {
	}
}
