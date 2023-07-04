
// GPIO
uint16_t profile_to_gpio_states(uint8_t profile_id);
void set_profile(uint8_t profile_id);
void set_ramp_direction(uint8_t direction);
void ad_pulse_io_update();
void ad_pulse_io_reset();
void ad_init_gpio();
void my_delay(uint32_t delay);
