#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"
#include "tasks.h"

#include "FreeRTOS.h"
#include "task.h"

static void run_commands_task(void* params) {
	(void)params;

	while (1) {
		isr_recorder_sync();
		run_all_tasks();
	}
}

static void init_task(void* params) {
	(void)params;

	system_init();

	xTaskCreate( run_commands_task, "CLI", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);

	while (1) {
	}
}

int main(void) {
	xTaskCreate( init_task, "init", configMINIMAL_STACK_SIZE*8, NULL, 1, NULL);
	vTaskStartScheduler();

	printf("Kernel exited\n");

	while (1) {
	}
}
