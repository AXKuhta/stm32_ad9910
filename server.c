
#define _GNU_SOURCE // Прототип memmem()
#include <assert.h>
#include <string.h>
#include <stdlib.h>

#include "FreeRTOS.h"
#include "task.h"

#include "FreeRTOS_IP.h"
#include "uart_cli.h"

// Отправить большой буфер
void send_all(void* client, const char* data, size_t size) {
	size_t pending = size;

	while (pending > 0) {
		BaseType_t result = FreeRTOS_send(client, data, pending, FREERTOS_MSG_DONTWAIT);

		if (result < 0) {
			printf("Send error: %ld\n", result);
			return;
		}

		if (result == 0) {
			printf("Unable to send; client disconnected?\n");
			return;
		}

		data += result;
		pending -= result;
	}
}

// Получение + запуск команд
// Огромный буфер для удержания JSON объектов
static void read_lines(Socket_t client) {
	char* buffer = malloc(16383);
	size_t capacity = 16383;
	size_t size = 0;

	BaseType_t status;

	while (1) {
		status = FreeRTOS_recv(client, buffer + size, capacity - size, 0);

		if (status < 0)
			return;

		size += status;

		// Поиск \n или \r\n
		char* loc = memmem(buffer, size, "\n", 1);

		if (loc) {
			*loc = 0;

			run(buffer);
			send_all(client, "> ", 2);

			// Прокрутка
			memmove(buffer, loc + 1, loc - buffer + 1);

			size -= (loc - buffer + 1);
		}

		assert(capacity - size > 0);
	}

	free(buffer);
}

static void client_task(void* params) {
	Socket_t client = params;

	printf_redirect(client);

	send_all(client, "Hello!\n> ", 9);
	read_lines(client);

	printf_redirect(NULL);

	printf("End of data\n");

	FreeRTOS_shutdown(client, FREERTOS_SHUT_RDWR);

	printf("Shutdown signalled\n");

	char shut_buf[8];
	BaseType_t shut_status;

	// Wait for it to shut down
	for (int i = 0; i < 2; i++) {
		shut_status = FreeRTOS_recv(client, shut_buf, 8, 0);

		if (shut_status < 0)
			break;

		printf("....waiting for FIN\n");
	}

	printf("Client disconnected\n");

	printf("Close socket: %lu\n", FreeRTOS_closesocket(client));

	// Nothing to return to, must delete self instead
	vTaskDelete(NULL);
}

void server_task(void* params) {
	(void) params;

	// Запуск сервера
	// create tcp socket, bind, listen...
	printf("Server startup\n");

	Socket_t socket = FreeRTOS_socket(
		FREERTOS_AF_INET,
		FREERTOS_SOCK_STREAM,
		FREERTOS_IPPROTO_TCP
	);

	assert(socket != FREERTOS_INVALID_SOCKET);

	struct freertos_sockaddr addr = {
		.sin_port = FreeRTOS_htons(80),
	};

	// FREERTOS_SO_RCVTIMEO также выступает таймаутом для accept()
	// Если выставить его в portMAX_DELAY, то будет блокирующий режим
	// static const TickType_t receive_timeout = portMAX_DELAY;
    // FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

	// Backlog = 1; это однопоточный сервер
	FreeRTOS_bind(socket, &addr, sizeof(addr));
	FreeRTOS_listen(socket, 1);

	while (1) {
		struct freertos_sockaddr client_addr = {0};
		uint32_t clinet_addr_len = 0;
		Socket_t client = FreeRTOS_accept(socket, &client_addr, &clinet_addr_len);

		if (!client)
			continue;

		printf("Client connected\n");

		xTaskCreate( client_task, "srvclient", configMINIMAL_STACK_SIZE*4, client, 1, NULL);
	}
}
