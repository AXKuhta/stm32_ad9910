#include <stdint.h>

void spi1_init(void);
void spi_send(uint8_t* buffer, uint16_t size);
void spi_recv(uint8_t* buffer, uint16_t size);
