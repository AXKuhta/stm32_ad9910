
// Макрос-обёртка над HAL_GPIO_Init()
// Позволяет передавать порт и номер ножки первыми двумя аргументами
// Порт и номер ножки можно объединить под одним идентификатором
#define _PIN_Init(PORT, PIN, MODE, PULL, AF) do { \
	HAL_GPIO_Init(PORT, &((GPIO_InitTypeDef) { \
		.Mode = MODE, \
		.Pull = PULL, \
		.Speed = GPIO_SPEED_FREQ_VERY_HIGH, \
		.Pin = PIN, \
		.Alternate = AF \
	})); \
} while (0)

// Инициализация выхода с параметрами по умолчанию
#define PIN_Init(...) _PIN_Init(__VA_ARGS__, GPIO_MODE_OUTPUT_PP, GPIO_NOPULL, 0)

// Инициализация выхода под периферал
#define PIN_AF_Init(...) _PIN_Init(__VA_ARGS__)
