#include <stdint.h>

void set_profile(uint8_t profile_id);
void ad_init();
void ad_readback_all();
void ad_write_all();
void ad_enable_amplitude_scaler();
void ad_set_profile_freq(int profile_id, uint32_t freq_hz);
void ad_set_profile_amplitude(int profile_id, uint16_t amplitude);
void ad_test_tone();
void ad_enable_ramp();
void ad_disable_ramp();
void ad_set_ramp_limits(uint32_t lower_hz, uint32_t upper_hz);
void ad_set_ramp_step(uint32_t decrement, uint32_t increment);
void ad_set_ramp_rate(uint16_t down_rate, uint16_t up_rate);
