
// Профили
void ad_set_profile_freq(int profile_id, uint32_t ftw);
void ad_set_profile_amplitude(int profile_id, uint16_t pow);
void ad_set_profile_phase(int profile_id, uint16_t asf);
void ad_set_ram_profile(int profile_id, uint16_t step_rate, uint16_t start, uint16_t end, uint8_t mode);
void ad_set_ram_freq(uint32_t ftw);
void ad_set_ram_phase(uint16_t pow);
void ad_set_ram_amplitude(uint16_t asf);

// Прочее
void ad_drop_phase_static_reset();
void ad_init();

#include "ad9910/registers.h"
#include "ad9910/pins.h"
#include "ad9910/drg.h"
#include "ad9910/units.h"
#include "ad9910/ram.h"
