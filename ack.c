#include <assert.h>
#include <stdio.h>

#include "FreeRTOS.h"
#include "queue.h"

#include "FreeRTOS_IP.h"

#include "timer/sequencing.h"
#include "events.h"
#include "ack.h"

// DDC Trigger Acknowledment receiver task
void ack_damenon_task(void* params) {
	Socket_t socket = FreeRTOS_socket(FREERTOS_AF_INET, FREERTOS_SOCK_DGRAM, 0);

	assert(socket != FREERTOS_INVALID_SOCKET);

	struct freertos_sockaddr addr = {
		.sin_port = FreeRTOS_htons(25000),
		.sin_address.ulIP_IPv4 = FreeRTOS_inet_addr("234.5.6.7")
	};

	// По умолчанию таймаут 5 секунд (см. ipconfigSOCK_DEFAULT_RECEIVE_BLOCK_TIME)
	// static const TickType_t receive_timeout = 1000;
	// FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_RCVTIMEO, &receive_timeout, sizeof(receive_timeout));

	// Добавление в multicast группу
	// - Без этого multicast пакеты будут аппаратно отфильтровываться (если поступают)
	// - Если на коммутаторе включен IGMP Snooping и нет подписки, то пакеты вообще не будут поступать
	IP_MReq_t req = {
		.pxMulticastNetIf = NULL,
		.xMulticastGroup = FreeRTOS_inet_addr("234.5.6.7")
	};

	assert( FreeRTOS_setsockopt(socket, 0, FREERTOS_SO_IP_ADD_MEMBERSHIP, &req, sizeof(req)) == 0 );

	assert( FreeRTOS_bind(socket, &addr, sizeof(addr)) == 0 );

	while (1) {
		struct freertos_sockaddr from = {0};
		socklen_t from_sz = sizeof(from);
		char buf;

		BaseType_t status = FreeRTOS_recvfrom(socket, &buf, 1, 0, &from, &from_sz);

		if (status == -pdFREERTOS_ERRNO_EWOULDBLOCK) {
			continue;
		} else if (status < 0) {
			FreeRTOS_closesocket(socket);
			printf("recvfrom error %ld\n", status);
			assert(0);
		}

		if (status > 0) {
			event_queue_push((event_t) { .origin = DDC_ACK_EVENT, .timestamp = logic_blaster_hrtime() });
		}
	}

	(void)params;
}
