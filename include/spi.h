#include <stdint.h>

void spi4_init();
void spi_send(uint8_t* buffer, uint16_t size);
void spi_recv(uint8_t* buffer, uint16_t size);
