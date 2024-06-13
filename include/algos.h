
sweep_t calculate_sweep(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns);
sweep_t calculate_sweep_v2(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns);
uint16_t fit_time(uint32_t time_ns);
int best_asf_fsc(double voltage_vrms, uint16_t* asfp, uint8_t* fscp);
