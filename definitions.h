#include <stdint.h>

// ANSI цвета
#define COLOR_RED "\x1b[31m"
#define COLOR_GREEN "\x1b[32m"
#define COLOR_RESET "\x1b[0m"

//
// Светодиоды
// leds.c
//
void led_init(void);

//
// Кнопки
// buttons.c
//
void button_init(void);

//
// SPI
// spi.c
//
void spi1_gpio_init(void);
void spi_init(void);
void spi_selftest(void);
void spi_send(uint8_t*, uint16_t);
void spi_recv(uint8_t*, uint16_t);
void spi_reset(void);

//
// UART
// uart.c
//
void usart3_gpio_init(void);
void uart_init(void);
void uart_send(uint8_t*, uint16_t);
void print(char*);
void print_hex(uint32_t);
void print_dec(uint32_t);

//
// AD9910
// ad9910.c
//
void ad_init_gpio(void);
void ad_select(void);
void ad_deselect(void);
void ad_pulse_io_update(void);
void ad_pulse_io_reset(void);
void ad_write(uint8_t, uint8_t*, uint16_t);
void ad_readback(uint8_t, uint8_t*, uint16_t);
void ad_readback_all(void);
void ad_write_all(void);
void ad_init(void);
void ad_test(void);
void ad_external_clock(uint32_t);
uint32_t ad_calc_ftw(uint32_t);
void ad_set_profile_freq(int, uint32_t);

//
// Запуск и главный цикл
// main.c
//
void SystemClock_Config(void);
void print_startup_info(void);
void set_next_task(void(void));
void run_next_task(void);

//
// Обработчики прерываний
// isr.c
//
void Error_Handler(void);	// Это прерывание?
void SysTick_Handler(void);
void EXTI15_10_IRQHandler(void);
void USART3_IRQHandler(void);
