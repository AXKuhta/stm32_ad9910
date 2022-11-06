#include <stdint.h>
#include <stdio.h>

uint32_t perf_usart3_bytes_rx = 0;
uint32_t perf_usart3_bytes_tx = 0;

void print_perf() {
    printf("Performance counters:\n");
    printf("usart3 bytes rx: %ld\n", perf_usart3_bytes_rx);
    printf("usart3 bytes tx: %ld\n", perf_usart3_bytes_tx);
}
