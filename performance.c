#include <stdint.h>
#include <stdio.h>

#include "FreeRTOS.h"

uint32_t perf_usart3_bytes_rx = 0;
uint32_t perf_usart3_bytes_tx = 0;

uint32_t perf_spi4_bytes_rx = 0;
uint32_t perf_spi4_bytes_tx = 0;

uint32_t perf_wakeups = 0;

uint32_t perf_eth_frames_rx = 0;
uint32_t perf_eth_frames_tx = 0;
uint32_t perf_eth_bytes_rx = 0;
uint32_t perf_eth_bytes_tx = 0;

void print_perf() {
	printf("%10s %s\n", "COUNT", "PERFORMANCE COUNTER");
	
	printf("%10ld %s\n", perf_usart3_bytes_rx, "usart3 bytes rx");
	printf("%10ld %s\n", perf_usart3_bytes_tx, "usart3 bytes tx");

	printf("%10ld %s\n", perf_spi4_bytes_rx, "spi4 bytes rx");
	printf("%10ld %s\n", perf_spi4_bytes_tx, "spi4 bytes tx");

	printf("%10ld %s\n", perf_wakeups, "wakeups");

	printf("%10u %s\n", xPortGetFreeHeapSize(), "heap bytes unused");

	printf("%10ld %s\n", perf_eth_frames_tx, "eth frames tx");
	printf("%10ld %s\n", perf_eth_frames_rx, "eth frames rx");
	printf("%10ld %s\n", perf_eth_bytes_tx, "eth bytes tx");
	printf("%10ld %s\n", perf_eth_bytes_rx, "eth bytes rx");
}
