
typedef enum event_origin_t {
	UNSET,
	TRIGGER_EVENT,	// Калибратор получил триггер
	READY_EVENT,	// Калибратор готов к подаче импульса
	DDC_ACK_EVENT	// Получен пакет от DDC (считается признанием триггера)
} event_origin_t;

typedef struct event_t {
	event_origin_t origin;
	uint64_t timestamp;

	int orda_type;
	int orda_size;
} event_t;

void event_queue_init();
void event_queue_push(event_t evt);
void event_queue_push_isr(event_t evt);
