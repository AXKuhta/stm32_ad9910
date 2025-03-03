#include <stdint.h>

#include "ad9910/registers.h"
#include "ad9910/pins.h"
#include "ad9910/misc.h"

void ad_set_full_scale_current(uint8_t fsc) {
	r03[03] = fsc;
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

// Выключить:
// - static reset фазы 
// - static reset drg (генератора свипов)
//
// И включить:
// - воспроизведение из оперативной памяти (если память задействована)
//
// Не изменяет снимок регистров; там предполагается, что:
// - static reset фазы всегда включен
// - static reset drg всегда включен
// - воспроизведение из памяти - всегда выключено
//
// Пока включен static reset генератора свипов, его выход принимает значение нижней границы
//
// Биты будут приняты при первом переключении профиля
// То есть после схода с парковочного профиля в момент начала излучения импульса
void ad_safety_off(int ram_playback_enable) {
	uint8_t altered_r00[4] = {
		r00[0] | (ram_playback_enable ? 0b10000000 : 0),
		r00[1],
		r00[2] & ~0b00011000,
		r00[3]
	};

	ad_write(0x00, altered_r00, 4);
}
