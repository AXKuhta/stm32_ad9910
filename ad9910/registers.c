#include <stdint.h>
#include <stdio.h>

#include "ad9910/registers.h"
#include "ad9910/pins.h"
#include "vt100.h"
#include "spi.h"

// Регистры AD9910
// Состояние после сброса
uint8_t r00[4] = {0x00, 0x00, 0x00, 0x00}; // CFR1
uint8_t r01[4] = {0x00, 0x40, 0x08, 0x20}; // CFR2
uint8_t r02[4] = {0x1F, 0x3F, 0x40, 0x00}; // CFR3
uint8_t r03[4] = {0x00, 0x00, 0x7F, 0x7F}; // Aux DAC control
uint8_t r04[4] = {0xFF, 0xFF, 0xFF, 0xFF}; // I/O Update Rate
uint8_t r05[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
uint8_t r06[6] = {0x00, 0x00, 0x00, 0x00, 0x00, 0x00}; // ???
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

uint8_t* regmap[23] = {
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

void ad_write(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = reg_addr & 0x1F;
	
	spi_send(&instruction, 1);

	for (uint16_t i = 0; i < size; i++) {
		spi_send(&buffer[i], 1);
	}
}

//
// Чтение регистров AD9910
//
void ad_readback(uint8_t reg_addr, uint8_t* buffer, uint16_t size) {
	uint8_t instruction = 0x80 | (reg_addr & 0x1F);
	uint8_t read[8] = {0};
	
	spi_send(&instruction, 1);
	
	for (int i = 0; i < size; i++) {
		spi_recv(&read[i], 1);
	}
	
	printf("ad9910.c: register 0x%02X: ", reg_addr);
	
	for (int i = 0; i < size; i++) {
		printf(COLOR_GREEN "%s0x%02X ", buffer[i] == read[i] ? COLOR_GREEN : COLOR_RED, read[i]);
	}
	
	printf(COLOR_RESET "\n");
}

// Прочитать и сверить содержимое всех регистров с ожидаемыми значениями
// Не трогать недокументированные регистры 0x05 и 0x06
// Не пытаться сверять оперативную память
void ad_readback_all() {
	printf("Full readback\n");

	ad_pulse_io_reset();
	
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
	
	//ad_readback(0x16, r16, 4);

	printf("Full readback complete\n");
}

// Записать все регистры
// Не трогать недокументированные регистры 0x05 и 0x06
// Не пытаться записывать в оперативную память
void ad_write_all() {	
	ad_pulse_io_reset();

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
}
