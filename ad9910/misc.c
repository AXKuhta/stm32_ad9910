#include <stdint.h>

#include "ad9910/registers.h"
#include "ad9910/pins.h"
#include "ad9910/misc.h"

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
