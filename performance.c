#include <stdint.h>
#include <stdio.h>

uint32_t perf_usart3_bytes_rx = 0;
uint32_t perf_usart3_bytes_tx = 0;

uint32_t perf_spi4_bytes_rx = 0;
uint32_t perf_spi4_bytes_tx = 0;

uint32_t perf_wakeups = 0;

void print_perf() {
	printf("Performance counters:\n");
	
	printf("usart3 bytes rx: %ld\n", perf_usart3_bytes_rx);
	printf("usart3 bytes tx: %ld\n", perf_usart3_bytes_tx);

	printf("spi4 bytes rx: %ld\n", perf_spi4_bytes_rx);
	printf("spi4 bytes tx: %ld\n", perf_spi4_bytes_tx);

	printf("wakeups: %ld\n", perf_wakeups);
}
