#include <stdio.h>

#include "stm32f7xx_hal.h"
#include "vt100.h"
#include "spi.h"

// PD14 Chip Select
// PD15 IO_RESET
// PF12 IO_UPDATE
void ad_init_gpio() {
	GPIO_InitTypeDef CS = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_14 };
	GPIO_InitTypeDef IO_RESET = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_15 };
	GPIO_InitTypeDef IO_UPDATE = { .Mode = GPIO_MODE_OUTPUT_PP, .Pull = GPIO_NOPULL, .Speed = GPIO_SPEED_LOW, .Pin = GPIO_PIN_12 };
	
	__HAL_RCC_GPIOD_CLK_ENABLE();
	__HAL_RCC_GPIOF_CLK_ENABLE();
	
	HAL_GPIO_Init(GPIOD, &CS);
	HAL_GPIO_Init(GPIOD, &IO_RESET);
	HAL_GPIO_Init(GPIOF, &IO_UPDATE);
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_12, GPIO_PIN_RESET);
}

void ad_select() {
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_RESET);
	
	for (uint16_t i = 0; i < 1024; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
}

void ad_deselect() {
	for (uint16_t i = 0; i < 1024; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
}

void ad_pulse_io_reset(void) {
	// IO_RESET
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_SET);
	
	// Подождать ~20 мкс
	//for (uint16_t i = 0; i < 1024; i++) {
	//	__NOP(); __NOP(); __NOP(); __NOP();
	//}
	HAL_Delay(1);
	
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
	
	// Подождать ещё
	//for (uint16_t i = 0; i < 1024; i++) {
	//	__NOP(); __NOP(); __NOP(); __NOP();
	//}
	HAL_Delay(1);
}

void my_delay(uint32_t delay) {
	for (uint32_t i=0; i<delay; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
}
	

void ad_pulse_io_update(void) {
	// IO_UPDATE в единичку
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_12, GPIO_PIN_SET);
/*	
	// Подождать ~20 мкс
	for (uint16_t i = 0; i < 1024; i++) {
		__NOP(); __NOP(); __NOP(); __NOP();
	}
*/
	HAL_Delay(2);
	// IO_UPDATE в ноль
	HAL_GPIO_WritePin(GPIOF, GPIO_PIN_12, GPIO_PIN_RESET);
}

void ad_write(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = reg_addr & 0x1F;
	
	ad_pulse_io_reset();
	ad_select();
	
	spi_send(&instruction, 1);
	my_delay(500);
	for (uint16_t i = 0; i < size; i++) {
		spi_send(&buffer[i], 1);
		my_delay(500);
	}
	ad_deselect();
}

//
// Чтение регистров AD9910
//
void ad_readback(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = 0x80 | (reg_addr & 0x1F);
	uint8_t read[8] = {0};
	
	ad_pulse_io_reset();
	ad_select();
	spi_send(&instruction, 1);
	my_delay(500);
	
	for (int i = 0; i < size; i++) {
		spi_recv(&read[i], 1);
		my_delay(500);
	}
	ad_deselect();
	
	printf("ad9910.c: register 0x%02X: ", reg_addr);
	
	for (int i = 0; i < size; i++) {
		printf(COLOR_GREEN "%s0x%02X ", buffer[i] == read[i] ? COLOR_GREEN : COLOR_RED, read[i]);
	}
	
	printf(COLOR_RESET "\n");
}

static uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
static uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2
static uint8_t r02[4] = {0x1F, 0x3F, 0x40, 0x00}; // CFR3
static uint8_t r03[4] = {0x00, 0x00, 0x7F, 0x7F}; // Aux DAC control
static uint8_t r04[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // I/O Update Rate
static uint8_t r05[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
static uint8_t r06[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
static uint8_t r07[4] = {0x00, 0x00, 0x00, 0x00}; // FTW
static uint8_t r08[2] = {0x00, 0x00}; // POW
static uint8_t r09[4] = {0x00, 0x00, 0x00, 0x00}; // ASF
static uint8_t r0A[4] = {0x00, 0x00, 0x00, 0x00}; // Multichip Sync
static uint8_t r0B[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Limit
static uint8_t r0C[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Step
static uint8_t r0D[4] = {0x00, 0x00, 0x00, 0x00}; // Digital Ramp Rate
static uint8_t r0E[8] = {0x08, 0xB5, 0x00, 0x00, 0x0C, 0xCC, 0xCC, 0xCD}; // Profile 0
static uint8_t r0F[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 1
static uint8_t r10[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 2
static uint8_t r11[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 3
static uint8_t r12[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 4
static uint8_t r13[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 5
static uint8_t r14[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 6
static uint8_t r15[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 7
static uint8_t r16[4] = {0x00, 0x00, 0x00, 0x00}; // RAM

static uint8_t* regmap[23] = {
	r00,
	r01,
	r02,
	r03,
	r04,
	r05,
	r06,
	r07,
	r08,
	r09,
	r0A,
	r0B,
	r0C,
	r0D,
	r0E,
	r0F,
	r10,
	r11,
	r12,
	r13,
	r14,
	r15,
	r16
};

static uint32_t ad_system_clock = 0;

void ad_init() {
    ad_init_gpio();

	// PLL
	r02[0] = 0x0D; // VCO + XTAL out enable/disable
	r02[1] = 0x3F; // Charge pump current
	r02[2] = 0xC1; // Divider disable + PLL enable
	r02[3] = 0x64; // x50 multiplier

	//r02[2] = 0xC0; // Divider disable + PLL disable
	
    ad_system_clock = 1000*1000*1000;

	r00[3] = 0x02; // ???
	
	r00[2] = 0b00100000; // Autoclear phase
	r01[3] = 0b10000000; // Matched latency
	r00[1] = 0b00000001; // Sine output
	
	// Записать измененные регистры
	ad_write(0x00, r00, 4);
	ad_write(0x01, r01, 4);
	ad_write(0x02, r02, 4);
	
	my_delay(1024);
	ad_pulse_io_update();
}

// Включить блок масштабирования амплитуды
// По умолчанию выключен в целях энергосбережения
// Необходимо для ad_set_profile_amplitude()
void ad_enable_amplitude_scaler() {
	r01[0] = 0x01;
}

// Прочитать и сверить содержимое всех регистров с ожидаемыми значениями
void ad_readback_all() {
	printf("Full readback\n");
	
	ad_readback(0x00, r00, 4);
	ad_readback(0x01, r01, 4);
	ad_readback(0x02, r02, 4);
	ad_readback(0x03, r03, 4);
	ad_readback(0x04, r04, 4);
	//ad_readback(0x05, r05, 6);
	//ad_readback(0x06, r06, 6);
	ad_readback(0x07, r07, 4);
	ad_readback(0x08, r08, 2);
	ad_readback(0x09, r09, 4);
	ad_readback(0x0A, r0A, 4);
	ad_readback(0x0B, r0B, 8);
	ad_readback(0x0C, r0C, 8);
	ad_readback(0x0D, r0D, 4);
	
	ad_readback(0x0E, r0E, 8);
	ad_readback(0x0F, r0F, 8);
	ad_readback(0x10, r10, 8);
	ad_readback(0x11, r11, 8);
	ad_readback(0x12, r12, 8);
	ad_readback(0x13, r13, 8);
	ad_readback(0x14, r14, 8);
	ad_readback(0x15, r15, 8);
	
	// Не пытаться сверять оперативную память
	//ad_readback(0x16, r16, 4);

	printf("Full readback complete\n");
}

// Записать все регистры
void ad_write_all() {
	printf("Full write\n");
	
	ad_write(0x00, r00, 4);
	ad_write(0x01, r01, 4);
	ad_write(0x02, r02, 4);
	ad_write(0x03, r03, 4);
	ad_write(0x04, r04, 4);
	//ad_write(0x05, r05, 6);
	//ad_write(0x06, r06, 6);
	ad_write(0x07, r07, 4);
	ad_write(0x08, r08, 2);
	ad_write(0x09, r09, 4);
	ad_write(0x0A, r0A, 4);
	ad_write(0x0B, r0B, 8);
	ad_write(0x0C, r0C, 8);
	ad_write(0x0D, r0D, 4);
	
	ad_write(0x0E, r0E, 8);
	ad_write(0x0F, r0F, 8);
	ad_write(0x10, r10, 8);
	ad_write(0x11, r11, 8);
	ad_write(0x12, r12, 8);
	ad_write(0x13, r13, 8);
	ad_write(0x14, r14, 8);
	ad_write(0x15, r15, 8);
	
	//ad_write(0x16, r16, 4);
	
	printf("Full write complete\n");
}

// Тест с переключением SYNC_CLK
// Полезно для тестирования задержек
void ad_toggle_sync_clk() {	
	static uint8_t cnt = 1;
	
	if (cnt % 2) {
		r01[1] &= 0x00;
	} else {
		r01[1] |= 0xBF;
	}
	cnt++;
	
	ad_write(0x01, r01, 4);
	
	my_delay(1024);
	ad_pulse_io_update();
}

uint32_t ad_calc_ftw(uint32_t freq_hz) {
	double ratio = (double)freq_hz / (double)ad_system_clock;
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);
	
	// Нельзя выдавать частоты выше половины ad_system_clock
	if (ftw > 2147483647) {
		ftw = 2147483647;
		printf("Frequency too high\n");
	}
	
	return ftw;
}

// Установить частоту в указанном профиле
void ad_set_profile_freq(int profile_id, uint32_t freq_hz) {
	uint8_t* profile = regmap[14 + profile_id];
	uint32_t ftw = ad_calc_ftw(freq_hz);
	
	uint8_t* view = (uint8_t*)&ftw;
	
	printf("Profile %d freq: %x%x%x%x\n", profile_id, view[3], view[2], view[1], view[0]);
	
	profile[7] = view[0];
	profile[6] = view[1];
	profile[5] = view[2];
	profile[4] = view[3];
}

// Установить амплитуду в указанном профиле
void ad_set_profile_amplitude(int profile_id, uint16_t amplitude) {
	uint8_t* profile = regmap[14 + profile_id];
	
	uint8_t* view = (uint8_t*)&amplitude;
	
	printf("Profile %d amplitude: %x%x\n", profile_id, view[1], view[0]);
	
	profile[1] = view[0];
	profile[0] = view[1] & 0x3F;
}

void ad_set_profile_phase(int profile_id, uint16_t phase) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&phase;
	
	profile[2] = view[0];
	profile[3] = view[1];
}

//
// Digital ramp
//

// Включить Digital Ramp Generator в режиме частоты
// Частота в профилях перестанет иметь эффект (Но амплитуда всё ещё продолжит применяться в соответствии с профилем)
void ad_enable_ramp() {
	r01[1] &= ~0b00111110; // Занулить все связанные с DRG биты
	r01[1] |=  0b00001000; // Включить в режиме частоты

	r00[2] &= ~0b11000100; // Занулить все связанные с DRG биты
	r00[2] |=  0b01000000; // Автосброс таймера (На нижний лимит)
}

// Установка начального и конечного значения
// Значения можно получить при помощи ad_calc_ftw()
void ad_set_ramp_limits(uint32_t lower, uint32_t upper) {
	uint8_t* view_u = (uint8_t*)&upper;
	uint8_t* view_l = (uint8_t*)&lower;

	r0B[0] = view_u[3];
	r0B[1] = view_u[2];
	r0B[2] = view_u[1];
	r0B[3] = view_u[0];

	r0B[4] = view_l[3];
	r0B[5] = view_l[2];
	r0B[6] = view_l[1];
	r0B[7] = view_l[0];
}

// Установка шага
// Тоже подойдут значения от ad_calc_ftw()
void ad_set_ramp_step(uint32_t decrement, uint32_t increment) {
	uint8_t* view_d = (uint8_t*)&decrement;
	uint8_t* view_i = (uint8_t*)&increment;

	r0C[0] = view_d[3];
	r0C[1] = view_d[2];
	r0C[2] = view_d[1];
	r0C[3] = view_d[0];

	r0C[4] = view_i[3];
	r0C[5] = view_i[2];
	r0C[6] = view_i[1];
	r0C[7] = view_i[0];
}

// Установка задержки между шагами
// Формула: 1s / SYSCLK/4
void ad_set_ramp_rate(uint16_t down_rate, uint16_t up_rate) {
	uint8_t* view_d = (uint8_t*)&down_rate;
	uint8_t* view_u = (uint8_t*)&up_rate;

	r0D[0] = view_d[1];
	r0D[1] = view_d[0];

	r0D[2] = view_u[1];
	r0D[3] = view_u[0];
}

void ad_test_tone() {
    ad_set_profile_freq(0, 15*1000*1000);
    ad_set_profile_amplitude(0, 0x3FFF);
    ad_write_all();
    timer2_stop();
}

//
// Чтение/запись оперативной памяти AD9910
//
// Напечатать дамп содержимого оперативной памяти
// Начальный и конечный адрес зависят от содержимого регистров профилей
// void ad_dump_ram(uint16_t size) {
// 	uint8_t instruction = 0x80 | 0x16;
// 	uint8_t read[4] = {0};
	
// 	ad_select();
// 	my_delay(500);
	
// 	for (int j = 0; j < size; j++) {
// 		spi_send(&instruction, 1);
// 		my_delay(500);
		
// 		for (int i = 0; i < 4; i++) {
// 			spi_recv(&read[i], 1);
// 			my_delay(500);
// 		}
// 		ad_deselect();
		
// 		print("ad9910.c: ram: ");
		
// 		for (int i = 0; i < 4; i++) {
// 			print_hex(read[i]);
// 			print(" ");
// 		}
		
// 		print("\r\n");
// 	}
// }

// void ad_ram_enable() {
// 	r00[0] = 0x80;
// }

// void ad_ram_disable() {
// 	r00[0] = 0x00;
// }

// // Установить адрес старта и окнчания в профиле для оперативной памяти
// void ad_set_ram_profile_address(int profile_id, uint16_t start, uint16_t stop) {
// 	uint8_t* profile = regmap[14 + profile_id];

// 	// Stop
// 	profile[3] = (stop >> 2) & 0xFF;
// 	profile[4] = (stop << 6) & 0xFF;
	
// 	// Start
// 	profile[5] = (start >> 2) & 0xFF;
// 	profile[6] = (start << 6) & 0xFF;
// }

// // Установить скорость в профиле для оперативной памяти
// void ad_set_ram_profile_step_rate(int profile_id, uint16_t step_rate) {
// 	uint8_t* profile = regmap[14 + profile_id];
// 	uint8_t* view = (uint8_t*)&step_rate;
	
// 	profile[1] = view[0];
// 	profile[2] = view[1];
// }

// // Настроить режим профиля для оперативной памяти
// // Пока что просто включить режим Continuous Bidirectional Ramp, 0b011
// void ad_set_ram_profile_mode(int profile_id) {
// 	uint8_t* profile = regmap[14 + profile_id];
// 	profile[7] = 0b011;
// }

// void ad_run_ram_test() {
// 	timer_stop();
// }

//
// Коллекция карт регистров
//

//
// Состояние после сброса
//
// ===================================================================================================================
/*
uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2
uint8_t r02[4] = {0x1F, 0x3F, 0x40, 0x00}; // CFR3
uint8_t r03[4] = {0x00, 0x00, 0x7F, 0x7F}; // Aux DAC control
uint8_t r04[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // I/O Update Rate
//uint8_t r05[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
//uint8_t r06[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
uint8_t r07[4] = {0x00, 0x00, 0x00, 0x00}; // FTW
uint8_t r08[2] = {0x00, 0x00}; // POW
uint8_t r09[4] = {0x00, 0x00, 0x00, 0x00}; // ASF
uint8_t r0A[4] = {0x00, 0x00, 0x00, 0x00}; // Multichip Sync
uint8_t r0B[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Limit, значения после сброса N/A
uint8_t r0C[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Step, значения после сброса N/A
uint8_t r0D[4] = {0x00, 0x00, 0x00, 0x00}; // Digital Ramp Rate

uint8_t r0E[8] = {0x08, 0xB5, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 0
uint8_t r0F[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 1
uint8_t r10[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 2
uint8_t r11[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 3
uint8_t r12[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 4
uint8_t r13[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 5
uint8_t r14[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 6
uint8_t r15[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 7

uint8_t r16[4] = {0x00, 0x00, 0x00, 0x00}; // RAM
*/
// ===================================================================================================================

//
// Внешний источник 200 МГц
// На выходе 5 МГц
//
// ===================================================================================================================
/*
uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2
uint8_t r02[4] = {0x1F, 0x3F, 0x40, 0x00}; // CFR3
uint8_t r03[4] = {0x00, 0x00, 0x7F, 0x7F}; // Aux DAC control
uint8_t r04[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // I/O Update Rate
//uint8_t r05[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
//uint8_t r06[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
uint8_t r07[4] = {0x00, 0x00, 0x00, 0x00}; // FTW
uint8_t r08[2] = {0x00, 0x00}; // POW
uint8_t r09[4] = {0x00, 0x00, 0x00, 0x00}; // ASF
uint8_t r0A[4] = {0x00, 0x00, 0x00, 0x00}; // Multichip Sync
uint8_t r0B[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Limit
uint8_t r0C[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Digital Ramp Step
uint8_t r0D[4] = {0x00, 0x00, 0x00, 0x00}; // Digital Ramp Rate

uint8_t r0E[8] = {0x08, 0xB5, 0x00, 0x00, 0x0C, 0xCC, 0xCC, 0xCD}; // Profile 0
uint8_t r0F[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 1
uint8_t r10[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 2
uint8_t r11[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 3
uint8_t r12[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 4
uint8_t r13[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 5
uint8_t r14[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 6
uint8_t r15[8] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // Profile 7

uint8_t r16[4] = {0x00, 0x00, 0x00, 0x00}; // RAM
*/
// ===================================================================================================================