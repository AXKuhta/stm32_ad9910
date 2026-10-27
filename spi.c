#include <assert.h>

#include "stm32f7xx_hal.h"
#include "pin_init.h"

#define SPI1_NSS  	GPIOA, GPIO_PIN_4
#define SPI1_SCK  	GPIOB, GPIO_PIN_3
#define SPI1_MISO 	GPIOB, GPIO_PIN_4
#define SPI1_MOSI 	GPIOB, GPIO_PIN_5

static void spi1_gpio_init() {
	PIN_AF_Init(SPI1_NSS, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI1);
	PIN_AF_Init(SPI1_SCK, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI1);
	PIN_AF_Init(SPI1_MISO, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI1);
	PIN_AF_Init(SPI1_MOSI, GPIO_MODE_AF_PP, GPIO_NOPULL, GPIO_AF5_SPI1);
}

SPI_HandleTypeDef spi1;

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
void spi1_init() {
	__HAL_RCC_SPI1_CLK_ENABLE();
	spi1_gpio_init();

    SPI_HandleTypeDef spi_defaults = {
        .Instance = SPI1,
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

	spi1 = spi_defaults;
	
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
