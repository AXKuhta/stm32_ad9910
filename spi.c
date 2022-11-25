#include <assert.h>

#include "stm32f7xx_hal.h"

static void spi4_gpio_init() {
	GPIO_InitTypeDef NSS = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_11, .Alternate = GPIO_AF5_SPI4 };
	GPIO_InitTypeDef SCK = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_12, .Alternate = GPIO_AF5_SPI4 };
	GPIO_InitTypeDef MISO = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_13, .Alternate = GPIO_AF5_SPI4 };
	GPIO_InitTypeDef MOSI = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_14, .Alternate = GPIO_AF5_SPI4 };
	
	__HAL_RCC_GPIOE_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOE, &NSS);
	HAL_GPIO_Init(GPIOE, &SCK);
	HAL_GPIO_Init(GPIOE, &MISO);
	HAL_GPIO_Init(GPIOE, &MOSI);
}

SPI_HandleTypeDef spi4;

// Вычисление скорости SPI:
// spi_clock = 108 MHz
// prescaler = 16
//
// bits_per_second = 1s * spi_clock / prescaler
// mbit = bits_per_second / 1000 / 1000
//
// mbit = 6.75
//
// Для AD9910 максимальная скорость SPI составляет 70 Мбит
// STM32 не может выдать больше 54 Мбит?
//
void spi4_init() {
	__HAL_RCC_SPI4_CLK_ENABLE();
	spi4_gpio_init();

    SPI_HandleTypeDef spi4_defaults = {
        .Instance = SPI4,
        .Init = {
            .Mode = SPI_MODE_MASTER,
            .Direction = SPI_DIRECTION_2LINES,
            .DataSize = SPI_DATASIZE_8BIT,
            .NSS = SPI_NSS_HARD_OUTPUT,
            .NSSPMode = SPI_NSS_PULSE_DISABLE,
            .BaudRatePrescaler = SPI_BAUDRATEPRESCALER_16,
            .FirstBit = SPI_FIRSTBIT_MSB,
            .TIMode = SPI_TIMODE_DISABLE,
            .CLKPolarity = SPI_POLARITY_LOW,
            .CLKPhase = SPI_PHASE_1EDGE,
            .CRCCalculation = SPI_CRCCALCULATION_DISABLE
        }
    };

	spi4 = spi4_defaults;
	
	HAL_SPI_Init(&spi4);
}

extern uint32_t perf_spi4_bytes_rx;
extern uint32_t perf_spi4_bytes_tx;

void spi_send(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&spi4, buffer, size, 1000);

	assert(status == HAL_OK);

	perf_spi4_bytes_tx += size;
}

void spi_recv(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Receive(&spi4, buffer, size, 1000);
	
	assert(status == HAL_OK);

	perf_spi4_bytes_rx += size;
}
