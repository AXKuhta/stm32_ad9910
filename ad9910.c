
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ad9910.h"
#include "vt100.h"

// Текущая тактовая частота AD9910
// Используется при вычислении FTW
uint32_t ad_system_clock = 0;

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
