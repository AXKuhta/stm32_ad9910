#include <stdint.h>

#include "ad9910/registers.h"
#include "ad9910/profiles.h"

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
