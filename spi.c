#include "stm32f7xx_hal.h"
#include "definitions.h"

//
// SPI1:
// PA5 SCK
// PA6 MISO
// PA7 MOSI
//
void spi1_gpio_init() {
	GPIO_InitTypeDef SCK = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_5, .Alternate = GPIO_AF5_SPI1 };
	GPIO_InitTypeDef MISO = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_6, .Alternate = GPIO_AF5_SPI1 };
	GPIO_InitTypeDef MOSI = { .Mode = GPIO_MODE_AF_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_HIGH, .Pin = GPIO_PIN_7, .Alternate = GPIO_AF5_SPI1 };
	
	// Уже сделано в led_init()
	__HAL_RCC_GPIOA_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOA, &SCK);
	HAL_GPIO_Init(GPIOA, &MISO);
	HAL_GPIO_Init(GPIOA, &MOSI);
}

void HAL_SPI_MspInit(SPI_HandleTypeDef *hspi) {
	// Просто выйти при попытке инициализировать что-то кроме SPI1
	if (hspi->Instance != SPI1)
		return;
	
	__HAL_RCC_SPI1_CLK_ENABLE();
	
	spi1_gpio_init();
}

static SPI_HandleTypeDef hspi;

void spi_init() {
	hspi.Instance = SPI1;
	hspi.Init.Mode = SPI_MODE_MASTER;
	hspi.Init.Direction = SPI_DIRECTION_2LINES;
	hspi.Init.DataSize = SPI_DATASIZE_8BIT;
	hspi.Init.NSS = SPI_NSS_SOFT;
	hspi.Init.NSSPMode = SPI_NSS_PULSE_DISABLE;
	hspi.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_256;
	hspi.Init.FirstBit = SPI_FIRSTBIT_MSB;
	hspi.Init.TIMode = SPI_TIMODE_DISABLE;
	
	hspi.Init.CLKPolarity = SPI_POLARITY_LOW;
	hspi.Init.CLKPhase = SPI_PHASE_1EDGE;
//	hspi.Init.CLKPhase = SPI_PHASE_2EDGE;
	
	hspi.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
	
	HAL_SPI_Init(&hspi);
}

// Протестировать передачу данных по SPI, когда MOSI и MISO соединены перемычкой, и используется 3-Wire режим
void spi_selftest() {
	uint8_t tx = 42;
	uint8_t rx = 0;
	
	HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(&hspi, &tx, &rx, 1, 100);
	
	if (status == HAL_OK && rx == 42)
		print("SPI self-test OK\r\n");
}

void spi_send(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Transmit(&hspi, buffer, size, 1000);
	
	if (status != HAL_OK)
		print(COLOR_RED "SPI send error\r\n" COLOR_RESET);
}

void spi_recv(uint8_t* buffer, uint16_t size) {
	HAL_StatusTypeDef status = HAL_SPI_Receive(&hspi, buffer, size, 1000);
	
	if (status == HAL_TIMEOUT) {
		print("SPI recv timeout\r\n");
	} else if (status != HAL_OK) {
		print(COLOR_RED "SPI recv error\r\n" COLOR_RESET);
	}
}

void spi_reset() {
	HAL_SPI_Abort(&hspi);
}
