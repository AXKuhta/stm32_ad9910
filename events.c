
#include "FreeRTOS.h"
#include "queue.h"

#include "events.h"

QueueHandle_t event_queue = NULL;

// Очередь событий для:
// - Триггера
// - Готовности к триггеру
// - Пакетов от DDC
//
// В данный момент просто очередь, в будущем можно сделать шину событий с подписками
//
void event_queue_init() {
	event_queue = xQueueCreate(32, sizeof(event_t));
}

void event_queue_push(event_t evt) {
	xQueueSend(event_queue, &evt, 0);
}

void event_queue_push_isr(event_t evt) {
	xQueueSendFromISR(event_queue, &evt, 0);
}
