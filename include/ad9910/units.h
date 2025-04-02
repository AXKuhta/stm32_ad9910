
uint32_t ad_calc_ftw(double freq_hz);

// Обратное преобразование значений
double ad_backconvert_ftw(uint32_t ftw);
double ad_backconvert_pow(uint16_t pow);
double ad_backconvert_asf(uint16_t asf);
double ad_backconvert_step_time(uint16_t step_time);

// Вычисление уровня сигнала
#define SQRT2 1.41421356237309504

double ad_fsc_i(uint8_t fsc);
double ad_vrms(uint16_t asf, uint8_t fsc);
