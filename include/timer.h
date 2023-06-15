
#define M_216MHz 216000000.0

#define NS_TO_216MHZ_MU 0.216

void timer2_init();
void timer2_stop();
void timer2_restart();

void timer8_init();
void timer8_stop();
void timer8_restart();

void radar_emulator_start();

uint32_t timer_mu(uint32_t time_ns);
