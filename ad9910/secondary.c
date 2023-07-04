#include <stdint.h>
#include <assert.h>

#include "ad9910/registers.h"
#include "ad9910/secondary.h"

//
// Вторичные источники параметров сигнала
//

// Установить вторичный источник частоты
// Действует когда RAM Playback Destination = Phase или Amplitude
void ad_set_secondary_freq(uint32_t ftw) {
	uint8_t* view = (uint8_t*)&ftw;

	r07[3] = view[0];
	r07[2] = view[1];
	r07[1] = view[2];
	r07[0] = view[3];
}

// Установить вторичный источник фазы
// Действует когда RAM Playback Destination = Frequency или Amplitude
void ad_set_secondary_phase(uint16_t pow) {
	uint8_t* view = (uint8_t*)&pow;

	r08[1] = view[0];
	r08[0] = view[1];
}

// Установить вторичный источник амплитуды
// Не действует, пока выставлен бит Enable amplitude scale from single tone profiles
// Формат несколько отличается от того, что в регистрах профилей
void ad_set_secondary_amplitude(uint16_t asf) {
	assert(asf <= 0x3FFF);

	r09[3] = (asf << 2) & 0xFF;
	r09[2] = (asf >> 5) & 0xFF;
}
