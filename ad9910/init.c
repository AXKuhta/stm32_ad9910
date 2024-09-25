#include <stdint.h>
#include <assert.h>

#include "ad9910/registers.h"
#include "ad9910/pins.h"

// Текущая тактовая частота AD9910
// Используется при вычислении FTW
uint32_t ad_system_clock = 0;

// Используемое по умолчанию значение амплитуды (ASF)
// Максимальным допустимым значением является 0x3FFF
uint16_t ad_default_asf = 1024;

// Используемое по умолчанию значение тока ЦАПа (FSC)
uint8_t ad_default_fsc = 0;

static struct { uint32_t min, max; } vco_ranges[] = {
	{ .min = 420000000, .max = 485000000 },
	{ .min = 482000000, .max = 562000000 },
	{ .min = 562000000, .max = 656000000 },
	{ .min = 656000000, .max = 832000000 },
	{ .min = 832000000, .max = 920000000 },
	{ .min = 920000000, .max = 1080000000 },
};

static int select_vco(uint32_t sysclk) {
	for (int i = 0; i < 6; i++) {
		if (sysclk >= vco_ranges[i].min && sysclk <= vco_ranges[i].max)
			return i;
	}

	assert(0);
}

static void ad_enable_pll(uint32_t refclk, uint8_t multiplier) {
	uint32_t sysclk = refclk * multiplier;
	uint8_t vco = select_vco(sysclk);
	uint8_t icp = 7;

	assert(icp < 8);

	r02[0] = 0b00011000 + vco; 	// XTAL out enable (Drive strength low) + VCO
	r02[1] = icp << 3; 			// Charge pump current
	r02[2] = 0b11000001; 		// Divider disable + PLL enable
	r02[3] = multiplier << 1;

	ad_system_clock = sysclk;
}

void ad_init() {
	ad_init_gpio();
	set_profile(0);

	// Ramp Generator
	set_ramp_direction(1);

	// SDIO Input Only
	r00[3] = 0x02;

	ad_enable_pll(25*1000*1000, 40);
	
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
