#include "stm32f7xx_hal.h"
#include "definitions.h"

// PD14 CS
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
	
	// Chip Select
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_14, GPIO_PIN_SET);
	
	// IO_RESET по умолчанию единичка
	HAL_GPIO_WritePin(GPIOD, GPIO_PIN_15, GPIO_PIN_RESET);
	
	// IO_UPDATE в начале ноль
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
		
	//print("write instr: ");
	//print_hex(instruction);
	//print("\n");
	
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
	
	print("ad9910.c: register "); print_hex(reg_addr); print(": ");
	
	for (int i = 0; i < size; i++) {
		if (buffer[i] == read[i]) {
			print(COLOR_GREEN " ");
			print_hex(read[i]);
		} else {
			print(COLOR_RED " ");
			print_hex(read[i]);
		}
	}
	
	print(COLOR_RESET "\r\n");
}


//
// External clock 200 MHz
// Output clock 5 MHz
//
static uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
static uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2 // uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20};
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

void ad_init() {	
	// Включить трёхпроводной режим
	r00[3] = 0x02;
	
	// Выключить делитель
	r02[2] = 0xC0;
	
	ad_write(0x00, r00, 4);
	my_delay(1024);
	ad_pulse_io_update();

}

void ad_readback_all() {
	print("Full readback\r\n");
	
	
	//ad_readback(0x02, r02, 4);
	
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
	
	ad_readback(0x16, r16, 4);
	
	

	print("Full readback complete\r\n");
}

// 
void ad_write_all() {
	print("Full write\r\n");
	
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
	
	ad_write(0x16, r16, 4);
	
	print("Full write complete\r\n");
}

// Тест с переключением SYNC_CLK
// Полезно для тестирования задержек
void ad_test() {	
	static uint8_t cnt = 1;
	
	if (cnt % 2) {
		r01[1] = 0x00;
	} else {
		r01[1] = 0x40;
	}
	cnt++;
	
	ad_write(0x01, r01, 4);
	
	my_delay(1024);
	ad_pulse_io_update();
}


static uint32_t external_clock = 0;

void ad_external_clock(uint32_t value) {
	external_clock = value;
}

uint32_t ad_calc_ftw(uint32_t freq_hz) {
	double ratio = (double)freq_hz / (double)external_clock;
	uint32_t ftw = (uint32_t)(4294967296.0 * ratio + 0.5);
	
	return ftw;
}

void ad_set_profile_freq(int profile_id, uint32_t freq_hz) {
	uint8_t* profile = regmap[14 + profile_id];
	uint32_t ftw = ad_calc_ftw(freq_hz);
	
	uint8_t* view = (uint8_t*)&ftw;
	
	print("Calculated freq: ");
	print_hex(view[3]); print(" ");
	print_hex(view[2]); print(" ");
	print_hex(view[1]); print(" ");
	print_hex(view[0]); print(" ");
	print("\r\n");
	
	profile[7] = view[0];
	profile[6] = view[1];
	profile[5] = view[2];
	profile[4] = view[3];
}

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
