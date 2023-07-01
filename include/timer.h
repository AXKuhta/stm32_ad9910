
#define M_216MHz 216000000.0

#define NS_TO_216MHZ_MU 0.216

#define MAX_NS_16BIT_216MHz ((1 << 16) - 1) / NS_TO_216MHZ_MU

void timer2_init();
void timer2_trgo_on_ch3();
void timer2_trgo_on_reset();
void timer2_stop();
void timer2_restart();

void timer8_init();
void timer8_reconfigure(uint32_t period);
void timer8_stop();
void timer8_restart();

void radar_emulator_start();

#define timer_mu(time_ns) (uint32_t)((time_ns) * NS_TO_216MHZ_MU + 0.5)
#define timer_ns(time_mu) ((double)(time_mu) / NS_TO_216MHZ_MU)
