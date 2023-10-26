
#include <assert.h>

#include "FreeRTOS_IP.h"

// Прочитать HTTP запрос + заголовки + тело
static void read_request(Socket_t client) {
	char buffer[128] = {0}; // Буффер строки
	size_t last = 0;

	BaseType_t status;

	while (1) {
		size_t avail = 128 - last;

		assert(avail >= 1);

		status = FreeRTOS_recv(client, buffer + last, avail, 0);

		if (status < 0)
			return;

		last += status;

		size_t i = 0;

		// Поиск \r\n
		while (last >= 2) {
			int found = (memcmp(buffer + i, "\r\n", 2) == 0);

			if (found) {
				buffer[i] = 0;
				printf("Header: %s\n", buffer);
				memmove(buffer, buffer + i + 2, 128 - i - 2);

				// Нашёлся \r\n\r\n - заголовки кончились
				if (i == 0) {
					printf("Headers ended; should read body now\n");
					return;
				}

				last -= i + 2;
				i = 0;
			} else {
				i++;
			}
		}
	}
}

// Отправить большой буффер
static void send_all(Socket_t client, char* data, size_t size) {
	size_t pending = size;

	while (pending > 0) {
		BaseType_t result = FreeRTOS_send(client, data, pending, 0);

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

		read_request(client);

		char* response = 	"HTTP/1.0 200 OK\r\n\r\n"
							"<!DOCTYPE html>"
							"Hello!!!!!!!";

		send_all(client, response, strlen(response));

		printf("All data sent\n");

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
	}
}
