
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

// RAM
#define AD_RAM_DESTINATION_FREQ 		0
#define AD_RAM_DESTINATION_PHASE 		1 // Не будет работать, пока выставлен бит Enable amplitude scale from single tone profiles (так как амплитуда будет нулевая)
#define AD_RAM_DESTINATION_AMPLITUDE 	2
#define AD_RAM_DESTINATION_POLAR 		3

#define AD_RAM_PROFILE_MODE_DIRECTSWITCH 0b00000000
#define AD_RAM_PROFILE_MODE_ZEROCROSSING 0b00001000 // Если включен zero-crossing, то режим обязательно direct switch

void ad_enable_ram();
void ad_disable_ram();
void ad_set_ram_destination(uint8_t destination);
void ad_write_ram(uint32_t* buffer, size_t count);
void ad_read_ram(uint32_t* buffer, size_t count);
void ad_ram_test();

// Прочее
void ad_drop_phase_static_reset();
void ad_init();

#include "ad9910/drg.h"
#include "ad9910/units.h"
