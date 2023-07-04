
// GPIO
uint16_t profile_to_gpio_states(uint8_t profile_id);
void set_profile(uint8_t profile_id);
void set_ramp_direction(uint8_t direction);
void ad_pulse_io_update();

// SPI
void ad_readback_all();
void ad_write_all();

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

#include "ad9910/drg.h"
#include "ad9910/units.h"
#include "ad9910/ram.h"
