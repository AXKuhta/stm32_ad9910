#include <assert.h>
#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "init.h"
#include "isr.h"
#include "tasks.h"

#include "FreeRTOS.h"
#include "task.h"

static void aaa_task(void* params) {
	(void)params;

	while (1) {
		_write(0, "AAA", 3);
		vTaskDelay(1000);
	}
}

static void bbb_task(void* params) {
	(void)params;

	while (1) {
		_write(0, "BBB", 3);
		vTaskDelay(1000);
	}
}

static void run_commands_task(void* params) {
	while (1) {
		isr_recorder_sync();
		run_all_tasks();
	}
}

int main(void) {
	system_init();

	xTaskCreate( aaa_task, "AAA", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate( bbb_task, "BBB", configMINIMAL_STACK_SIZE, NULL, 1, NULL);
	xTaskCreate( run_commands_task, "CLI", configMINIMAL_STACK_SIZE, NULL, 1, NULL);

	printf("Task create\n");

	vTaskStartScheduler();

	printf("Kernel exited\n");

	while (1) {
	}
}
