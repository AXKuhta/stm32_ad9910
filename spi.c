#include <assert.h>

#include "stm32f7xx_hal.h"

void spi1_gpio_init() {
	GPIO_InitTypeDef SCK = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_5, .Alternate = GPIO_AF5_SPI1 };
	GPIO_InitTypeDef MISO = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_6, .Alternate = GPIO_AF5_SPI1 };
	GPIO_InitTypeDef MOSI = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_7, .Alternate = GPIO_AF5_SPI1 };
	
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOA, &SCK);
	HAL_GPIO_Init(GPIOA, &MISO);
	HAL_GPIO_Init(GPIOA, &MOSI);
}

SPI_HandleTypeDef spi1;

void spi1_init() {
	__HAL_RCC_SPI1_CLK_ENABLE();
	spi1_gpio_init();

    SPI_HandleTypeDef spi1_defaults = {
        .Instance = SPI1,
        .Init = {
            .Mode = SPI_MODE_MASTER,
            .Direction = SPI_DIRECTION_2LINES,
            .DataSize = SPI_DATASIZE_8BIT,
            .NSS = SPI_NSS_SOFT,
            .NSSPMode = SPI_NSS_PULSE_DISABLE,
            .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256,
            .FirstBit = SPI_FIRSTBIT_MSB,
            .TIMode = SPI_TIMODE_DISABLE,
            .CLKPolarity = SPI_POLARITY_LOW,
            .CLKPhase = SPI_PHASE_1EDGE,
            .CRCCalculation = SPI_CRCCALCULATION_DISABLE
        }
    };

	spi1 = spi1_defaults;
	
	HAL_SPI_Init(&spi1);
}

extern uint32_t perf_spi1_bytes_rx;
extern uint32_t perf_spi1_bytes_tx;

void spi_send(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&spi1, buffer, size, 1000);

	assert(status == HAL_OK);

	perf_spi1_bytes_tx += size;
}

void spi_recv(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Receive(&spi1, buffer, size, 1000);
	
	assert(status == HAL_OK);

	perf_spi1_bytes_rx += size;
}
