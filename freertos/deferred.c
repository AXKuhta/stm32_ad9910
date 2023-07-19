#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"
#include "task.h"

static QueueHandle_t deferred_fn_calls = NULL;

void init_deferred_daemon() {
	deferred_fn_calls = xQueueCreate( 32, sizeof(void*) );
}

void deferred_daemon_run_all() {
	void (*f)();

	while (xQueueReceive( deferred_fn_calls, &f, 1000 ) == pdPASS) {
		f();
	}
}

// Запланировать вызов некоторой функции после выхода из прерывания
void run_later(void f()) {
	xQueueSendFromISR(deferred_fn_calls, &f, NULL);
}
