
// GPIO
uint16_t profile_to_gpio_states(uint8_t profile_id);
void set_profile(uint8_t profile_id);
void set_ramp_direction(uint8_t direction);
void ad_pulse_io_update();

// SPI
void ad_readback_all();
void ad_write_all();

// Профили
void ad_set_profile_freq(int profile_id, uint32_t freq_hz);
void ad_set_profile_amplitude(int profile_id, uint16_t amplitude);
void ad_set_profile_phase(int profile_id, uint16_t phase);
void ad_set_ram_profile(int profile_id, uint16_t step_rate, uint16_t start, uint16_t end);
void ad_set_ram_freq(uint32_t freq_hz);
void ad_set_ram_phase(uint16_t phase);
void ad_set_ram_amplitude(uint16_t amplitude);

// DRG
void ad_enable_ramp();
void ad_disable_ramp();
void ad_set_ramp_limits(uint32_t lower_hz, uint32_t upper_hz);
void ad_set_ramp_step(uint32_t decrement, uint32_t increment);
void ad_set_ramp_rate(uint16_t down_rate, uint16_t up_rate);

// RAM
#define AD_RAM_DESTINATION_FREQ 		0
#define AD_RAM_DESTINATION_PHASE 		1 // Не будет работать, пока выставлен бит Enable amplitude scale from single tone profiles (так как амплитуда будет нулевая)
#define AD_RAM_DESTINATION_AMPLITUDE 	2
#define AD_RAM_DESTINATION_POLAR 		3

void ad_enable_ram();
void ad_disable_ram();
void ad_set_ram_destination(uint8_t destination);
void ad_write_ram(uint32_t* buffer, size_t size);
void ad_read_ram(uint32_t* buffer, size_t size);
void ad_ram_test();

// Прочее
void ad_drop_phase_static_reset();
void ad_init();

typedef struct ad_ramp_t {
	uint32_t fstep_ftw;
	uint32_t tstep_mul;
} ad_ramp_t;

uint32_t ad_calc_ftw(uint32_t freq_hz);
ad_ramp_t ad_calc_ramp(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns);
