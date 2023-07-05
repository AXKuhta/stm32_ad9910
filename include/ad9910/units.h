
typedef struct ad_ramp_t {
	uint32_t fstep_ftw;
	uint32_t tstep_mul;
} ad_ramp_t;

uint32_t ad_calc_ftw(double freq_hz);
ad_ramp_t ad_calc_ramp(uint32_t f1_hz, uint32_t f2_hz, uint32_t time_ns);

// Обратное преобразование значений
double ad_backconvert_ftw(uint32_t ftw);
double ad_backconvert_pow(uint16_t pow);
double ad_backconvert_asf(uint16_t asf);
double ad_backconvert_step_time(uint16_t step_time);
