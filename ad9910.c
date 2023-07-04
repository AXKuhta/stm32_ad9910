
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ad9910.h"
#include "vt100.h"

// Текущая тактовая частота AD9910
// Используется при вычислении FTW
uint32_t ad_system_clock = 0;

// Установить общую частоту при использовании оперативной памяти
// Действует когда RAM Playback Destination = Phase или Amplitude
void ad_set_ram_freq(uint32_t ftw) {
	uint8_t* view = (uint8_t*)&ftw;

	r07[3] = view[0];
	r07[2] = view[1];
	r07[1] = view[2];
	r07[0] = view[3];
}

// Установить общий сдвиг фазы при использовании оперативной памяти
// Действует когда RAM Playback Destination = Frequency или Amplitude
void ad_set_ram_phase(uint16_t pow) {
	uint8_t* view = (uint8_t*)&pow;

	r08[1] = view[0];
	r08[0] = view[1];
}

// Установить общую амплитуду при использовании оперативной памяти
// Не действует, пока выставлен бит Enable amplitude scale from single tone profiles
// Формат несколько отличается от того, что в регистрах профилей
void ad_set_ram_amplitude(uint16_t asf) {
	assert(asf <= 0x3FFF);

	r09[3] = (asf << 2) & 0xFF;
	r09[2] = (asf >> 5) & 0xFF;
}

void ad_init() {
	ad_init_gpio();
	set_profile(0);

	// Ramp Generator
	set_ramp_direction(1);

	// SDIO Input Only
	r00[3] = 0x02;

	// PLL
	r02[0] = 0x0D; // VCO + XTAL out enable/disable
	r02[1] = 0x3F; // Charge pump current
	r02[2] = 0xC1; // Divider disable + PLL enable
	r02[3] = 0x64; // x50 multiplier

	//r02[2] = 0xC0; // Divider disable + PLL disable
	
	ad_system_clock = 1000*1000*1000;
	
	r01[0] = 0b00000001; // Enable amplitude scale from single tone profiles
	r00[2] = 0b00001000; // Phase static reset
	r01[3] = 0b10000000; // Matched latency
	r00[1] = 0b00000001; // Sine output


	// Записать измененные регистры
	ad_write(0x00, r00, 4);
	ad_write(0x01, r01, 4);
	ad_write(0x02, r02, 4);
	
	ad_pulse_io_update();
}
