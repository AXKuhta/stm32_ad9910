
uint16_t profile_to_gpio_states(uint8_t profile_id);
void set_profile(uint8_t profile_id);
void set_ramp_direction(uint8_t direction);
void ad_pulse_io_update();
void ad_readback_all();
void ad_write_all();
void ad_enable_amplitude_scaler();
void ad_set_profile_freq(int profile_id, uint32_t freq_hz);
void ad_set_profile_amplitude(int profile_id, uint16_t amplitude);
void ad_set_profile_phase(int profile_id, uint16_t phase);
void ad_set_ram_profile(int profile_id, uint16_t step_rate, uint16_t start, uint16_t end);
void ad_ram_test();
void ad_enable_ramp();
void ad_disable_ramp();
void ad_set_ramp_limits(uint32_t lower_hz, uint32_t upper_hz);
void ad_set_ramp_step(uint32_t decrement, uint32_t increment);
void ad_set_ramp_rate(uint16_t down_rate, uint16_t up_rate);
void ad_enable_ram();
void ad_disable_ram();
void ad_drop_phase_static_reset();
void ad_init();

typedef struct ad_ramp_t {
	uint32_t fstep_ftw;
	uint32_t tstep_mul;
} ad_ramp_t;

uint32_t ad_calc_ftw(uint32_t freq_hz);
ad_ramp_t ad_calc_ramp(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns);
