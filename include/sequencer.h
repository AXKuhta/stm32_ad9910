
void enter_rfkill_mode();
void enter_test_tone_mode(uint32_t freq_hz);
void enter_basic_pulse_mode(uint32_t t0_ns, uint32_t t1_ns, uint32_t freq_hz);
void enter_basic_sweep_mode(uint32_t t1_ns, uint32_t t2_ns, uint32_t f1_hz, uint32_t f2_hz);
void sequencer_init();
void sequencer_reset();
void sequencer_show();
void sequencer_add(pulse_t pulse);
void sequencer_run();
void sequencer_stop();
