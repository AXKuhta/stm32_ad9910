
// GPIO
void set_profile(uint8_t profile_id);
void set_ramp_direction(uint8_t direction);
void ad_pulse_io_update();
void ad_pulse_io_reset();
void ad_init_gpio();
void ad_slave_to_arm();
void ad_slave_to_tim();
void my_delay(uint32_t delay);
