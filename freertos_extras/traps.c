#include "FreeRTOS.h"
#include "task.h"

#include "syscalls.h"

void vApplicationStackOverflowHook( TaskHandle_t xTask, char *pcTaskName) {
	(void)xTask;
	(void)pcTaskName;

	_write(0, "\nstack smashing\n", 16);

	while (1) {
	}
}

void vApplicationMallocFailedHook() {
	_write(0, "\nmalloc fail\n", 13);

	while (1) {
	}
}
