#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

QueueHandle_t deferred_fn_calls = NULL;

void init_deferred_fn_call_infra() {
	deferred_fn_calls = xQueueCreate( 32, sizeof(void*) );
}

void run_all_deferred_fn() {
	void (*f)();

	while (xQueueReceive( deferred_fn_calls, &f, 1000 ) == pdPASS) {
		f();
	}
}

void add_task(void f()) {
	xQueueSendFromISR(deferred_fn_calls, &f, NULL);
}
