
#include <stdlib.h>
#include <stdio.h>
#include <assert.h>

#include "ad9910.h"
#include "vt100.h"

// Текущая тактовая частота AD9910
// Используется при вычислении FTW
uint32_t ad_system_clock = 0;

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

//
// Профили
//

// Установить частоту в указанном профиле
void ad_set_profile_freq(int profile_id, uint32_t ftw) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&ftw;
	
	profile[7] = view[0];
	profile[6] = view[1];
	profile[5] = view[2];
	profile[4] = view[3];
}

// Установить амплитуду в указанном профиле
void ad_set_profile_amplitude(int profile_id, uint16_t asf) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&asf;
	
	profile[1] = view[0];
	profile[0] = view[1] & 0x3F;
}

// Установить сдвиг фазы в указанном профиле
void ad_set_profile_phase(int profile_id, uint16_t pow) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view = (uint8_t*)&pow;
	
	profile[3] = view[0];
	profile[2] = view[1];
}

// Установить в указанном профиле адрес начала, адрес конца и интервал шагов по оперативной памяти
void ad_set_ram_profile(int profile_id, uint16_t step_rate, uint16_t start, uint16_t end, uint8_t mode) {
	uint8_t* profile = regmap[14 + profile_id];
	uint8_t* view_step_rate = (uint8_t*)&step_rate;

	profile[2] = view_step_rate[0];
	profile[1] = view_step_rate[1];

	profile[4] = (end << 6) & 0b11000000;
	profile[3] = (end >> 2);

	profile[6] = (start << 6) & 0b11000000;
	profile[5] = (start >> 2);

	profile[7] = mode;
}

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

// Выключить static reset фазы
// Не изменяет снимок регистров; предполагается, что там static reset всегда должен быть включен
// Бит будет применён при первом переключении профиля
void ad_drop_phase_static_reset() {
	uint8_t altered_r00[4] = {
		r00[0],
		r00[1],
		r00[2] & ~0b00001000,
		r00[3]
	};

	ad_write(0x00, altered_r00, 4);
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
