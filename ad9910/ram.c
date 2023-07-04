#include <stdint.h>
#include <stddef.h>
#include <stdlib.h>
#include <assert.h>
#include <stdio.h>

#include "ad9910/registers.h"
#include "ad9910/ram.h"
#include "vt100.h"
#include "spi.h"

//
// AD9910 RAM
//

void ad_enable_ram() {
	r00[0] |=  0b10000000;
}

void ad_disable_ram() {
	r00[0] &= ~0b10000000;
}

void ad_set_ram_destination(uint8_t destination) {
	r00[0] &= ~0b01100000;
	r00[0] |= destination << 5;
}

// Записать count 32-битных слов в оперативную память AD9910
// Запись происходит с конца, т.е. с count - 1
void ad_write_ram(uint32_t* buffer, size_t count) {
	assert(count <= 1024);

	ad_set_ram_profile(0, 0, 0, count - 1, AD_RAM_PROFILE_MODE_DIRECTSWITCH);
	ad_disable_ram();
	ad_write_all();
	ad_pulse_io_update();
	set_profile(0);

	uint8_t instruction = 0x16;
	spi_send(&instruction, 1);

	for (size_t i = 0; i < count; i++) {
		spi_send((uint8_t*)&buffer[count - 1 - i], 4);
	}
}

// Прочитать count 32-битных слов из оперативной памяти AD9910
// Чтение тоже происходит с конца, т.е. count - 1
void ad_read_ram(uint32_t* buffer, size_t count) {
	assert(count <= 1024);

	ad_set_ram_profile(0, 0, 0, count - 1, AD_RAM_PROFILE_MODE_DIRECTSWITCH);
	ad_disable_ram();
	ad_write_all();
	ad_pulse_io_update();
	set_profile(0);

	uint8_t instruction = 0x80 | 0x16;
	spi_send(&instruction, 1);

	for (size_t i = 0; i < count; i++) {
		spi_recv((uint8_t*)&buffer[count - 1 - i], 4);
	}
}

// Протестировать оперативную память
void ad_ram_test() {
	uint8_t* buffer_a = malloc(4096);
	uint8_t* buffer_b = malloc(4096);

	for (size_t i = 0; i < 4096; i++) {
		buffer_a[i] = i & 0xFF;
	}

	ad_write_ram((uint32_t*)buffer_a, 1024);
	ad_read_ram((uint32_t*)buffer_b, 1024);

	for (size_t i = 0; i < 4096; i++) {
		printf(COLOR_GREEN "%s0x%02X ", buffer_a[i] == buffer_b[i] ? COLOR_GREEN : COLOR_RED, buffer_b[i]);

		if (((1+i) % 16) == 0) printf("\n"); 
	}

	printf(COLOR_RESET "\n");

	free(buffer_a);
	free(buffer_b);
}
